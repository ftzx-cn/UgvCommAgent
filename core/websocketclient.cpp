#include "websocketclient.h"

#include "spdlog/spdlog.h"

#include <QDebug>
#include <QJsonDocument>

WebSocketClient::WebSocketClient(QObject *parent) : QObject(parent) {
    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::errorOccurred,
            [this](QAbstractSocket::SocketError error) { emit errorOccurred(m_webSocket->errorString()); });

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::onReconnectTimeout);
}

WebSocketClient::~WebSocketClient() { disconnectFromServer(); }

void WebSocketClient::connectToServer(const QString &url) {
    m_serverUrl = url;
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
    m_manualDisconnect = false;
    m_reconnectTimer->stop();
    m_webSocket->open(QUrl(url));
}

void WebSocketClient::disconnectFromServer() {
    m_manualDisconnect = true;
    m_reconnectTimer->stop();
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
}

void WebSocketClient::sendJson(const QJsonObject &json) const {
    if (!isConnected())
        return;
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    m_webSocket->sendTextMessage(QString::fromUtf8(data));
}

bool WebSocketClient::isConnected() const { return m_webSocket->state() == QAbstractSocket::ConnectedState; }

// ---------- slots ----------
void WebSocketClient::onConnected() {
    m_manualDisconnect = false;
    m_reconnectTimer->stop();
    emit connected();
    qDebug() << "WebSocket connected to" << m_serverUrl;
}

void WebSocketClient::onDisconnected() {
    emit disconnected();
    if (!m_manualDisconnect) {
        // 意外断开：启动重连
        spdlog::warn("WebSocket disconnected, will reconnect in " + std::to_string(m_reconnectIntervalMs)+ " ms");
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(m_reconnectIntervalMs);
        }
    } else {
        // 主动断开：不重连，并复位标志
        qDebug() << "WebSocket manually disconnected, no reconnect";
        m_manualDisconnect = false;
    }
}

void WebSocketClient::onTextMessageReceived(const QString &message) {
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON received:" << message;
        return;
    }
    emit textMessageReceived(doc.object());
}

void WebSocketClient::onReconnectTimeout() const {
    if (m_serverUrl.isEmpty())
        return;
    spdlog::info("重连WS服务器 - {}", m_serverUrl.toStdString());
    m_webSocket->open(QUrl(m_serverUrl));
}