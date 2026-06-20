#ifndef WEBSOCKETMANAGER_H
#define WEBSOCKETMANAGER_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcWebSocket)

class WebSocketManager : public QObject {
    Q_OBJECT

public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        LoginSent,
        Error,
        SessionClosed
    };
    Q_ENUM(ConnectionState)

    enum class FrameType {
        KeepAlive,
        NewMessage,
        Reload,
        Error,
        SessionClosedByServer,
        Unknown
    };
    Q_ENUM(FrameType)

    static constexpr int RECONNECT_INITIAL_MS = 1000;
    static constexpr int RECONNECT_MAX_MS = 300000;
    static constexpr const char *WS_URL = "wss://client.pushover.net/push";

    explicit WebSocketManager(QObject *parent = nullptr);

    void connectToServer(const QString &deviceId, const QString &secret);
    void disconnectFromServer();
    void reconnect();

    ConnectionState state() const { return m_state; }
    bool isConnected() const { return m_state == ConnectionState::LoginSent; }

signals:
    void stateChanged(ConnectionState state);
    void newMessageAvailable();
    void connectionError(const QString &error);
    void sessionClosedByServer();
    void reloadRequested();

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    void onError(QAbstractSocket::SocketError error);
    void onReconnectTimer();

private:
    void sendLoginFrame();
    void scheduleReconnect();
    void setState(ConnectionState newState);
    Q_INVOKABLE int parseFrame(const QByteArray &data);
    void handleFrame(FrameType type);

    QWebSocket *m_webSocket;
    QTimer *m_reconnectTimer;
    QString m_deviceId;
    QString m_secret;
    ConnectionState m_state;
    int m_reconnectDelay;
    bool m_autoReconnect;
};

#endif // WEBSOCKETMANAGER_H
