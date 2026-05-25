#include <QtTest>
#include "message.h"

class TestMessage : public QObject {
    Q_OBJECT

private slots:
    void testDefaultConstructor();
    void testFromJson();
    void testFromJsonWithStrFallback();
    void testToJson();
    void testRoundTrip();
    void testPriorityHelpers();
    void testDisplayName();
};

void TestMessage::testDefaultConstructor()
{
    Message msg;
    QCOMPARE(msg.id, QString());
    QCOMPARE(msg.title, QString());
    QCOMPARE(msg.message, QString());
    QCOMPARE(msg.priority, 0);
    QCOMPARE(msg.acked, false);
    QCOMPARE(msg.html, false);
    QCOMPARE(msg.read, false);
    QCOMPARE(msg.date, 0LL);
}

void TestMessage::testFromJson()
{
    QJsonObject obj;
    obj.insert("id_str", "123456");
    obj.insert("umid_str", "789012");
    obj.insert("title", "Test Title");
    obj.insert("message", "Test Message Body");
    obj.insert("app", "TestApp");
    obj.insert("aid_str", "111");
    obj.insert("icon", "test_icon");
    obj.insert("date", 1700000000);
    obj.insert("priority", 1);
    obj.insert("sound", "bike");
    obj.insert("url", "https://example.com");
    obj.insert("url_title", "Example");
    obj.insert("acked", 1);
    obj.insert("receipt", "receipt_abc");
    obj.insert("html", 0);
    obj.insert("read", true);
    obj.insert("local_timestamp", 1700000100);

    Message msg = Message::fromJson(obj);

    QCOMPARE(msg.id, QString("123456"));
    QCOMPARE(msg.umid, QString("789012"));
    QCOMPARE(msg.title, QString("Test Title"));
    QCOMPARE(msg.message, QString("Test Message Body"));
    QCOMPARE(msg.app, QString("TestApp"));
    QCOMPARE(msg.aid, QString("111"));
    QCOMPARE(msg.icon, QString("test_icon"));
    QCOMPARE(msg.date, 1700000000LL);
    QCOMPARE(msg.priority, 1);
    QCOMPARE(msg.sound, QString("bike"));
    QCOMPARE(msg.url, QString("https://example.com"));
    QCOMPARE(msg.urlTitle, QString("Example"));
    QCOMPARE(msg.acked, true);
    QCOMPARE(msg.receipt, QString("receipt_abc"));
    QCOMPARE(msg.html, false);
    QCOMPARE(msg.read, true);
    QCOMPARE(msg.localTimestamp, 1700000100LL);
}

void TestMessage::testFromJsonWithStrFallback()
{
    QJsonObject obj;
    obj.insert("id", 123456);
    obj.insert("umid", 789012);
    obj.insert("title", "Title");
    obj.insert("message", "Body");

    Message msg = Message::fromJson(obj);
    QCOMPARE(msg.id, QString("123456"));
    QCOMPARE(msg.umid, QString("789012"));
}

void TestMessage::testToJson()
{
    Message msg;
    msg.id = "123";
    msg.title = "Title";
    msg.message = "Body";
    msg.app = "App";
    msg.priority = 2;
    msg.acked = true;
    msg.date = 1700000000;

    QJsonObject obj = msg.toJson();

    QCOMPARE(obj.value("id").toString(), QString("123"));
    QCOMPARE(obj.value("title").toString(), QString("Title"));
    QCOMPARE(obj.value("message").toString(), QString("Body"));
    QCOMPARE(obj.value("app").toString(), QString("App"));
    QCOMPARE(obj.value("priority").toInt(), 2);
    QCOMPARE(obj.value("acked").toInt(), 1);
    QCOMPARE(obj.value("date").toVariant().toLongLong(), 1700000000LL);
}

void TestMessage::testRoundTrip()
{
    Message original;
    original.id = "999";
    original.umid = "888";
    original.title = "Round Trip";
    original.message = "Test body";
    original.app = "TestApp";
    original.priority = -1;
    original.sound = "none";
    original.acked = false;
    original.html = true;
    original.date = 1700000000;
    original.localTimestamp = 1700000050;

    Message restored = Message::fromJson(original.toJson());

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.umid, original.umid);
    QCOMPARE(restored.title, original.title);
    QCOMPARE(restored.message, original.message);
    QCOMPARE(restored.app, original.app);
    QCOMPARE(restored.priority, original.priority);
    QCOMPARE(restored.sound, original.sound);
    QCOMPARE(restored.acked, original.acked);
    QCOMPARE(restored.html, original.html);
    QCOMPARE(restored.date, original.date);
}

void TestMessage::testPriorityHelpers()
{
    Message msg;

    msg.priority = -2;
    QVERIFY(msg.isSilent());
    QVERIFY(msg.isLowPriority());
    QVERIFY(!msg.isHighPriority());
    QVERIFY(!msg.isEmergency());

    msg.priority = -1;
    QVERIFY(!msg.isSilent());
    QVERIFY(msg.isLowPriority());
    QVERIFY(!msg.isHighPriority());

    msg.priority = 0;
    QVERIFY(!msg.isSilent());
    QVERIFY(!msg.isLowPriority());
    QVERIFY(!msg.isHighPriority());

    msg.priority = 1;
    QVERIFY(!msg.isSilent());
    QVERIFY(!msg.isLowPriority());
    QVERIFY(msg.isHighPriority());
    QVERIFY(!msg.isEmergency());

    msg.priority = 2;
    QVERIFY(msg.isEmergency());
    QVERIFY(msg.isHighPriority());
}

void TestMessage::testDisplayName()
{
    Message msg;

    msg.title = "My Title";
    msg.app = "MyApp";
    QCOMPARE(msg.displayName(), QString("My Title"));

    msg.title = "";
    QCOMPARE(msg.displayName(), QString("MyApp"));

    msg.title = "";
    msg.app = "";
    QCOMPARE(msg.displayName(), QString(""));
}

// tst_message.cpp - TestMessage class implementation
#include "tst_message.moc"
