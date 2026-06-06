#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "dbusinterface.h"
#include "messagestore.h"
#include "websocketmanager.h"

class TestDbusInterface : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testGetMessagesEmpty();
    void testGetMessagesWithData();
    void testGetMessageCount();
    void testGetUnreadCount();
    void testGetConnectionState();
    void testGetSettings();
    void testMarkAsRead();
    void testMarkAllAsRead();
    void testMessageToMapFields();
    void testGetVersion();

private:
    QTemporaryDir m_tempDir;
    MessageStore *m_store;
    WebSocketManager *m_ws;
    DbusInterface *m_dbus;
};

void TestDbusInterface::init()
{
    m_store = new MessageStore(m_tempDir.path(), this);
    m_ws = new WebSocketManager(this);
    m_dbus = new DbusInterface(m_store, m_ws, m_tempDir.path(), this);
    // Note: NOT calling registerService() — we test logic only, not D-Bus registration
}

void TestDbusInterface::cleanup()
{
    delete m_dbus;
    m_dbus = nullptr;
    delete m_ws;
    m_ws = nullptr;
    delete m_store;
    m_store = nullptr;
}

void TestDbusInterface::testGetMessagesEmpty()
{
    QVariantList msgs = m_dbus->GetMessages();
    QCOMPARE(msgs.size(), 0);
}

void TestDbusInterface::testGetMessagesWithData()
{
    Message msg;
    msg.id = "123";
    msg.title = "Test Title";
    msg.message = "Test Body";
    msg.app = "TestApp";
    msg.priority = 1;
    msg.date = 1700000000;
    m_store->addMessage(msg);

    QVariantList msgs = m_dbus->GetMessages();
    QCOMPARE(msgs.size(), 1);

    QVariantMap map = msgs.first().toMap();
    QCOMPARE(map["id"].toString(), QString("123"));
    QCOMPARE(map["title"].toString(), QString("Test Title"));
    QCOMPARE(map["message"].toString(), QString("Test Body"));
    QCOMPARE(map["app"].toString(), QString("TestApp"));
    QCOMPARE(map["priority"].toInt(), 1);
    QCOMPARE(map["date"].toLongLong(), 1700000000LL);
}

void TestDbusInterface::testGetMessageCount()
{
    QCOMPARE(m_dbus->GetMessageCount(), 0);

    Message msg;
    msg.id = "1";
    m_store->addMessage(msg);
    QCOMPARE(m_dbus->GetMessageCount(), 1);

    Message msg2;
    msg2.id = "2";
    m_store->addMessage(msg2);
    QCOMPARE(m_dbus->GetMessageCount(), 2);
}

void TestDbusInterface::testGetUnreadCount()
{
    QCOMPARE(m_dbus->GetUnreadCount(), 0);

    Message msg;
    msg.id = "1";
    msg.read = false;
    m_store->addMessage(msg);
    QCOMPARE(m_dbus->GetUnreadCount(), 1);

    m_store->markAsRead("1");
    QCOMPARE(m_dbus->GetUnreadCount(), 0);
}

void TestDbusInterface::testGetConnectionState()
{
    // WebSocketManager starts disconnected
    QString state = m_dbus->GetConnectionState();
    QCOMPARE(state, QString("disconnected"));
}

void TestDbusInterface::testGetSettings()
{
    QVariantMap settings = m_dbus->GetSettings();
    QVERIFY(settings.contains("unreadCount"));
    QVERIFY(settings.contains("connectionState"));
    QVERIFY(settings.contains("messageCount"));
    QCOMPARE(settings["unreadCount"].toInt(), 0);
    QCOMPARE(settings["connectionState"].toString(), QString("disconnected"));
    QCOMPARE(settings["messageCount"].toInt(), 0);
}

void TestDbusInterface::testMarkAsRead()
{
    Message msg;
    msg.id = "1";
    msg.read = false;
    m_store->addMessage(msg);
    QCOMPARE(m_dbus->GetUnreadCount(), 1);

    QSignalSpy spy(m_dbus, &DbusInterface::unreadCountChanged);
    m_dbus->MarkAsRead("1");
    QCOMPARE(m_dbus->GetUnreadCount(), 0);
    QCOMPARE(spy.count(), 1);
}

void TestDbusInterface::testMarkAllAsRead()
{
    for (int i = 0; i < 3; ++i) {
        Message msg;
        msg.id = QString::number(i);
        msg.read = false;
        m_store->addMessage(msg);
    }
    QCOMPARE(m_dbus->GetUnreadCount(), 3);

    QSignalSpy spy(m_dbus, &DbusInterface::unreadCountChanged);
    m_dbus->MarkAllAsRead();
    QCOMPARE(m_dbus->GetUnreadCount(), 0);
    QCOMPARE(spy.count(), 1);
}

void TestDbusInterface::testMessageToMapFields()
{
    Message msg;
    msg.id = "42";
    msg.umid = "99";
    msg.title = "Title";
    msg.message = "Body";
    msg.app = "App";
    msg.aid = "1";
    msg.icon = "icon";
    msg.date = 1700000000;
    msg.priority = 2;
    msg.sound = "bike";
    msg.url = "https://example.com";
    msg.urlTitle = "Example";
    msg.acked = true;
    msg.receipt = "receipt";
    msg.html = true;
    msg.read = false;
    msg.localTimestamp = 1700000050;
    m_store->addMessage(msg);

    QVariantList msgs = m_dbus->GetMessages();
    QVariantMap map = msgs.first().toMap();

    QCOMPARE(map["id"].toString(), QString("42"));
    QCOMPARE(map["umid"].toString(), QString("99"));
    QCOMPARE(map["title"].toString(), QString("Title"));
    QCOMPARE(map["message"].toString(), QString("Body"));
    QCOMPARE(map["app"].toString(), QString("App"));
    QCOMPARE(map["aid"].toString(), QString("1"));
    QCOMPARE(map["icon"].toString(), QString("icon"));
    QCOMPARE(map["date"].toLongLong(), 1700000000LL);
    QCOMPARE(map["priority"].toInt(), 2);
    QCOMPARE(map["sound"].toString(), QString("bike"));
    QCOMPARE(map["url"].toString(), QString("https://example.com"));
    QCOMPARE(map["url_title"].toString(), QString("Example"));
    QCOMPARE(map["acked"].toBool(), true);
    QCOMPARE(map["receipt"].toString(), QString("receipt"));
    QCOMPARE(map["html"].toBool(), true);
    QCOMPARE(map["read"].toBool(), false);
    QCOMPARE(map["local_timestamp"].toLongLong(), 1700000050LL);
    QCOMPARE(map["is_emergency"].toBool(), true);
}

void TestDbusInterface::testGetVersion()
{
    QString version = m_dbus->GetVersion();
    // GIT_VERSION is always defined (falls back to "dev")
    QVERIFY(!version.isEmpty());
}

QTEST_MAIN(TestDbusInterface)
#include "tst_dbusinterface.moc"
