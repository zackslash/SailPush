#include <QtTest>
#include "websocketmanager.h"

class TestWebSocketManager : public QObject {
    Q_OBJECT

private slots:
    void testParseKeepAlive();
    void testParseNewMessage();
    void testParseReload();
    void testParseError();
    void testParseSessionClosed();
    void testParseUnknown();
    void testParseEmpty();
    void testReconnectBackoff();
    void testInitialState();
    void testConstants();

private:
    WebSocketManager *m_manager;
};

void TestWebSocketManager::testParseKeepAlive()
{
    m_manager = new WebSocketManager(this);

    QVariant result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QByteArray, QByteArray("#")));

    QCOMPARE(result.toInt(), static_cast<int>(WebSocketManager::FrameType::KeepAlive));
    delete m_manager;
}

void TestWebSocketManager::testParseNewMessage()
{
    m_manager = new WebSocketManager(this);

    QVariant result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QByteArray, QByteArray("!")));

    QCOMPARE(result.toInt(), static_cast<int>(WebSocketManager::FrameType::NewMessage));
    delete m_manager;
}

void TestWebSocketManager::testParseReload()
{
    m_manager = new WebSocketManager(this);

    QVariant result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QByteArray, QByteArray("R")));

    QCOMPARE(result.toInt(), static_cast<int>(WebSocketManager::FrameType::Reload));
    delete m_manager;
}

void TestWebSocketManager::testParseError()
{
    m_manager = new WebSocketManager(this);

    QVariant result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QByteArray, QByteArray("E")));

    QCOMPARE(result.toInt(), static_cast<int>(WebSocketManager::FrameType::Error));
    delete m_manager;
}

void TestWebSocketManager::testParseSessionClosed()
{
    m_manager = new WebSocketManager(this);

    QVariant result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QByteArray, QByteArray("A")));

    QCOMPARE(result.toInt(), static_cast<int>(WebSocketManager::FrameType::SessionClosedByServer));
    delete m_manager;
}

void TestWebSocketManager::testParseUnknown()
{
    m_manager = new WebSocketManager(this);

    QVariant result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QByteArray, QByteArray("X")));

    QCOMPARE(result.toInt(), static_cast<int>(WebSocketManager::FrameType::Unknown));
    delete m_manager;
}

void TestWebSocketManager::testParseEmpty()
{
    m_manager = new WebSocketManager(this);

    QVariant result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QByteArray, QByteArray()));

    QCOMPARE(result.toInt(), static_cast<int>(WebSocketManager::FrameType::Unknown));
    delete m_manager;
}

void TestWebSocketManager::testReconnectBackoff()
{
    m_manager = new WebSocketManager(this);

    QCOMPARE(WebSocketManager::RECONNECT_INITIAL_MS, 1000);
    QCOMPARE(WebSocketManager::RECONNECT_MAX_MS, 300000);

    QVERIFY(WebSocketManager::RECONNECT_INITIAL_MS < WebSocketManager::RECONNECT_MAX_MS);
    delete m_manager;
}

void TestWebSocketManager::testInitialState()
{
    m_manager = new WebSocketManager(this);
    QCOMPARE(m_manager->state(), WebSocketManager::ConnectionState::Disconnected);
    QCOMPARE(m_manager->isConnected(), false);
    delete m_manager;
}

void TestWebSocketManager::testConstants()
{
    QCOMPARE(QString(WebSocketManager::WS_URL), QString("wss://client.pushover.net/push"));
    QCOMPARE(WebSocketManager::RECONNECT_INITIAL_MS, 1000);
    QCOMPARE(WebSocketManager::RECONNECT_MAX_MS, 300000);
}

// tst_websocketmanager.cpp - TestWebSocketManager class implementation
#include "tst_websocketmanager.moc"
