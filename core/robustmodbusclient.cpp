#include "robustmodbusclient.h"
#include <QDebug>

// ========== 构造 / 析构 ==========
RobustModbusClient::RobustModbusClient(QModbusClient *client, QObject *parent) : QObject(parent), m_client(client) {
    if (m_client) {
        m_client->setTimeout(m_replyTimeoutMs);
        connect(m_client, &QModbusDevice::stateChanged, this, &RobustModbusClient::onModbusStateChanged);
    }
}

RobustModbusClient::~RobustModbusClient() {
    stopPolling();
    cancelAllRequests();
}

// ========== Modbus 状态变化 ==========
void RobustModbusClient::onModbusStateChanged(QModbusDevice::State state) {
    if (state == QModbusDevice::UnconnectedState) {
        cancelAllRequests();
    }
}

void RobustModbusClient::updateReadTask(const QModbusDataUnit::RegisterType type, const int serverId,
                                        const int startAddr, const int count, const quint32 userData) {
    m_readTasks[userData] = ReadUnit{type, serverId, startAddr, count};
}

// ========== 读请求（公开接口） ==========
quint64 RobustModbusClient::readHoldingRegisters(const int startAddr, const int count, const int serverId,
                                                 const quint32 userData) {
    return enqueueRequest(QModbusDataUnit::HoldingRegisters, startAddr, count, serverId, userData, false, nullptr);
}

quint64 RobustModbusClient::readInputRegisters(const int startAddr, const int count, const int serverId,
                                               const quint32 userData) {
    return enqueueRequest(QModbusDataUnit::InputRegisters, startAddr, count, serverId, userData, false, nullptr);
}

quint64 RobustModbusClient::readCoils(const int startAddr, const int count, const int serverId,
                                      const quint32 userData) {
    return enqueueRequest(QModbusDataUnit::Coils, startAddr, count, serverId, userData, false, nullptr);
}

quint64 RobustModbusClient::readDiscreteInputs(const int startAddr, const int count, const int serverId,
                                               const quint32 userData) {
    return enqueueRequest(QModbusDataUnit::DiscreteInputs, startAddr, count, serverId, userData, false, nullptr);
}

// ========== 写请求（高优先级插队） ==========
quint64 RobustModbusClient::writeHoldingRegister(const int addr, const quint16 value, const int serverId,
                                                 const quint32 userData) {
    const QList<quint16> values = {value};
    return enqueueRequest(QModbusDataUnit::HoldingRegisters, addr, 1, serverId, userData, true, &values);
}

quint64 RobustModbusClient::writeHoldingRegisters(const int startAddr, const QList<quint16> &values, const int serverId,
                                                  const quint32 userData) {
    if (values.isEmpty()) {
        qWarning() << "RobustModbusClient: 写入值为空";
        return 0;
    }
    const int count = static_cast<int>(values.size());
    if (count > 123) {
        qWarning() << "RobustModbusClient: 批量写入数量过多" << count;
        return 0;
    }
    return enqueueRequest(QModbusDataUnit::HoldingRegisters, startAddr, count, serverId, userData, true, &values);
}

quint64 RobustModbusClient::writeCoil(const int addr, const bool value, const int serverId, const quint32 userData) {
    const QList<quint16> values = {static_cast<quint16>(value ? 0xFF00 : 0x0000)};
    return enqueueRequest(QModbusDataUnit::Coils, addr, 1, serverId, userData, true, &values);
}

quint64 RobustModbusClient::writeCoils(const int startAddr, const QList<bool> &values, const int serverId,
                                       const quint32 userData) {
    if (values.isEmpty()) {
        qWarning() << "RobustModbusClient: 写入值为空";
        return 0;
    }
    const int count = static_cast<int>(values.size());
    if (count > 800) {
        qWarning() << "RobustModbusClient: 批量写入数量过多" << count;
        return 0;
    }
    QList<quint16> writeValues;
    writeValues.reserve(count);
    for (const bool v : values) {
        writeValues.append(static_cast<quint16>(v ? 0xFF00 : 0x0000));
    }
    return enqueueRequest(QModbusDataUnit::Coils, startAddr, count, serverId, userData, true, &writeValues);
}

// ========== 核心入队函数 ==========
quint64 RobustModbusClient::enqueueRequest(const QModbusDataUnit::RegisterType type, const int startAddr,
                                           const int count, const int serverId, const quint32 userData,
                                           const bool isWrite, const QList<quint16> *writeValues) {
    // 检查清理标志
    if (m_isClearing) {
        qWarning() << "RobustModbusClient: 正在清理中，拒绝入队";
        return 0;
    }

    if (!m_client || m_client->state() != QModbusDevice::ConnectedState) {
        qWarning() << "RobustModbusClient: 客户端未连接";
        return 0;
    }
    if (count <= 0 || count > 125) {
        qWarning() << "RobustModbusClient: 寄存器数量无效" << count;
        return 0;
    }

    // 显式转换避免收缩警告
    QModbusDataUnit newUnit(type, static_cast<quint16>(startAddr), static_cast<quint16>(count));
    if (isWrite && writeValues) {
        for (int i = 0; i < qMin(count, writeValues->size()); ++i) {
            newUnit.setValue(i, writeValues->at(i));
        }
    }

    // ---------- 请求合并（仅对读请求，且不合并写请求） ----------
    if (!isWrite) {
        for (int i = 0; i < m_pendingQueue.size(); ++i) {
            quint64 existId = m_pendingQueue.at(i);
            auto it = m_contexts.find(existId);
            if (it == m_contexts.end())
                continue;

            RequestContext &existCtx = it.value();
            // 只合并相同设备、相同类型、尚未激活、且不是写操作的请求
            if (existCtx.serverId != serverId || existCtx.unit.registerType() != type || existCtx.isActive ||
                existCtx.isWrite) {
                continue;
            }

            if (canMerge(existCtx.unit, newUnit)) {
                // 保存原始请求片段
                const OriginalRequest origA{existCtx.unit, existCtx.userData, existCtx.serverId};
                const OriginalRequest origB{newUnit, userData, serverId};
                existCtx.originalRequests.append(origA);
                existCtx.originalRequests.append(origB);

                // 合并成大范围请求
                existCtx.unit = mergeUnits(existCtx.unit, newUnit);

                qDebug() << "请求合并 ID:" << existId << "新范围:" << existCtx.unit.startAddress()
                         << "长度:" << existCtx.unit.valueCount();

                // 将合并后的请求移到队列头部，优先处理
                m_pendingQueue.removeAt(i);
                m_pendingQueue.push_front(existId);
                return existId;
            }
        }
    }

    // ---------- 新建请求 ----------
    const quint64 id = ++m_nextId;
    RequestContext ctx;
    ctx.id = id;
    ctx.unit = newUnit;
    ctx.serverId = serverId;
    ctx.userData = userData;
    ctx.maxRetries = m_maxRetries;
    ctx.isActive = false;
    ctx.isCancelled = false;
    ctx.isWrite = isWrite;

    m_contexts.insert(id, ctx);

    // 写请求插队到最前面，读请求排到队尾
    if (isWrite) {
        m_pendingQueue.push_front(id);
        qDebug() << "写请求插队 ID:" << id << "地址:" << startAddr;
    } else {
        m_pendingQueue.enqueue(id);
    }

    processQueue();
    return id;
}

// ========== 队列调度器 ==========
void RobustModbusClient::processQueue() {
    // 1. 清理僵尸请求（reply 被意外销毁）
    {
        QMutableMapIterator<quint64, RequestContext> it(m_contexts);
        while (it.hasNext()) {
            it.next();
            RequestContext &ctx = it.value();
            if (ctx.isActive && ctx.reply.isNull()) {
                qWarning() << "请求" << ctx.id << "的 reply 意外销毁";
                cleanupRequest(ctx.id, true); // 内部会发射信号（已修正拆分）
                it.remove();                  // 物理删除
            }
        }
    }

    // 2. 发送新请求（受并发数限制）
    while (m_activeCount < m_maxConcurrent && !m_pendingQueue.isEmpty()) {
        quint64 id = m_pendingQueue.dequeue();
        auto it = m_contexts.find(id);
        if (it == m_contexts.end())
            continue;

        const RequestContext &ctx = it.value();
        if (ctx.isCancelled) {
            cleanupRequest(id, false); // 不发射信号（已由 cancel 发射）
            m_contexts.remove(id);
            continue;
        }
        sendRequest(id);
    }

    // 3. 检查是否全部完成
    if (m_activeCount == 0 && m_pendingQueue.isEmpty() && m_contexts.isEmpty()) {
        emit allRequestsCompleted();
    }
}

// ========== 辅助函数：发射完成信号（自动处理拆分） ==========
void RobustModbusClient::emitCompleted(const quint64 requestId, RequestContext &ctx, const bool success,
                                       const QString &errorMsg, const QModbusDataUnit *resultData) {
    if (!ctx.originalRequests.isEmpty()) {
        // 合并请求：遍历原始片段
        for (const OriginalRequest &orig : ctx.originalRequests) {
            if (success && resultData) {
                // 成功：拆分数据
                QModbusDataUnit subData(orig.unit.registerType(), orig.unit.startAddress(), orig.unit.valueCount());
                const int offset = orig.unit.startAddress() - resultData->startAddress();
                const int count = static_cast<int>(orig.unit.valueCount());
                for (int i = 0; i < count; ++i) {
                    const int idx = offset + i;
                    if (idx >= 0 && idx < static_cast<int>(resultData->valueCount())) {
                        subData.setValue(i, resultData->value(idx));
                    }
                }
                emit requestCompleted(requestId, subData, true, "", orig.userData);
            } else {
                // 失败或取消：直接使用原始单元
                emit requestCompleted(requestId, orig.unit, false, errorMsg, orig.userData);
            }
        }
    } else {
        // 未合并：直接使用上下文的单元
        if (success && resultData) {
            emit requestCompleted(requestId, *resultData, true, "", ctx.userData);
        } else {
            emit requestCompleted(requestId, ctx.unit, false, errorMsg, ctx.userData);
        }
    }
}

// ========== 实际发送请求 ==========
void RobustModbusClient::sendRequest(quint64 requestId) {
    const auto it = m_contexts.find(requestId);
    if (it == m_contexts.end())
        return;

    RequestContext &ctx = it.value();
    QModbusReply *reply = nullptr;

    if (ctx.isWrite) {
        reply = m_client->sendWriteRequest(ctx.unit, ctx.serverId);
    } else {
        reply = m_client->sendReadRequest(ctx.unit, ctx.serverId);
    }

    if (!reply) {
        qWarning() << "发送请求失败 ID:" << requestId << m_client->errorString();
        if (ctx.retryCount < ctx.maxRetries) {
            ctx.retryCount++;
            QTimer::singleShot(m_retryDelayMs, this, [this, requestId]() { retryRequest(requestId); });
        } else {
            // 重试耗尽，发射失败信号（自动拆分）
            emitCompleted(requestId, ctx, false, m_client->errorString(), nullptr);
            cleanupRequest(requestId, false);
            m_contexts.remove(requestId);
        }
        return;
    }

    ctx.reply = reply;
    ctx.isActive = true;
    ctx.sentTime = QDateTime::currentDateTime();
    m_activeCount++;

    // 绑定完成信号
    QPointer<RobustModbusClient> self(this);
    connect(reply, &QModbusReply::finished, this, [self, this]() {
        if (self)
            onReplyFinished();
    });

    // 独立超时定时器
    const auto timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->start(m_replyTimeoutMs);
    m_timeoutTimers.insert(requestId, timer);
    connect(timer, &QTimer::timeout, this, [self, this]() {
        if (self)
            onTimeoutCheck();
    });

    emit requestProgress(m_activeCount + static_cast<int>(m_pendingQueue.size()),
                         static_cast<int>(m_contexts.size() - m_pendingQueue.size()));
}

// ========== 回复处理 ==========
void RobustModbusClient::onReplyFinished() {
    const auto finishedReply = qobject_cast<QModbusReply *>(sender());
    if (!finishedReply)
        return;

    // 查找对应的请求
    quint64 requestId = 0;
    RequestContext *ctx = nullptr;
    for (auto &context : m_contexts) {
        if (context.reply == finishedReply) {
            requestId = context.id;
            ctx = &context;
            break;
        }
    }
    if (!ctx) {
        finishedReply->deleteLater();
        return;
    }

    // 清理超时定时器
    if (m_timeoutTimers.contains(requestId)) {
        m_timeoutTimers[requestId]->stop();
        m_timeoutTimers[requestId]->deleteLater();
        m_timeoutTimers.remove(requestId);
    }

    // 如果已被取消，直接清理并删除
    if (ctx->isCancelled) {
        cleanupRequest(requestId, false);
        m_contexts.remove(requestId);
        finishedReply->deleteLater();
        processQueue();
        return;
    }

    bool success = false;
    QModbusDataUnit result;
    QString errorMsg;

    if (finishedReply->error() == QModbusDevice::NoError) {
        success = true;
        result = finishedReply->result();
    } else {
        errorMsg = finishedReply->errorString();
        qWarning() << "请求" << requestId << "失败:" << errorMsg;

        // 重试逻辑
        if (ctx->retryCount < ctx->maxRetries) {
            ctx->retryCount++;
            qDebug() << "重试" << requestId << ctx->retryCount << "/" << ctx->maxRetries;
            cleanupRequest(requestId, false); // 释放资源，保留节点
            retryRequest(requestId);
            finishedReply->deleteLater();
            return;
        }
    }

    // ---------- 最终完成（成功或不可重试失败） ----------
    // 清理资源（不发射信号，我们手动发射）
    cleanupRequest(requestId, false);

    // 发射完成信号（自动拆分）
    if (success) {
        emitCompleted(requestId, *ctx, true, "", &result);
    } else {
        emitCompleted(requestId, *ctx, false, errorMsg, nullptr);
    }

    // 物理删除节点
    m_contexts.remove(requestId);
    finishedReply->deleteLater();
    processQueue();
}

// ========== 超时检查 ==========
void RobustModbusClient::onTimeoutCheck() {
    const auto timer = qobject_cast<QTimer *>(sender());
    if (!timer)
        return;

    quint64 requestId = 0;
    for (auto it = m_timeoutTimers.begin(); it != m_timeoutTimers.end(); ++it) {
        if (it.value() == timer) {
            requestId = it.key();
            break;
        }
    }
    if (requestId == 0)
        return;

    const auto ctxIt = m_contexts.find(requestId);
    if (ctxIt == m_contexts.end()) {
        m_timeoutTimers.remove(requestId);
        timer->deleteLater();
        return;
    }

    RequestContext &ctx = ctxIt.value();
    qWarning() << "请求" << requestId << "超时";

    if (ctx.reply) {
        ctx.reply->deleteLater();
        ctx.reply.clear();
    }

    ctx.isActive = false;
    m_activeCount--;
    m_timeoutTimers.remove(requestId);
    timer->deleteLater();

    // 重试
    if (ctx.retryCount < ctx.maxRetries) {
        ctx.retryCount++;
        qDebug() << "超时重试" << requestId << ctx.retryCount << "/" << ctx.maxRetries;
        QTimer::singleShot(m_retryDelayMs, this, [this, requestId]() { retryRequest(requestId); });
    } else {
        // 重试耗尽，发射超时失败信号（自动拆分）
        emitCompleted(requestId, ctx, false, "Timeout after retries", nullptr);
        cleanupRequest(requestId, false);
        m_contexts.remove(requestId);
        processQueue();
    }
}

// ========== 重试 ==========
void RobustModbusClient::retryRequest(const quint64 requestId) {
    const auto it = m_contexts.find(requestId);
    if (it == m_contexts.end())
        return;

    RequestContext &ctx = it.value();
    if (ctx.isCancelled) {
        cleanupRequest(requestId, false);
        m_contexts.remove(requestId);
        return;
    }

    // 防御：如果还在队列里（理论上不会），先移除防止重复
    for (int i = 0; i < m_pendingQueue.size(); ++i) {
        if (m_pendingQueue.at(i) == requestId) {
            m_pendingQueue.removeAt(i);
            break;
        }
    }

    ctx.isActive = false;
    m_pendingQueue.enqueue(requestId);
    processQueue();
}

// ========== 清理资源（不删除节点） ==========
void RobustModbusClient::cleanupRequest(const quint64 requestId, const bool emitSignal) {
    const auto it = m_contexts.find(requestId);
    if (it == m_contexts.end())
        return;

    RequestContext &ctx = it.value();

    if (m_timeoutTimers.contains(requestId)) {
        m_timeoutTimers[requestId]->stop();
        m_timeoutTimers[requestId]->deleteLater();
        m_timeoutTimers.remove(requestId);
    }

    if (ctx.reply) {
        ctx.reply->deleteLater();
        ctx.reply.clear();
    }

    if (ctx.isActive) {
        ctx.isActive = false;
        m_activeCount--;
    }

    if (emitSignal) {
        // 发射取消信号（自动拆分）
        emitCompleted(requestId, ctx, false, "Cancelled", nullptr);
    }
}

// ========== 取消单个请求 ==========
bool RobustModbusClient::cancelRequest(const quint64 requestId) {
    const auto it = m_contexts.find(requestId);
    if (it == m_contexts.end())
        return false;

    RequestContext &ctx = it.value();
    ctx.isCancelled = true;

    // 从等待队列中移除（如果存在）
    for (int i = 0; i < m_pendingQueue.size(); ++i) {
        if (m_pendingQueue.at(i) == requestId) {
            m_pendingQueue.removeAt(i);
            break;
        }
    }

    // 清理资源
    if (ctx.isActive) {
        cleanupRequest(requestId, true); // 内部会调用 emitCompleted 发射取消信号
    } else {
        // 未发出的请求，直接发射取消信号（自动拆分）
        emitCompleted(requestId, ctx, false, "Cancelled", nullptr);
    }

    // 物理删除节点
    m_contexts.remove(requestId);
    return true;
}

// ========== 取消所有请求 ==========
void RobustModbusClient::cancelAllRequests() {
    // 防止重入
    if (m_isClearing) {
        qWarning() << "RobustModbusClient: 已有清理操作正在进行";
        return;
    }
    m_isClearing = true;

    // 1. 拷贝所有 ID，避免在迭代过程中因 erase 导致迭代器失效
    QList<quint64> allIds = m_contexts.keys();

    // 2. 逐个精准取消（复用 cancelRequest 的全部逻辑，内部已包含信号发射和节点删除）
    for (const quint64 id : allIds) {
        cancelRequest(id);
    }

    // 3. 二次保险：强制清空队列（防止 cancelRequest 中因特殊原因漏删）
    m_pendingQueue.clear();

    // 4. 强制归零活跃计数（防止 cancelRequest 中因计数异常导致残留）
    m_activeCount = 0;

    // 5. 最终安全检查：如果还有残留（理论上不会），暴力清空并警告
    if (!m_contexts.isEmpty()) {
        qWarning() << "cancelAllRequests: 存在未处理的残留上下文，强制清空";
        m_contexts.clear();
    }

    // 6. 恢复清理标志
    m_isClearing = false;

    // 7. 通知外部系统进入空闲状态
    if (m_pendingQueue.isEmpty() && m_contexts.isEmpty()) {
        emit allRequestsCompleted();
    }

    qDebug() << "RobustModbusClient: 所有请求已彻底取消并清理";
}

// ========== 定时轮询 ==========
void RobustModbusClient::startPolling(const int intervalMs) {
    m_pollIntervalMs = intervalMs;
    if (!m_pollTimer) {
        m_pollTimer = new QTimer(this);
        connect(m_pollTimer, &QTimer::timeout, this, &RobustModbusClient::onPollingTimeout);
    }
    // QTimer::singleShot(0, this, &m_pollTimer);
    m_pollTimer->start(m_pollIntervalMs);
}

void RobustModbusClient::stopPolling() const {
    if (m_pollTimer) {
        m_pollTimer->stop();
    }
}

void RobustModbusClient::setPollingInterval(int intervalMs) {
    m_pollIntervalMs = intervalMs;
    if (m_pollTimer && m_pollTimer->isActive()) {
        m_pollTimer->start(m_pollIntervalMs);
    }
}

void RobustModbusClient::onPollingTimeout() {
    for (auto it = m_readTasks.constBegin(); it != m_readTasks.constEnd(); ++it) {
        const auto &userData = it.key();
        const ReadUnit &readUnit = it.value();
        switch (m_readTasks.value(userData).type) {
            case QModbusDataUnit::Coils:
                readCoils(readUnit.startAddr, readUnit.count, readUnit.serverId, userData);
                break;
            case QModbusDataUnit::DiscreteInputs:
                readDiscreteInputs(readUnit.startAddr, readUnit.count, readUnit.serverId, userData);
                break;
            case QModbusDataUnit::HoldingRegisters:
                readHoldingRegisters(readUnit.startAddr, readUnit.count, readUnit.serverId, userData);
                break;
            case QModbusDataUnit::InputRegisters:
                readInputRegisters(readUnit.startAddr, readUnit.count, readUnit.serverId, userData);
                break;
            default:
                break;
        }
    }
}

// ========== 请求合并辅助函数 ==========
bool RobustModbusClient::canMerge(const QModbusDataUnit &a, const QModbusDataUnit &b) {
    if (a.registerType() != b.registerType())
        return false;
    const int aStart = a.startAddress();
    const int aEnd = aStart + static_cast<int>(a.valueCount()) - 1;
    const int bStart = b.startAddress();
    const int bEnd = bStart +static_cast<int>( b.valueCount()) - 1;
    // 连续或重叠即可合并
    return (bStart <= aEnd + 1 && bEnd >= aStart - 1);
}

QModbusDataUnit RobustModbusClient::mergeUnits(const QModbusDataUnit &a, const QModbusDataUnit &b) {
    const int newStart = qMin(a.startAddress(), b.startAddress());
    const int newEnd = qMax(a.startAddress() +static_cast<int>( a.valueCount()) - 1, b.startAddress() +static_cast<int>( b.valueCount()) - 1);
    const int newCount = newEnd - newStart + 1;
    return QModbusDataUnit{a.registerType(), static_cast<quint16>(newStart), static_cast<quint16>(newCount)};
}

// ========== 状态查询 ==========
bool RobustModbusClient::isRequestPending(const quint64 requestId) const { return m_contexts.contains(requestId); }