#include <QtTest>
#include "sailpushclient.h"

class TestSailPushClient : public QObject {
    Q_OBJECT

private slots:
    void testLoginUrl();
    void testRegisterUrl();
    void testMessagesUrl();
    void testDeleteUrl();
    void testAcknowledgeUrl();
    void testUrlFormat();

};

void TestSailPushClient::testLoginUrl()
{
    QString url = SailPushClient::loginUrl();
    QVERIFY(url.contains("users/login.json"));
    QVERIFY(url.startsWith("https://api.pushover.net/1"));
}

void TestSailPushClient::testRegisterUrl()
{
    QString url = SailPushClient::registerUrl();
    QVERIFY(url.contains("devices.json"));
    QVERIFY(url.startsWith("https://api.pushover.net/1"));
}

void TestSailPushClient::testMessagesUrl()
{
    QString url = SailPushClient::messagesUrl("secret123", "device456");
    QVERIFY(url.contains("messages.json"));
    QVERIFY(url.contains("secret=secret123"));
    QVERIFY(url.contains("device_id=device456"));
    QVERIFY(url.startsWith("https://api.pushover.net/1"));
}

void TestSailPushClient::testDeleteUrl()
{
    QString url = SailPushClient::deleteUrl("device789");
    QVERIFY(url.contains("devices/device789/update_highest_message.json"));
    QVERIFY(url.startsWith("https://api.pushover.net/1"));
}

void TestSailPushClient::testAcknowledgeUrl()
{
    QString url = SailPushClient::acknowledgeUrl("receipt_abc");
    QVERIFY(url.contains("receipts/receipt_abc/acknowledge.json"));
    QVERIFY(url.startsWith("https://api.pushover.net/1"));
}

void TestSailPushClient::testUrlFormat()
{
    QString url = SailPushClient::messagesUrl("my_secret", "my_device");
    QVERIFY(url.contains("secret=my_secret"));
    QVERIFY(url.contains("device_id=my_device"));

    QString deleteUrl = SailPushClient::deleteUrl("my_device_id");
    QVERIFY(deleteUrl.contains("my_device_id"));

    QString ackUrl = SailPushClient::acknowledgeUrl("my_receipt");
    QVERIFY(ackUrl.contains("my_receipt"));
}

// tst_sailpushclient.cpp - TestSailPushClient class implementation
#include "tst_sailpushclient.moc"
