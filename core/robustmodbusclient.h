#ifndef ROBUSTMODBUSCLIENT_H
#define ROBUSTMODBUSCLIENT_H

#include <QAtomicInt>
#include <QDateTime>
#include <QMap>
#include <QModbusClient>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QTimer>

/*!
 * \brief RobustModbusClient - 工业级 Modbus 异步请求管理器
 *
 * 核心特性：
 * - 异步非阻塞，基于 Qt 事件循环
 * - 并发数控制（防止打爆设备）
 * - 自动重试（失败/超时）
 * - 请求合并（连续地址自动合并）
 * - 高优先级写入（插队）
 * - 定时轮询
 * - 彻底资源清理（无内存泄漏）
 * - 清理期间拒绝新请求（m_isClearing）
 */
class RobustModbusClient : public QObject {
    Q_OBJECT
  public:
    explicit RobustModbusClient(QModbusClient *client, QObject *parent = nullptr);
    ~RobustModbusClient() override;

    // ========== 配置接口 ==========
    void setMaxConcurrent(const int max) { m_maxConcurrent = max; }
    void setMaxRetries(const int max) { m_maxRetries = max; }
    void setReplyTimeout(const int ms) {
        m_replyTimeoutMs = ms;
        if (m_client)
            m_client->setTimeout(ms);
    }
    void setRetryDelay(const int ms) { m_retryDelayMs = ms; }

    void updateReadTask(QModbusDataUnit::RegisterType type, int serverId, int startAddr, int count, quint32 userData);

    // ========== 读请求（自动合并） ==========
    quint64 readHoldingRegisters(int startAddr, int count, int serverId = 1, quint32 userData = 0);
    quint64 readInputRegisters(int startAddr, int count, int serverId = 1, quint32 userData = 0);
    quint64 readCoils(int startAddr, int count, int serverId = 1, quint32 userData = 0);
    quint64 readDiscreteInputs(int startAddr, int count, int serverId = 1, quint32 userData = 0);

    // ========== 写请求（高优先级插队） ==========
    // 单个写入
    quint64 writeHoldingRegister(int addr, quint16 value, int serverId = 1, quint32 userData = 0);
    quint64 writeCoil(int addr, bool value, int serverId = 1, quint32 userData = 0);
    // 批量写入
    quint64 writeHoldingRegisters(int startAddr, const QList<quint16> &values, int serverId = 1, quint32 userData = 0);
    quint64 writeCoils(int startAddr, const QList<bool> &values, int serverId = 1, quint32 userData = 0);

    // ========== 定时轮询 ==========
    void startPolling(int intervalMs);
    void stopPolling() const;
    void setPollingInterval(int intervalMs);

    // ========== 取消请求 ==========
    bool cancelRequest(quint64 requestId);
    void cancelAllRequests();

    // ========== 状态查询 ==========
    [[nodiscard]] bool isRequestPending(quint64 requestId) const;
    [[nodiscard]] int pendingCount() const { return static_cast<int>(m_pendingQueue.size()) + m_activeCount; }

  signals:
    // 单个请求完成（成功或失败），userData 透传
    void requestCompleted(quint64 requestId, const QModbusDataUnit &data, bool success, QString errorMsg,
                          quint32 userData);
    // 进度信号
    void requestProgress(int total, int completed);
    // 所有请求完成
    void allRequestsCompleted();

  private slots:
    void processQueue();
    void onReplyFinished();
    void onTimeoutCheck();
    void onPollingTimeout();
    void onModbusStateChanged(QModbusDevice::State state);

  private:
    // ========== 数据结构 ==========
    struct OriginalRequest {
        QModbusDataUnit unit{};
        quint32 userData{};
        int serverId{};
    };

    struct RequestContext {
        quint64 id = 0;
        QModbusDataUnit unit;
        int serverId = 1;
        quint32 userData = 0;
        QPointer<QModbusReply> reply{nullptr};
        int retryCount = 0;
        int maxRetries = 3;
        bool isActive = false;
        bool isCancelled = false;
        bool isWrite = false;
        QDateTime sentTime;
        QTimer *timeoutTimer = nullptr;

        // 请求合并相关：存储合并前的原始片段
        QList<OriginalRequest> originalRequests;
    };

    // ========== 核心成员 ==========
    QModbusClient *m_client;
    QQueue<quint64> m_pendingQueue;
    QMap<quint64, RequestContext> m_contexts;
    QMap<quint64, QTimer *> m_timeoutTimers;

    quint64 m_nextId = 0;
    int m_maxConcurrent = 3;
    int m_maxRetries = 3;
    int m_replyTimeoutMs = 1000;
    int m_retryDelayMs = 200;
    int m_activeCount = 0;

    // 定时轮询
    QTimer *m_pollTimer = nullptr;
    int m_pollIntervalMs = 100;

    // 清理标志（防止重入及清理期间新请求入队）
    bool m_isClearing = false;

    // ========== 内部辅助函数 ==========
    quint64 enqueueRequest(QModbusDataUnit::RegisterType type, int startAddr, int count, int serverId, quint32 userData,
                           bool isWrite = false, const QList<quint16> *writeValues = nullptr);

    void cleanupRequest(quint64 requestId, bool emitSignal);
    void retryRequest(quint64 requestId);
    void sendRequest(quint64 requestId);

    // 请求合并辅助
    static bool canMerge(const QModbusDataUnit &a, const QModbusDataUnit &b);
    static QModbusDataUnit mergeUnits(const QModbusDataUnit &a, const QModbusDataUnit &b);

    // 辅助发射完成信号（自动处理合并拆分）
    void emitCompleted(quint64 requestId, RequestContext &ctx, bool success, const QString &errorMsg,
                       const QModbusDataUnit *resultData = nullptr);

    struct ReadUnit {
        QModbusDataUnit::RegisterType type;
        int serverId;
        int startAddr;
        int count;
    };
    QMap<quint32, ReadUnit> m_readTasks{};
};

#endif // ROBUSTMODBUSCLIENT_H