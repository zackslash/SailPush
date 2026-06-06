#include <QtTest>
#include <QTemporaryDir>
#include "notificationmanager.h"
#include "message.h"

class TestNotificationManager : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // stripHtml tests
    void testStripHtmlSimpleTag();
    void testStripHtmlBrTag();
    void testStripHtmlParagraphTag();
    void testStripHtmlNestedTags();
    void testStripHtmlNamedEntities();
    void testStripHtmlHexEntities();
    void testStripHtmlDecEntities();
    void testStripHtmlEmpty();
    void testStripHtmlPlainText();
    void testStripHtmlMixedContent();

    // truncate tests
    void testTruncateShortString();
    void testTruncateExactLength();
    void testTruncateLongString();
    void testTruncateEmpty();
    void testTruncateSingleChar();

    // buildHints tests
    void testBuildHintsDisplayOn();
    void testBuildHintsDisplayOff();
    void testBuildHintsWithSound();
    void testBuildHintsEmptySound();
    void testBuildHintsHtmlMessage();
    void testBuildHintsPlainText();

    // buildActions tests
    void testBuildActionsNormalMessage();
    void testBuildActionsEmergencyUnacked();
    void testBuildActionsEmergencyAcked();

private:
    QTemporaryDir m_tempDir;
    NotificationManager *m_mgr;
};

void TestNotificationManager::init()
{
    m_mgr = new NotificationManager(m_tempDir.path(), this);
}

void TestNotificationManager::cleanup()
{
    delete m_mgr;
    m_mgr = nullptr;
}

// --- stripHtml tests ---

void TestNotificationManager::testStripHtmlSimpleTag()
{
    QCOMPARE(NotificationManager::stripHtml("<b>bold</b>"), QString("bold"));
    QCOMPARE(NotificationManager::stripHtml("<i>italic</i>"), QString("italic"));
    QCOMPARE(NotificationManager::stripHtml("<a href=\"url\">link</a>"), QString("link"));
}

void TestNotificationManager::testStripHtmlBrTag()
{
    QCOMPARE(NotificationManager::stripHtml("line1<br>line2"), QString("line1\nline2"));
    QCOMPARE(NotificationManager::stripHtml("line1<br/>line2"), QString("line1\nline2"));
    QCOMPARE(NotificationManager::stripHtml("line1<br />line2"), QString("line1\nline2"));
}

void TestNotificationManager::testStripHtmlParagraphTag()
{
    QCOMPARE(NotificationManager::stripHtml("<p>para1</p><p>para2</p>"),
             QString("para1\n\npara2\n\n"));
}

void TestNotificationManager::testStripHtmlNestedTags()
{
    QCOMPARE(NotificationManager::stripHtml("<div><b>bold <i>and italic</i></b></div>"),
             QString("bold and italic"));
}

void TestNotificationManager::testStripHtmlNamedEntities()
{
    QCOMPARE(NotificationManager::stripHtml("one &amp; two"), QString("one & two"));
    QCOMPARE(NotificationManager::stripHtml("&lt;tag&gt;"), QString("<tag>"));
    QCOMPARE(NotificationManager::stripHtml("a&nbsp;b"), QString("a b"));
    QCOMPARE(NotificationManager::stripHtml("&quot;quoted&quot;"), QString("\"quoted\""));
    QCOMPARE(NotificationManager::stripHtml("&apos;apos&apos;"), QString("'apos'"));
    QCOMPARE(NotificationManager::stripHtml("&copy;"), QString("\u00A9"));
    QCOMPARE(NotificationManager::stripHtml("&mdash;"), QString("\u2014"));
    QCOMPARE(NotificationManager::stripHtml("&ndash;"), QString("\u2013"));
    QCOMPARE(NotificationManager::stripHtml("&hellip;"), QString("\u2026"));
}

void TestNotificationManager::testStripHtmlHexEntities()
{
    QCOMPARE(NotificationManager::stripHtml("&#x41;"), QString("A"));  // hex 'A'
    QCOMPARE(NotificationManager::stripHtml("&#x26;"), QString("&"));  // hex '&'
}

void TestNotificationManager::testStripHtmlDecEntities()
{
    QCOMPARE(NotificationManager::stripHtml("&#65;"), QString("A"));   // dec 'A'
    QCOMPARE(NotificationManager::stripHtml("&#38;"), QString("&"));   // dec '&'
}

void TestNotificationManager::testStripHtmlEmpty()
{
    QCOMPARE(NotificationManager::stripHtml(""), QString(""));
}

void TestNotificationManager::testStripHtmlPlainText()
{
    QCOMPARE(NotificationManager::stripHtml("no tags here"), QString("no tags here"));
}

void TestNotificationManager::testStripHtmlMixedContent()
{
    QString input = "<p>Hello <b>world</b> &amp; <i>friends</i></p>";
    QString expected = "Hello world & friends\n\n";
    QCOMPARE(NotificationManager::stripHtml(input), expected);
}

// --- truncate tests ---

void TestNotificationManager::testTruncateShortString()
{
    QCOMPARE(NotificationManager::truncate("hello", 10), QString("hello"));
}

void TestNotificationManager::testTruncateExactLength()
{
    QCOMPARE(NotificationManager::truncate("hello", 5), QString("hello"));
}

void TestNotificationManager::testTruncateLongString()
{
    QCOMPARE(NotificationManager::truncate("hello world", 8), QString("hello..."));
}

void TestNotificationManager::testTruncateEmpty()
{
    QCOMPARE(NotificationManager::truncate("", 10), QString(""));
}

void TestNotificationManager::testTruncateSingleChar()
{
    QCOMPARE(NotificationManager::truncate("a", 1), QString("a"));
    QCOMPARE(NotificationManager::truncate("ab", 1), QString("a"));
    QCOMPARE(NotificationManager::truncate("abc", 2), QString("ab"));
    QCOMPARE(NotificationManager::truncate("abcd", 3), QString("..."));
}

// --- buildHints tests ---

void TestNotificationManager::testBuildHintsDisplayOn()
{
    Message msg;
    QVariantMap hints = m_mgr->buildHints(msg, true);
    QVERIFY(hints.contains("x-nemo-display-on"));
    QCOMPARE(hints["x-nemo-display-on"].toBool(), true);
}

void TestNotificationManager::testBuildHintsDisplayOff()
{
    Message msg;
    QVariantMap hints = m_mgr->buildHints(msg, false);
    QVERIFY(!hints.contains("x-nemo-display-on"));
}

void TestNotificationManager::testBuildHintsWithSound()
{
    Message msg;
    msg.sound = "siren";
    QVariantMap hints = m_mgr->buildHints(msg, false);
    QVERIFY(hints.contains("x-nemo-feedback"));
    QCOMPARE(hints["x-nemo-feedback"].toString(), QString("siren"));
}

void TestNotificationManager::testBuildHintsEmptySound()
{
    Message msg;
    msg.sound = "";
    QVariantMap hints = m_mgr->buildHints(msg, false);
    QVERIFY(!hints.contains("x-nemo-feedback"));
}

void TestNotificationManager::testBuildHintsHtmlMessage()
{
    Message msg;
    msg.html = true;
    msg.message = "<b>bold</b>";
    QVariantMap hints = m_mgr->buildHints(msg, false);
    QVERIFY(hints.contains("x-nemo-html-body"));
    QCOMPARE(hints["x-nemo-html-body"].toString(), QString("<b>bold</b>"));
}

void TestNotificationManager::testBuildHintsPlainText()
{
    Message msg;
    msg.html = false;
    msg.message = "plain";
    QVariantMap hints = m_mgr->buildHints(msg, false);
    QVERIFY(!hints.contains("x-nemo-html-body"));
}

// --- buildActions tests ---

void TestNotificationManager::testBuildActionsNormalMessage()
{
    Message msg;
    msg.priority = 0;
    QStringList actions = m_mgr->buildActions(msg);
    QVERIFY(actions.contains("default"));
    QVERIFY(actions.contains("ignore"));
    QVERIFY(!actions.contains("acknowledge"));
}

void TestNotificationManager::testBuildActionsEmergencyUnacked()
{
    Message msg;
    msg.priority = 2;
    msg.acked = false;
    QStringList actions = m_mgr->buildActions(msg);
    QVERIFY(actions.contains("default"));
    QVERIFY(actions.contains("acknowledge"));
    QVERIFY(actions.contains("ignore"));
}

void TestNotificationManager::testBuildActionsEmergencyAcked()
{
    Message msg;
    msg.priority = 2;
    msg.acked = true;
    QStringList actions = m_mgr->buildActions(msg);
    QVERIFY(actions.contains("default"));
    QVERIFY(!actions.contains("acknowledge"));
    QVERIFY(actions.contains("ignore"));
}

QTEST_MAIN(TestNotificationManager)
#include "tst_notificationmanager.moc"
