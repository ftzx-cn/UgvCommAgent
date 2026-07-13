#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // modbus读写类与重连定时器
    m_tcpClient = new QModbusTcpClient(this);
    m_robustModbusClient = new RobustModbusClient(m_tcpClient, this);
    // 初始化重连定时器（单次触发）
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        const QString ip = m_configSource->property("plcIp").toString();
        const int port = m_configSource->property("plcPort").toInt();
        spdlog::info("重连PLC - {}/{}", ip.toStdString(), QString::number(port).toStdString());
        m_tcpClient->connectDevice();
    });

    // 设置中心窗口和布局
    const auto centralWidget = new QFrame(this);
    this->setCentralWidget(centralWidget);
    const auto rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(3, 3, 3, 3);
    rootLayout->setSpacing(5);
    const auto workAreaLayout = new QVBoxLayout();
    rootLayout->addLayout(workAreaLayout);

    // 创建工具栏并设置属性
    m_toolBar = this->addToolBar(tr("工具栏"));
    m_toolBar->setMovable(false);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->setIconSize(QSize(32, 32));
    m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);

    // 添加工具栏按钮
    m_toolBar->addSeparator();
    m_connectPlcAction = m_toolBar->addAction(QIcon(R"(:/icons/批量启动.svg)"), tr("连接PLC"));
    m_disconnectPlcAction = m_toolBar->addAction(QIcon(R"(:/icons/批量停止.svg)"), tr("断开PLC"));

    m_plcStatusIcon = new QLabel(m_toolBar);
    m_plcStatusIcon->setFixedWidth(48);
    m_plcStatusIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_toolBar->addWidget(m_plcStatusIcon);
    m_toolBar->addSeparator();

    auto spacerFrame = new QFrame(m_toolBar);
    spacerFrame->setFixedWidth(16);
    m_toolBar->addWidget(spacerFrame);

    m_connectWebSocketAction = m_toolBar->addAction(QIcon(R"(:/icons/云同步-sync.svg)"), tr("连接WS"));
    m_disconnectWebSocketAction = m_toolBar->addAction(QIcon(R"(:/icons/云失败-fail.svg)"), tr("中断WS"));
    m_webSocketStatusIcon = new QLabel(m_toolBar);
    m_webSocketStatusIcon->setFixedWidth(48);
    m_webSocketStatusIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_toolBar->addWidget(m_webSocketStatusIcon);
    m_toolBar->addSeparator();

    m_saveAction = m_toolBar->addAction(QIcon(R"(:/icons/保存.svg)"), tr("保存设置"));

    m_toolBar->addSeparator();

    // 创建监控区容器，设置属性，添加标题，并将其加入到工作区布局中
    const auto monitorContainer = new QFrame(centralWidget);
    monitorContainer->setFixedHeight(360);
    monitorContainer->setMinimumWidth(720);
    monitorContainer->setFrameShape(QFrame::Box);
    monitorContainer->setFrameShadow(QFrame::Raised);
    monitorContainer->setLineWidth(1);
    const auto monitorContainerLayout = new QVBoxLayout(monitorContainer);
    monitorContainerLayout->addWidget(new QLabel(tr("设备监控"), monitorContainer));
    workAreaLayout->addWidget(monitorContainer);
    // 创建泵和液位监控布局，并将其加入到监控区容器布局中
    const auto tankPumpLayout = new QHBoxLayout();
    monitorContainerLayout->addLayout(tankPumpLayout);
    // 添加桶监控部件
    m_tank = new TankWidget(monitorContainer);
    m_tank->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_tank->setShape(TankWidget::TankShape::Spherical);
    m_tank->setUsedValue(true, false, false);
    tankPumpLayout->addWidget(m_tank);
    // 添加泵布局和部件
    const auto pumpLayout = new QVBoxLayout();
    tankPumpLayout->addLayout(pumpLayout);
    m_pump = new PumpWidget(monitorContainer);
    m_switchMode = new SwitchButtonWidget(monitorContainer);
    pumpLayout->addWidget(m_pump);
    pumpLayout->addWidget(m_switchMode);

    pumpLayout->addStretch();
    pumpLayout->setAlignment(m_pump, Qt::AlignTop | Qt::AlignHCenter);
    pumpLayout->setAlignment(m_switchMode, Qt::AlignTop | Qt::AlignHCenter);
    pumpLayout->setSpacing(0);
    tankPumpLayout->setStretch(0, 1);
    tankPumpLayout->setStretch(1, 1);

    // 创建日志区容器，设置属性，添加标题
    const auto logViewContainer = new QFrame(centralWidget);
    logViewContainer->setFrameShape(QFrame::Box);
    logViewContainer->setFrameShadow(QFrame::Raised);
    logViewContainer->setLineWidth(1);
    const auto logViewContainerLayout = new QVBoxLayout(logViewContainer);
    logViewContainerLayout->addWidget(new QLabel(tr("日志信息"), logViewContainer));
    m_logView = new QTextEdit(logViewContainer);
    m_logView->setReadOnly(true);
    logViewContainerLayout->addWidget(m_logView);
    workAreaLayout->addWidget(logViewContainer);
    // logView自动滚动到底部
    connect(m_logView, &QTextEdit::textChanged,
            [=]() { m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum()); });

    // 创建参数设置区容器，设置属性，添加标题和水平分隔线
    const auto configContainer = new QFrame(centralWidget);
    configContainer->setFrameShape(QFrame::Box);
    configContainer->setFrameShadow(QFrame::Raised);
    configContainer->setLineWidth(1);
    configContainer->setFixedWidth(400);
    const auto configContainerLayout = new QVBoxLayout(configContainer);
    configContainerLayout->addWidget(new QLabel(tr("参数设置"), configContainer));
    //  创建propertybrowser和封装的propertymodel，并将browser、manager和factory三者关联
    m_configBrowser = new QtTreePropertyBrowser(this);
    configContainerLayout->addWidget(m_configBrowser);

    m_propertyManager = new QtVariantPropertyManager(this);
    m_editFactory = new QtVariantEditorFactory(this);
    m_configBrowser->setFactoryForManager(m_propertyManager, m_editFactory);

    m_configBrowser->setResizeMode(QtTreePropertyBrowser::Interactive);
    m_configBrowser->setSplitterPosition(200);

    m_configSource = new ConfigSource(this);
    m_configSource->loadFromFile("config.json");
    m_configModel = new ConfigModel(this);
    m_configModel->setPorpertyManager(m_propertyManager);
    m_configModel->setConfigSource(m_configSource);
    m_configModel->buildPropertyBinding();

    rootLayout->addWidget(configContainer);

    for (const auto &property : m_configModel->properties()) {
        if (property->propertyType() != QtVariantPropertyManager::groupTypeId()) {
            if (!property->parentProperty()) {
                m_configBrowser->addProperty(property);
                property->setEnabled(false);
            }
        }
    }

    for (const auto &property : m_configModel->properties()) {
        if (property->propertyType() == QtVariantPropertyManager::groupTypeId()) {
            if (!property->parentProperty()) {
                m_configBrowser->addProperty(property);
            }
        }
    }

    connect(m_propertyManager, &QtVariantPropertyManager::valueChanged,
            [this](QtProperty *prop, const QVariant &value) {
                const auto property = dynamic_cast<QtVariantProperty *>(prop);
                if (!property) {
                    return;
                }
                m_configModel->updateSourceProperty(property, value);
                if (m_configModel->equalValues(property)) {
                    prop->setModified(false);
                } else {
                    prop->setModified(true);
                }
            });

    connect(m_saveAction, &QAction::triggered, [this]() { m_configSource->saveToFile(); });

    // 启动PLC连接
    connect(m_connectPlcAction, &QAction::triggered, [this]() {
        if (!m_connectPlcAction->isEnabled() && m_disconnectPlcAction->isEnabled()) {
            return;
        }
        m_connectPlcAction->setEnabled(false);
        m_disconnectPlcAction->setEnabled(true);
        setPropertiesEnabled("PLC连接配置", false);
        setPropertiesEnabled("设备监控配置", false);

        setPlcConnectionParameter();
        setReadTasks();

        const QString deviceTankLevelRange = m_configSource->property("deviceTankLevelRange").toString();
        if (QStringList partsForLevelRange = deviceTankLevelRange.trimmed().split(':', Qt::SkipEmptyParts);
            partsForLevelRange.size() == 2) {
            bool ok1, ok2;
            const double out1 = partsForLevelRange[0].trimmed().toDouble(&ok1);
            const double out2 = partsForLevelRange[1].trimmed().toDouble(&ok2);
            if (ok1 && ok2 && out1 < out2) {
                m_tank->setHeightConfig(out1, out2, "m");
            }
        }

        const QString deviceAiScaleRange = m_configSource->property("deviceAiScaleRange").toString();
        if (QStringList parts = deviceAiScaleRange.trimmed().split(':', Qt::SkipEmptyParts); parts.size() == 2) {
            bool ok1, ok2;
            const int out1 = parts[0].trimmed().toInt(&ok1);
            const int out2 = parts[1].trimmed().toInt(&ok2);
            if (ok1 && ok2 && out1 < out2) {
                m_aiScaleRange.min = out1;
                m_aiScaleRange.max = out2;
            }
        }
        emit m_tcpClient->stateChanged(m_tcpClient->state());
    });
    // 断开PLC连接
    connect(m_disconnectPlcAction, &QAction::triggered, [this]() {
        if (m_connectPlcAction->isEnabled() && !m_disconnectPlcAction->isEnabled()) {
            return;
        }

        if (m_reconnectTimer->isActive()) {
            m_reconnectTimer->stop();
        }

        m_connectPlcAction->setEnabled(true);
        m_disconnectPlcAction->setEnabled(false);
        setPropertiesEnabled("PLC连接配置", true);
        setPropertiesEnabled("设备监控配置", true);

        emit m_tcpClient->stateChanged(m_tcpClient->state());
    });

    connect(m_tcpClient, &QModbusTcpClient::stateChanged, [this]() {
        const QString ip = m_configSource->property("plcIp").toString();
        const int port = m_configSource->property("plcPort").toInt();
        const int pollingIntervalMs = m_configSource->property("plcPollingIntervalMs").toInt();
        const int plcConnectRetryIntervalMs = m_configSource->property("plcConnectRetryIntervalMs").toInt();
        struct ConnectSession {
            int sessionId{0};
            int reconnectCount{0};
        };
        static ConnectSession connectSession{0};
        switch (m_tcpClient->state()) {
            case QModbusDevice::ConnectedState:
                m_plcStatusIcon->setPixmap(QIcon(R"(:/icons/主机正常.svg)").pixmap(QSize(30, 30)));
                if (m_reconnectTimer->isActive()) {
                    m_reconnectTimer->stop();
                }
                if (!m_connectPlcAction->isEnabled() && m_disconnectPlcAction->isEnabled()) {
                    spdlog::info("成功连接PLC - {}/{},开始轮询", ip.toStdString(), QString::number(port).toStdString());
                    m_robustModbusClient->startPolling(pollingIntervalMs);
                } else {
                    spdlog::info("开始断开PLC连接 - {}/{}", ip.toStdString(), QString::number(port).toStdString());
                    m_tcpClient->disconnectDevice();
                }
                connectSession = {1, 0};
                break;
            case QModbusDevice::UnconnectedState:
                m_plcStatusIcon->setPixmap(QIcon(R"(:/icons/主机异常.svg)").pixmap(QSize(30, 30)));
                if (!m_connectPlcAction->isEnabled() && m_disconnectPlcAction->isEnabled()) {
                    if (!m_reconnectTimer->isActive()) {
                        if (connectSession.sessionId == 0) {
                            spdlog::info("开始连接PLC - {}/{}", ip.toStdString(), QString::number(port).toStdString());
                            connectSession.sessionId = 1;
                            m_tcpClient->connectDevice();
                        } else {
                            if (connectSession.reconnectCount == 0) {
                                spdlog::warn("连接中断，重新连接PLC - {}/{}", ip.toStdString(),
                                             QString::number(port).toStdString());
                                m_tcpClient->connectDevice();
                            } else {
                                spdlog::warn("连接PLC失败 - {}/{}, 失败次数: {}, {}ms后重新连接!", ip.toStdString(),
                                             QString::number(port).toStdString(),
                                             QString::number(connectSession.reconnectCount).toStdString(),
                                             QString::number(plcConnectRetryIntervalMs).toStdString());
                                m_reconnectTimer->start(plcConnectRetryIntervalMs);
                            }
                        }
                        ++connectSession.reconnectCount;
                    }
                } else {
                    if (m_reconnectTimer->isActive()) {
                        m_reconnectTimer->stop();
                    }
                    m_robustModbusClient->stopPolling();
                    m_robustModbusClient->cancelAllRequests();
                    spdlog::info("已断开PLC连接 - {}/{}", ip.toStdString(), QString::number(port).toStdString());
                    connectSession = {0, 0};
                }
                break;
            case QModbusDevice::ClosingState:
            case QModbusDevice::ConnectingState:
                m_plcStatusIcon->setPixmap(QIcon(R"(:/icons/正在连接.svg)").pixmap(QSize(32, 32)));
                break;
            default:
                break;
        }
    });

    connect(m_robustModbusClient, &RobustModbusClient::requestCompleted, this,
            &MainWindow::handleModbusMessageReceived);

    spdlog::init_thread_pool(8192, 1);
    spdlog::flush_every(std::chrono::seconds(30)); // 5秒写入磁盘
    spdlog::flush_on(spdlog::level::err);          // error级别实时写入，其他定时

    const auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/app.log", 10 * 1024 * 1024,
                                                                                  5); // 滚动文件 10MB × 5 备份

    std::vector<spdlog::sink_ptr> sinks{file_sink};

    const auto global_logger = std::make_shared<spdlog::async_logger>(
        "global", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::register_logger(global_logger);
    spdlog::set_default_logger(global_logger);
    global_logger->set_level(spdlog::level::trace);

    m_logview_sink = std::make_shared<spdlog::sinks::qt_color_sink_mt>(m_logView, 1000, true, true);
    m_logview_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l] %v%$");
    spdlog::get("global")->sinks().push_back(m_logview_sink);

    // 分割线，醒目区分启动日志块
    spdlog::info("-------------------------------------------------------------------------------------");
    spdlog::info("Application started successfully!");
    spdlog::info("");
    spdlog::info("App Name:\t{}", QApplication::applicationName().toStdString());
    spdlog::info("App Version:\t{}", QApplication::applicationVersion().toStdString());
    spdlog::info("Qt Version:\t{}", QT_VERSION_STR);
    spdlog::info("LocalPc Byte Order:\t{}", "Little Endian");
    spdlog::info(""); // 空行分隔后续业务日志
    spdlog::info("System service is running, ready to process tasks...");
    spdlog::info("-------------------------------------------------------------------------------------");

    connect(m_pump, &PumpWidget::remoteStateChanaged, m_switchMode, [this](const PumpWidget::RemoteState state) {
        if (state != PumpWidget::RemoteState::Remote) {
            m_switchMode->setEnabled(false);
        } else {
            m_switchMode->setEnabled(true);
        }
    });

    connect(m_switchMode, &SwitchButtonWidget::toggled, [this](const bool checked) {
        static QMap<int, QModbusDataUnit::RegisterType> registerTypeMap{};
        registerTypeMap.insert(0, QModbusDataUnit::Coils);
        registerTypeMap.insert(1, QModbusDataUnit::DiscreteInputs);
        registerTypeMap.insert(2, QModbusDataUnit::HoldingRegisters);
        registerTypeMap.insert(3, QModbusDataUnit::InputRegisters);

        static QMap<QModbusDataUnit::RegisterType, std::string> registerNameMap{};
        registerNameMap.insert(QModbusDataUnit::Coils, "线圈");
        registerNameMap.insert(QModbusDataUnit::DiscreteInputs, "离散输入");
        registerNameMap.insert(QModbusDataUnit::HoldingRegisters, "保存寄存器");
        registerNameMap.insert(QModbusDataUnit::InputRegisters, "输入寄存器");

        const int serverId = m_configSource->property("plcSlaveId").toInt();

        const int devicePumpCtrlFuncCode = m_configSource->property("devicePumpCtrlFuncCode").toInt();
        const int devicePumpCtrlAddress = m_configSource->property("devicePumpCtrlAddress").toInt();

        if (devicePumpCtrlFuncCode == 0) {
            m_robustModbusClient->writeCoil(devicePumpCtrlAddress, checked, serverId, 10);
            spdlog::info("写线圈");
        }
    });

    emit m_pump->remoteStateChanaged(m_pump->remoteState());
    emit m_connectPlcAction->trigger();

    m_webSocketClient = new WebSocketClient(this);
    connect(m_webSocketClient, &WebSocketClient::textMessageReceived, this, &MainWindow::handleWebSocketMessage);
    connect(m_webSocketClient, &WebSocketClient::connected, [this]() {
        m_webSocketStatusIcon->setPixmap(QIcon(R"(:/icons/网络_正常-copy.svg)").pixmap(QSize(28, 28)));
        spdlog::info("WebSocket 已连接");
    });
    connect(m_webSocketClient, &WebSocketClient::disconnected, [this]() {
        m_webSocketStatusIcon->setPixmap(QIcon(R"(:/icons/网络_异常-copy.svg)").pixmap(QSize(28, 28)));
        spdlog::warn("WebSocket 断开");
    });

    connect(m_connectWebSocketAction, &QAction::triggered, [this]() {
        const QString wsUrl = m_configSource->property("wsServerUrl").toString();
        const int reconnectInterval = m_configSource->property("wsReconnectIntervalMs").toInt();
        m_webSocketClient->setReconnectIntervalMs(reconnectInterval);
        m_connectWebSocketAction->setEnabled(false);
        m_disconnectWebSocketAction->setEnabled(true);
        setPropertiesEnabled("WebSocket配置", false);
        m_webSocketClient->connectToServer(wsUrl);
        spdlog::info("开始连接WS服务器 - {}", wsUrl.toStdString());
    });

    connect(m_disconnectWebSocketAction, &QAction::triggered, [this]() {
        const QString wsUrl = m_configSource->property("wsServerUrl").toString();
        spdlog::info("断开WS服务器 - {}", wsUrl.toStdString());
        m_connectWebSocketAction->setEnabled(true);
        m_disconnectWebSocketAction->setEnabled(false);
        setPropertiesEnabled("WebSocket配置", true);
        if (m_webSocketClient) {
            m_webSocketClient->disconnectFromServer();
        }
    });

    emit m_connectWebSocketAction->trigger();
}

MainWindow::~MainWindow() {
    if (m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }

    if (m_robustModbusClient) {
        m_robustModbusClient->stopPolling();
        m_robustModbusClient->cancelAllRequests();
        disconnect(m_robustModbusClient, nullptr, nullptr, nullptr);
        disconnect(nullptr, nullptr, m_robustModbusClient, nullptr);
    }

    if (m_tcpClient) {
        m_tcpClient->disconnectDevice();
    }

    if (m_webSocketClient) {
        m_webSocketClient->disconnectFromServer();
    }

    if (const auto logger = spdlog::default_logger()) {
        auto &sinks = logger->sinks();
        if (const auto it = std::find(sinks.begin(), sinks.end(), m_logview_sink); it != sinks.end()) {
            sinks.erase(it);
        }
    }

    spdlog::shutdown();
}

void MainWindow::setPlcConnectionParameter() const {
    const QString ip = m_configSource->property("plcIp").toString();
    const int port = m_configSource->property("plcPort").toInt();
    const int responseTimeoutMs = m_configSource->property("plcResponseTimeoutMs").toInt();
    const int connectRetryCount = m_configSource->property("plcConnectRetryCount").toInt();
    m_tcpClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ip);
    m_tcpClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    m_tcpClient->setTimeout(responseTimeoutMs);
    m_tcpClient->setNumberOfRetries(connectRetryCount);

    const int pollingIntervalMs = m_configSource->property("plcPollingIntervalMs").toInt();
    const int readRetryCount = m_configSource->property("plcReadRetryCount").toInt();
    const int readRetryIntervalMs = m_configSource->property("plcReadRetryIntervalMs").toInt();
    m_robustModbusClient->setMaxRetries(readRetryCount);
    m_robustModbusClient->setRetryDelay(readRetryIntervalMs);
    m_robustModbusClient->setPollingInterval(pollingIntervalMs);

    spdlog::info("");
    spdlog::info("-------------------------------------------------------------------------------------");
    spdlog::info("设置PLC连接参数: {}/{}, 响应超时: {}ms, 重连次数: {}", ip.toStdString(),
                 QString::number(port).toStdString(), QString::number(responseTimeoutMs).toStdString(),
                 QString::number(connectRetryCount).toStdString());
    spdlog::info(""); // 空行分隔后续业务日志
    spdlog::info("设置轮询参数: 轮询间隔 : {}ms, 重读次数: {}， 重读间隔: {}ms",
                 QString::number(pollingIntervalMs).toStdString(), QString::number(readRetryCount).toStdString(),
                 QString::number(readRetryIntervalMs).toStdString());
}

void MainWindow::handleModbusMessageReceived(quint64 requestId, const QModbusDataUnit &data, bool success,
                                             const QString &errorMsg, const quint32 userData) const {
    static QMap<quint32, std::string> userDataMap{
        {1, "deviceRemoteMode"}, {2, "devicePumpStatus"}, {3, "deviceBarrelLevel"}, {10, "devicePumpCtrl"}};
    struct StateCount {
        int successCount = 0;
        int faultCount = 0;
    };
    static QMap<quint32, StateCount> userStateMap{{1, {0, 0}}, {2, {0, 0}}, {3, {0, 0}}, {10, {0, 0}}};

    switch (userData) {
        case 1:
            if (!success) {
                if (userStateMap.value(userData).faultCount == 0) {
                    m_pump->setRemoteState(PumpWidget::RemoteState::Null);
                    ++userStateMap[userData].faultCount;
                    userStateMap[userData].successCount = 0;
                    spdlog::warn("点位 ({}) 读取错误: {}", userDataMap.value(userData), errorMsg.toStdString());
                }
            } else {
                if (userStateMap.value(userData).successCount == 0) {
                    spdlog::info("点位 ({}) 读取正常", userDataMap.value(userData));
                    ++userStateMap[userData].successCount;
                    userStateMap[userData].faultCount = 0;
                }
                const auto remote = data.value(0) ? PumpWidget::RemoteState::Remote : PumpWidget::RemoteState::Local;
                m_pump->setRemoteState(remote);

                QJsonObject json;
                json["name"] = "PumpRemoteState";
                json["value"] = remote; // 假设每个点位读1个寄存器
                m_webSocketClient->sendJson(json);
            }
            break;
        case 2:
            if (!success) {
                if (userStateMap.value(userData).faultCount == 0) {
                    m_pump->setRunningState(PumpWidget::RunningState::Fault);
                    ++userStateMap[userData].faultCount;
                    userStateMap[userData].successCount = 0;
                    spdlog::warn("点位 ({}) 读取错误: {}", userDataMap.value(userData), errorMsg.toStdString());
                }
            } else {
                if (userStateMap.value(userData).successCount == 0) {
                    spdlog::info("点位 ({}) 读取正常", userDataMap.value(userData));
                    ++userStateMap[userData].successCount;
                    userStateMap[userData].faultCount = 0;
                }
                const auto runningState =
                    data.value(0) ? PumpWidget::RunningState::Running : PumpWidget::RunningState::Stopped;
                m_pump->setRunningState(runningState);

                QJsonObject json;
                json["name"] = "PumpRunningState";
                json["value"] = runningState; // 假设每个点位读1个寄存器
                m_webSocketClient->sendJson(json);
            }
            break;
        case 3:
            if (!success) {
                if (userStateMap.value(userData).faultCount == 0) {
                    m_tank->setConnectedState(false);
                    ++userStateMap[userData].faultCount;
                    userStateMap[userData].successCount = 0;
                    spdlog::warn("点位 ({}) 读取错误: {}", userDataMap.value(userData), errorMsg.toStdString());
                }
            } else {
                if (userStateMap.value(userData).successCount == 0) {
                    spdlog::info("点位 ({}) 读取正常", userDataMap.value(userData));
                    ++userStateMap[userData].successCount;
                    userStateMap[userData].faultCount = 0;
                }
                m_tank->setConnectedState(true);
                const auto value = std::clamp(static_cast<int>(data.value(0)), m_aiScaleRange.min, m_aiScaleRange.max);
                const double per = (value - static_cast<double>(m_aiScaleRange.min)) /
                                   static_cast<double>(m_aiScaleRange.max - m_aiScaleRange.min) * 100;
                m_tank->setPercentage(per);

                QJsonObject json;
                json["name"] = "PumpRunningState";
                json["value"] = per; // 假设每个点位读1个寄存器
                m_webSocketClient->sendJson(json);
            }
            break;
        case 10:
            if (!success) {
                spdlog::warn("点位 ({}) 写入错误: {}", userDataMap.value(userData), errorMsg.toStdString());
            }
            break;
        default:
            break;
    }
}
void MainWindow::setPropertiesEnabled(const QString &propertyGroupName, const bool enabled) const {
    for (const auto &property : m_configModel->properties()) {
        if (!property->parentProperty() && property->propertyType() == QtVariantPropertyManager::groupTypeId() &&
            property->propertyName() == propertyGroupName) {
            property->setEnabled(enabled);
        }
    }
}
void MainWindow::handleWebSocketMessage(const QJsonObject &json) {
    QString action = json["action"].toString();
    if (action == "write") {
        int addr = json["addr"].toInt();
        int value = json["value"].toInt();
        int serverId = json.value("serverId").toInt(1);
        // 根据功能码选择写线圈或寄存器
        // 这里默认写保持寄存器，可以根据需求扩展
        m_robustModbusClient->writeHoldingRegister(addr, static_cast<quint16>(value), serverId, 10);
        spdlog::info("WebSocket 写指令: addr={}, value={}", addr, value);
    } else {
        qWarning() << "Unknown WebSocket action:" << action;
    }
}

void MainWindow::setReadTasks() const {

    static QMap<int, QModbusDataUnit::RegisterType> registerTypeMap{};
    registerTypeMap.insert(0, QModbusDataUnit::Coils);
    registerTypeMap.insert(1, QModbusDataUnit::DiscreteInputs);
    registerTypeMap.insert(2, QModbusDataUnit::HoldingRegisters);
    registerTypeMap.insert(3, QModbusDataUnit::InputRegisters);

    static QMap<QModbusDataUnit::RegisterType, std::string> registerNameMap{};
    registerNameMap.insert(QModbusDataUnit::Coils, "线圈");
    registerNameMap.insert(QModbusDataUnit::DiscreteInputs, "离散输入");
    registerNameMap.insert(QModbusDataUnit::HoldingRegisters, "保存寄存器");
    registerNameMap.insert(QModbusDataUnit::InputRegisters, "输入寄存器");

    const int serverId = m_configSource->property("plcSlaveId").toInt();

    const int deviceRemoteModeFuncCode = m_configSource->property("deviceRemoteModeFuncCode").toInt();
    const int deviceRemoteModeAddress = m_configSource->property("deviceRemoteModeAddress").toInt();
    m_robustModbusClient->updateReadTask(registerTypeMap.value(deviceRemoteModeFuncCode), serverId,
                                         deviceRemoteModeAddress, 1, 1);

    const int devicePumpStatusFuncCode = m_configSource->property("devicePumpStatusFuncCode").toInt();
    const int devicePumpStatusAddress = m_configSource->property("devicePumpStatusAddress").toInt();
    m_robustModbusClient->updateReadTask(registerTypeMap.value(devicePumpStatusFuncCode), serverId,
                                         devicePumpStatusAddress, 1, 2);

    const int deviceBarrelLevelFuncCode = m_configSource->property("deviceBarrelLevelFuncCode").toInt();
    const int deviceBarrelLevelAddress = m_configSource->property("deviceBarrelLevelAddress").toInt();
    m_robustModbusClient->updateReadTask(registerTypeMap.value(deviceBarrelLevelFuncCode), serverId,
                                         deviceBarrelLevelAddress, 1, 3);
    spdlog::info("");
    spdlog::info("设置轮询点位");
    spdlog::info("点位名称: 本地/远程状态, 从站地址: {}, 类型: {}, 起始地址: {}, 读取数量: {}",
                 QString::number(serverId).toStdString(),
                 registerNameMap.value(registerTypeMap.value(deviceRemoteModeFuncCode)),
                 QString::number(deviceRemoteModeAddress).toStdString(), "1");
    spdlog::info("点位名称: 泵运行状态, 从站地址: {}, 类型: {}, 起始地址: {}, 读取数量: {}",
                 QString::number(serverId).toStdString(),
                 registerNameMap.value(registerTypeMap.value(devicePumpStatusFuncCode)),
                 QString::number(devicePumpStatusAddress).toStdString(), "1");
    spdlog::info("点位名称: 桶液位,从站地址: {}, 类型: {}, 起始地址: {}, 读取数量: {}",
                 QString::number(serverId).toStdString(),
                 registerNameMap.value(registerTypeMap.value(deviceBarrelLevelFuncCode)),
                 QString::number(deviceBarrelLevelAddress).toStdString(), "1");
    spdlog::info("-------------------------------------------------------------------------------------");
    spdlog::info("");
}