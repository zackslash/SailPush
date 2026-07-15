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
    void testIdleTimeoutConstant();
    void testResetCredentialsKeepsDisconnected();
    void testIdleTimeoutClosesSocket();

private:
    WebSocketManager *m_manager;
};

void TestWebSocketManager::testParseKeepAlive()
{
    m_manager = new WebSocketManager(this);

    int result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(int, result),
        Q_ARG(QByteArray, QByteArray("#")));

    QCOMPARE(result, static_cast<int>(WebSocketManager::FrameType::KeepAlive));
    delete m_manager;
}

void TestWebSocketManager::testParseNewMessage()
{
    m_manager = new WebSocketManager(this);

    int result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(int, result),
        Q_ARG(QByteArray, QByteArray("!")));

    QCOMPARE(result, static_cast<int>(WebSocketManager::FrameType::NewMessage));
    delete m_manager;
}

void TestWebSocketManager::testParseReload()
{
    m_manager = new WebSocketManager(this);

    int result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(int, result),
        Q_ARG(QByteArray, QByteArray("R")));

    QCOMPARE(result, static_cast<int>(WebSocketManager::FrameType::Reload));
    delete m_manager;
}

void TestWebSocketManager::testParseError()
{
    m_manager = new WebSocketManager(this);

    int result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(int, result),
        Q_ARG(QByteArray, QByteArray("E")));

    QCOMPARE(result, static_cast<int>(WebSocketManager::FrameType::Error));
    delete m_manager;
}

void TestWebSocketManager::testParseSessionClosed()
{
    m_manager = new WebSocketManager(this);

    int result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(int, result),
        Q_ARG(QByteArray, QByteArray("A")));

    QCOMPARE(result, static_cast<int>(WebSocketManager::FrameType::SessionClosedByServer));
    delete m_manager;
}

void TestWebSocketManager::testParseUnknown()
{
    m_manager = new WebSocketManager(this);

    int result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(int, result),
        Q_ARG(QByteArray, QByteArray("X")));

    QCOMPARE(result, static_cast<int>(WebSocketManager::FrameType::Unknown));
    delete m_manager;
}

void TestWebSocketManager::testParseEmpty()
{
    m_manager = new WebSocketManager(this);

    int result;
    QMetaObject::invokeMethod(m_manager, "parseFrame",
        Q_RETURN_ARG(int, result),
        Q_ARG(QByteArray, QByteArray()));

    QCOMPARE(result, static_cast<int>(WebSocketManager::FrameType::Unknown));
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

void TestWebSocketManager::testIdleTimeoutConstant()
{
    // Watchdog must exceed Pushover's ~60s keepalive interval with margin.
    QVERIFY(WebSocketManager::IDLE_TIMEOUT_MS >= 60000);
    QVERIFY(WebSocketManager::IDLE_TIMEOUT_MS <= WebSocketManager::RECONNECT_MAX_MS);
}

void TestWebSocketManager::testResetCredentialsKeepsDisconnected()
{
    // resetCredentials() must not flip state or mark us connected. It clears
    // cached creds for the logout path; reconnect-with-creds behavior itself
    // needs a live socket and is covered by integration/on-device testing.
    m_manager = new WebSocketManager(this);
    QCOMPARE(m_manager->state(), WebSocketManager::ConnectionState::Disconnected);
    m_manager->resetCredentials();
    QCOMPARE(m_manager->state(), WebSocketManager::ConnectionState::Disconnected);
    QCOMPARE(m_manager->isConnected(), false);
    delete m_manager;
}

void TestWebSocketManager::testIdleTimeoutClosesSocket()
{
    // No live WS server in this unit test, so this only smoke-checks that onIdleTimeout
    // leaves state consistent (no real socket means no disconnected signal fires).
    m_manager = new WebSocketManager(this);
    QCOMPARE(m_manager->state(), WebSocketManager::ConnectionState::Disconnected);
    QMetaObject::invokeMethod(m_manager, "onIdleTimeout");
    QCOMPARE(m_manager->state(), WebSocketManager::ConnectionState::Disconnected);
    QCOMPARE(m_manager->isConnected(), false);
    delete m_manager;
}

QTEST_MAIN(TestWebSocketManager)
#include "tst_websocketmanager.moc"
