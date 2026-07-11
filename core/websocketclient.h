#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QWebSocket>

class WebSocketClient : public QObject {
    Q_OBJECT
  public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    void connectToServer(const QString &url);
    void disconnectFromServer() const;
    void sendJson(const QJsonObject &json) const;
    [[nodiscard]] bool isConnected() const;

  signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QJsonObject &json);
    void errorOccurred(const QString &error);

  private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onReconnectTimeout() const;

  private:
    QWebSocket *m_webSocket;
    QTimer *m_reconnectTimer;
    QString m_serverUrl;
    int m_reconnectIntervalMs = 3000;
};

#endif // WEBSOCKETCLIENT_H