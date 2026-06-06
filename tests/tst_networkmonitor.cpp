#include <QtTest>
#include <QSignalSpy>
#include <QDBusConnection>
#include <QDBusInterface>
#include "networkmonitor.h"

class TestNetworkMonitor : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void testConstruction();
    void testIsOnlineDefault();
    void testSignalsExist();

private:
    NetworkMonitor *m_monitor;
    bool m_connmanAvailable;
};

void TestNetworkMonitor::initTestCase()
{
    QDBusInterface manager("net.connman", "/", "net.connman.Manager",
                           QDBusConnection::systemBus());
    m_connmanAvailable = manager.isValid();
}

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
    if (!m_connmanAvailable) QSKIP("ConnMan not available");
    // Verify isOnline() returns without crashing when ConnMan is available
    Q_UNUSED(m_monitor->isOnline());
}

void TestNetworkMonitor::testSignalsExist()
{
    QSignalSpy availableSpy(m_monitor, &NetworkMonitor::networkAvailable);
    QSignalSpy lostSpy(m_monitor, &NetworkMonitor::networkLost);
    QVERIFY(availableSpy.isValid());
    QVERIFY(lostSpy.isValid());
}

QTEST_MAIN(TestNetworkMonitor)
#include "tst_networkmonitor.moc"
