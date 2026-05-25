#include <QtTest>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "messagestore.h"

class TestMessageStore : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testEmptyStore();
    void testAddMessage();
    void testAddMessages();
    void testDuplicateMessage();
    void testMarkAsRead();
    void testMarkAllAsRead();
    void testRemoveMessage();
    void testUnreadCount();
    void testHighestMessageId();
    void testContainsMessage();
    void testTrimMessages();
    void testSaveAndLoad();

private:
    QTemporaryDir m_tempDir;
    MessageStore *m_store;
};

void TestMessageStore::init()
{
    m_store = new MessageStore(m_tempDir.path(), this);
}

void TestMessageStore::cleanup()
{
    delete m_store;
    m_store = nullptr;
}

void TestMessageStore::testEmptyStore()
{
    QCOMPARE(m_store->messages().size(), 0);
    QCOMPARE(m_store->unreadCount(), 0);
    QCOMPARE(m_store->totalCount(), 0);
    QCOMPARE(m_store->highestMessageId(), QString());
}

void TestMessageStore::testAddMessage()
{
    Message msg;
    msg.id = "1";
    msg.message = "Hello";
    msg.app = "TestApp";
    msg.priority = 0;

    m_store->addMessage(msg);

    QCOMPARE(m_store->messages().size(), 1);
    QCOMPARE(m_store->messages().first().id, QString("1"));
    QCOMPARE(m_store->unreadCount(), 1);
}

void TestMessageStore::testAddMessages()
{
    for (int i = 0; i < 5; ++i) {
        Message msg;
        msg.id = QString::number(i);
        msg.message = QString("Message %1").arg(i);
        m_store->addMessage(msg);
    }
    QCOMPARE(m_store->messages().size(), 5);
    QCOMPARE(m_store->totalCount(), 5);
}

void TestMessageStore::testDuplicateMessage()
{
    Message msg;
    msg.id = "1";
    msg.message = "First";

    m_store->addMessage(msg);
    QCOMPARE(m_store->messages().size(), 1);

    Message msg2;
    msg2.id = "1";
    msg2.message = "Duplicate";

    m_store->addMessage(msg2);
    QCOMPARE(m_store->messages().size(), 1);
    QCOMPARE(m_store->messages().first().message, QString("First"));
}

void TestMessageStore::testMarkAsRead()
{
    Message msg;
    msg.id = "1";
    msg.message = "Unread";

    m_store->addMessage(msg);
    QCOMPARE(m_store->unreadCount(), 1);

    m_store->markAsRead("1");
    QCOMPARE(m_store->unreadCount(), 0);
}

void TestMessageStore::testMarkAllAsRead()
{
    for (int i = 0; i < 3; ++i) {
        Message msg;
        msg.id = QString::number(i);
        m_store->addMessage(msg);
    }

    QCOMPARE(m_store->unreadCount(), 3);
    m_store->markAllAsRead();
    QCOMPARE(m_store->unreadCount(), 0);
}

void TestMessageStore::testRemoveMessage()
{
    Message msg;
    msg.id = "1";
    m_store->addMessage(msg);
    QCOMPARE(m_store->messages().size(), 1);

    m_store->removeMessage("1");
    QCOMPARE(m_store->messages().size(), 0);
}

void TestMessageStore::testUnreadCount()
{
    Message m1, m2, m3;
    m1.id = "1"; m1.read = false;
    m2.id = "2"; m2.read = true;
    m3.id = "3"; m3.read = false;

    m_store->addMessage(m1);
    m_store->addMessage(m2);
    m_store->addMessage(m3);

    QCOMPARE(m_store->unreadCount(), 2);
}

void TestMessageStore::testHighestMessageId()
{
    Message m1, m2;
    m1.id = "1";
    m2.id = "2";

    m_store->addMessage(m1);
    m_store->addMessage(m2);

    QCOMPARE(m_store->highestMessageId(), QString("2"));
}

void TestMessageStore::testContainsMessage()
{
    Message msg;
    msg.id = "exists";
    m_store->addMessage(msg);

    QVERIFY(m_store->containsMessage("exists"));
    QVERIFY(!m_store->containsMessage("nonexistent"));
}

void TestMessageStore::testTrimMessages()
{
    for (int i = 0; i < 501; ++i) {
        Message msg;
        msg.id = QString::number(i);
        m_store->addMessage(msg);
    }
    QCOMPARE(m_store->messages().size(), 500);
}

void TestMessageStore::testSaveAndLoad()
{
    Message msg;
    msg.id = "save_test";
    msg.title = "Saved Title";
    msg.message = "Saved Body";
    msg.app = "SavedApp";
    msg.priority = 1;

    m_store->addMessage(msg);
    QVERIFY(m_store->save());

    MessageStore *store2 = new MessageStore(m_tempDir.path(), this);
    QVERIFY(store2->load());
    QCOMPARE(store2->messages().size(), 1);
    QCOMPARE(store2->messages().first().id, QString("save_test"));
    QCOMPARE(store2->messages().first().title, QString("Saved Title"));

    delete store2;
}

// tst_messagestore.cpp - TestMessageStore class implementation
#include "tst_messagestore.moc"
