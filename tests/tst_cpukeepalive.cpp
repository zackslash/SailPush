#include <QtTest>
#include "cpukeepalive.h"

class TestCpuKeepalive : public QObject {
    Q_OBJECT

private slots:
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
};

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
    m_keepalive->start();
    QVERIFY(m_keepalive->isActive());

    m_keepalive->stop();
    QVERIFY(!m_keepalive->isActive());
}

void TestCpuKeepalive::testNestedStartStop()
{
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
    CpuKeepalive *ka = new CpuKeepalive();
    ka->start();
    delete ka;
}

// tst_cpukeepalive.cpp - TestCpuKeepalive class implementation
#include "tst_cpukeepalive.moc"
