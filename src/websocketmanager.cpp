#include "websocketmanager.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcWebSocket, "net.sailpush.sailfish.websocket")

WebSocketManager::WebSocketManager(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_reconnectTimer(new QTimer(this))
    , m_state(ConnectionState::Disconnected)
    , m_reconnectDelay(RECONNECT_INITIAL_MS)
    , m_autoReconnect(true)
{
    m_reconnectTimer->setSingleShot(true);

    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketManager::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketManager::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketManager::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketManager::onBinaryMessageReceived);
    connect(m_webSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketManager::onReconnectTimer);
}

void WebSocketManager::connectToServer(const QString &deviceId, const QString &secret)
{
    if (m_state == ConnectionState::LoginSent || m_state == ConnectionState::Connecting) {
        qCInfo(lcWebSocket) << "Already connected, ignoring";
        return;
    }

    m_deviceId = deviceId;
    m_secret = secret;
    m_autoReconnect = true;
    m_reconnectDelay = RECONNECT_INITIAL_MS;

    qCInfo(lcWebSocket) << "Connecting to" << WS_URL;
    m_reconnectTimer->stop();
    m_webSocket->close();
    setState(ConnectionState::Connecting);
    m_webSocket->open(QUrl(WS_URL));
}

void WebSocketManager::disconnectFromServer()
{
    m_autoReconnect = false;
    m_reconnectTimer->stop();
    m_webSocket->close();
    setState(ConnectionState::Disconnected);
}

void WebSocketManager::reconnect()
{
    m_reconnectDelay = RECONNECT_INITIAL_MS;
    connectToServer(m_deviceId, m_secret);
}

void WebSocketManager::onConnected()
{
    qCInfo(lcWebSocket) << "WebSocket connected";
    m_reconnectTimer->stop();
    setState(ConnectionState::Connected);
    sendLoginFrame();
}

void WebSocketManager::onDisconnected()
{
    qCInfo(lcWebSocket) << "WebSocket disconnected";
    setState(ConnectionState::Disconnected);

    if (m_autoReconnect) {
        scheduleReconnect();
    }
}

void WebSocketManager::onTextMessageReceived(const QString &message)
{
    FrameType frame = static_cast<FrameType>(parseFrame(message.toUtf8()).toInt());

    switch (frame) {
    case FrameType::NewMessage:
        qCInfo(lcWebSocket) << "New message available";
        emit newMessageAvailable();
        break;
    case FrameType::Reload:
        qCInfo(lcWebSocket) << "Reload requested";
        emit reloadRequested();
        break;
    case FrameType::Error:
        qCWarning(lcWebSocket) << "Server error received";
        m_autoReconnect = false;
        emit connectionError(QStringLiteral("Permanent server error. Please re-login."));
        break;
    case FrameType::SessionClosedByServer:
        qCWarning(lcWebSocket) << "Session closed by server (logged in elsewhere)";
        m_autoReconnect = false;
        emit sessionClosedByServer();
        break;
    case FrameType::KeepAlive:
        break;
    case FrameType::Unknown:
        qCDebug(lcWebSocket) << "Unknown frame received:" << message;
        break;
    }
}

void WebSocketManager::onBinaryMessageReceived(const QByteArray &message)
{
    FrameType frame = static_cast<FrameType>(parseFrame(message).toInt());

    switch (frame) {
    case FrameType::NewMessage:
        emit newMessageAvailable();
        break;
    case FrameType::Reload:
        emit reloadRequested();
        break;
    case FrameType::Error:
        m_autoReconnect = false;
        emit connectionError(QStringLiteral("Permanent server error."));
        break;
    case FrameType::SessionClosedByServer:
        m_autoReconnect = false;
        emit sessionClosedByServer();
        break;
    default:
        break;
    }
}

void WebSocketManager::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qCWarning(lcWebSocket) << "WebSocket error:" << m_webSocket->errorString();
    setState(ConnectionState::Error);
}

void WebSocketManager::onReconnectTimer()
{
    if (m_state != ConnectionState::Disconnected) {
        return;
    }
    qCInfo(lcWebSocket) << "Reconnecting...";
    m_webSocket->close();
    m_webSocket->open(QUrl(WS_URL));
    setState(ConnectionState::Connecting);
}

void WebSocketManager::sendLoginFrame()
{
    QString loginFrame = QString("login:%1:%2\n").arg(m_deviceId, m_secret);
    m_webSocket->sendTextMessage(loginFrame);
    qCInfo(lcWebSocket) << "Login frame sent";
    setState(ConnectionState::LoginSent);
    m_reconnectDelay = RECONNECT_INITIAL_MS;
}

void WebSocketManager::scheduleReconnect()
{
    qCInfo(lcWebSocket) << "Scheduling reconnect in" << m_reconnectDelay << "ms";
    m_reconnectTimer->start(m_reconnectDelay);
    m_reconnectDelay = qMin(m_reconnectDelay * 2, RECONNECT_MAX_MS);
}

void WebSocketManager::setState(ConnectionState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

QVariant WebSocketManager::parseFrame(const QByteArray &data)
{
    if (data.isEmpty()) return static_cast<int>(FrameType::Unknown);

    char frame = data.at(0);
    switch (frame) {
    case '#': return static_cast<int>(FrameType::KeepAlive);
    case '!': return static_cast<int>(FrameType::NewMessage);
    case 'R': return static_cast<int>(FrameType::Reload);
    case 'E': return static_cast<int>(FrameType::Error);
    case 'A': return static_cast<int>(FrameType::SessionClosedByServer);
    default:  return static_cast<int>(FrameType::Unknown);
    }
}
