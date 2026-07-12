#pragma once

#ifndef UGVCOMMAGENT_MAINWINDOW_H
#define UGVCOMMAGENT_MAINWINDOW_H

#include "./core/configmodel.h"
#include "./core/robustmodbusclient.h"
#include "./core/websocketclient.h"
#include "./utils/PumpWidget.h"
#include "./utils/SwitchButtonWidget.h"
#include "./utils/TankWidget.h"
#include "./utils/cywiplineedit.h"

#include "./3rdparty/qtpropertybrowser/qteditorfactory_p.h"
#include "./3rdparty/qtpropertybrowser/qtpropertymanager_p.h"
#include "./3rdparty/qtpropertybrowser/qttreepropertybrowser_p.h"
#include "./3rdparty/qtpropertybrowser/qtvariantproperty_p.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/qt_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QHostAddress>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QModbusTcpClient>
#include <QPushButton>
#include <QScrollBar>
#include <QSize>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

  signals:

  private:
    void setReadTasks() const;
    void setPlcConnectionParameter() const;
    void handleModbusMessageReceived(quint64 requestId, const QModbusDataUnit &data, bool success,
                                     const QString &errorMsg, quint32 userData) const;
    void setPropertiesEnabled(const QString &propertyGroupName, bool enabled) const;

    void handleWebSocketMessage(const QJsonObject &json);

    struct AiScaleRange {
        int min;
        int max;
    };
    AiScaleRange m_aiScaleRange{4000, 20000};

    ConfigSource *m_configSource{nullptr};
    QtVariantPropertyManager *m_propertyManager{nullptr};
    QtVariantEditorFactory *m_editFactory{nullptr};
    QtTreePropertyBrowser *m_configBrowser{nullptr};
    ConfigModel *m_configModel{nullptr};

    QTimer *m_reconnectTimer{nullptr};
    QModbusClient *m_tcpClient{nullptr};
    RobustModbusClient *m_robustModbusClient{nullptr};

    QToolBar *m_toolBar{nullptr};
    QAction *m_connectPlcAction{nullptr};
    QAction *m_disconnectPlcAction{nullptr};

    QAction *m_connectWebSocketAction{nullptr};
    QAction *m_disconnectWebSocketAction{nullptr};
    QLabel *m_webSocketStatusIcon{nullptr};

    QAction *m_saveAction{nullptr};
    QLabel *m_plcStatusIcon{nullptr};

    TankWidget *m_tank{nullptr};
    PumpWidget *m_pump{nullptr};
    SwitchButtonWidget *m_switchMode{nullptr};
    QTextEdit *m_logView{nullptr};

    std::shared_ptr<spdlog::sinks::qt_color_sink_mt> m_logview_sink;

    bool m_isConnectedPlc{false};
    bool m_isConnectedWebSocket{false};

    WebSocketClient *m_webSocketClient{nullptr};
};

#endif // UGVCOMMAGENT_MAINWINDOW_H