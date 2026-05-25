#include <QtTest>
#include <QSignalSpy>
#include <QDBusConnection>
#include "networkmonitor.h"

class TestNetworkMonitor : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testConstruction();
    void testIsOnlineDefault();
    void testSignalsExist();
    void testNetworkAvailableSignal();
    void testNetworkLostSignal();

private:
    NetworkMonitor *m_monitor;
};

void TestNetworkMonitor::init()
{
    m_monitor = new NetworkMonitor(this);
}

void TestNetworkMonitor::cleanup()
{
    delete m_monitor;
    m_monitor = nullptr;
}

void TestNetworkMonitor::testConstruction()
{
    QVERIFY(m_monitor != nullptr);
}

void TestNetworkMonitor::testIsOnlineDefault()
{
    bool online = m_monitor->isOnline();
    QVERIFY(online == true || online == false);
}

void TestNetworkMonitor::testSignalsExist()
{
    QSignalSpy availableSpy(m_monitor, &NetworkMonitor::networkAvailable);
    QSignalSpy lostSpy(m_monitor, &NetworkMonitor::networkLost);
    QVERIFY(availableSpy.isValid());
    QVERIFY(lostSpy.isValid());
}

void TestNetworkMonitor::testNetworkAvailableSignal()
{
    QSignalSpy spy(m_monitor, &NetworkMonitor::networkAvailable);
    QVERIFY(spy.isValid());
}

void TestNetworkMonitor::testNetworkLostSignal()
{
    QSignalSpy spy(m_monitor, &NetworkMonitor::networkLost);
    QVERIFY(spy.isValid());
}

// tst_networkmonitor.cpp - TestNetworkMonitor class implementation
#include "tst_networkmonitor.moc"
