#include <QtTest>
#include <QDBusConnection>
#include <QDBusInterface>
#include "cpukeepalive.h"

class TestCpuKeepalive : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void testConstruction();
    void testInitialNotActive();
    void testStartStop();
    void testNestedStartStop();
    void testMultipleStarts();
    void testStopWithoutStart();
    void testDestructorStops();

private:
    CpuKeepalive *m_keepalive;
    bool m_mceAvailable;
};

void TestCpuKeepalive::initTestCase()
{
    // Check if MCE D-Bus service is available (only on Sailfish devices)
    QDBusInterface mce("com.nokia.mce", "/com/nokia/mce/request",
                       "com.nokia.mce.request", QDBusConnection::systemBus());
    m_mceAvailable = mce.isValid();
}

void TestCpuKeepalive::init()
{
    m_keepalive = new CpuKeepalive(this);
}

void TestCpuKeepalive::cleanup()
{
    delete m_keepalive;
    m_keepalive = nullptr;
}

void TestCpuKeepalive::testConstruction()
{
    QVERIFY(m_keepalive != nullptr);
}

void TestCpuKeepalive::testInitialNotActive()
{
    QVERIFY(!m_keepalive->isActive());
}

void TestCpuKeepalive::testStartStop()
{
    if (!m_mceAvailable) QSKIP("MCE D-Bus service not available");
    m_keepalive->start();
    QVERIFY(m_keepalive->isActive());

    m_keepalive->stop();
    QVERIFY(!m_keepalive->isActive());
}

void TestCpuKeepalive::testNestedStartStop()
{
    if (!m_mceAvailable) QSKIP("MCE D-Bus service not available");
    m_keepalive->start();
    QVERIFY(m_keepalive->isActive());

    m_keepalive->start();
    QVERIFY(m_keepalive->isActive());

    m_keepalive->stop();
    QVERIFY(m_keepalive->isActive());

    m_keepalive->stop();
    QVERIFY(!m_keepalive->isActive());
}

void TestCpuKeepalive::testMultipleStarts()
{
    if (!m_mceAvailable) QSKIP("MCE D-Bus service not available");
    for (int i = 0; i < 5; ++i) {
        m_keepalive->start();
    }
    QVERIFY(m_keepalive->isActive());

    for (int i = 0; i < 5; ++i) {
        m_keepalive->stop();
    }
    QVERIFY(!m_keepalive->isActive());
}

void TestCpuKeepalive::testStopWithoutStart()
{
    m_keepalive->stop();
    QVERIFY(!m_keepalive->isActive());
}

void TestCpuKeepalive::testDestructorStops()
{
    if (!m_mceAvailable) QSKIP("MCE D-Bus service not available");
    CpuKeepalive *ka = new CpuKeepalive();
    ka->start();
    delete ka;
}

QTEST_MAIN(TestCpuKeepalive)
#include "tst_cpukeepalive.moc"
