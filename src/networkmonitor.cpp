#include "networkmonitor.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcNetworkMonitor, "com.zackslash.sailpush.network")

NetworkMonitor::NetworkMonitor(QObject *parent)
    : QObject(parent)
    , m_conn(QDBusConnection::systemBus())
    , m_isOnline(false)
{
    QDBusInterface manager("net.connman", "/", "net.connman.Manager", m_conn);
    if (manager.isValid()) {
        QVariant onlineProp = manager.property("Online");
        if (onlineProp.isValid()) {
            m_isOnline = onlineProp.toBool();
        }

        m_conn.connect("net.connman", "/", "net.connman.Manager",
                       "PropertyChanged", this,
                       SLOT(onManagerPropertyChanged(QString, QDBusVariant)));

        m_conn.connect("net.connman", "/", "net.connman.Manager",
                       "TechnologyAdded", this,
                       SLOT(onTechnologyAdded(QDBusObjectPath)));

        m_conn.connect("net.connman", "/", "net.connman.Manager",
                       "TechnologyRemoved", this,
                       SLOT(onTechnologyRemoved(QDBusObjectPath)));

        QDBusReply<QList<QDBusObjectPath>> techs = manager.call("GetTechnologies");
        if (techs.isValid()) {
            for (const QDBusObjectPath &path : techs.value()) {
                watchTechnology(path.path());
            }
        }
    } else {
        qCWarning(lcNetworkMonitor) << "ConnMan Manager not available, assuming offline";
    }
}

NetworkMonitor::~NetworkMonitor()
{
    for (const QString &path : m_watchedTechnologies) {
        unwatchTechnology(path);
    }
}

bool NetworkMonitor::isOnline() const
{
    return m_isOnline;
}

void NetworkMonitor::onManagerPropertyChanged(const QString &property, const QDBusVariant &value)
{
    if (property == "Online") {
        bool wasOnline = m_isOnline;
        m_isOnline = value.variant().toBool();
        qCInfo(lcNetworkMonitor) << "ConnMan Online:" << m_isOnline;
        if (m_isOnline && !wasOnline) {
            emit networkAvailable();
        } else if (!m_isOnline && wasOnline) {
            emit networkLost();
        }
    }
}

void NetworkMonitor::onTechnologyAdded(const QDBusObjectPath &path)
{
    qCInfo(lcNetworkMonitor) << "Technology added:" << path.path();
    watchTechnology(path.path());
    checkOnlineStatus();
}

void NetworkMonitor::onTechnologyRemoved(const QDBusObjectPath &path)
{
    qCInfo(lcNetworkMonitor) << "Technology removed:" << path.path();
    unwatchTechnology(path.path());
    checkOnlineStatus();
}

void NetworkMonitor::onTechnologyPropertyChanged(const QString &property, const QDBusVariant &value)
{
    if (property == "State" || property == "Connected") {
        qCInfo(lcNetworkMonitor) << "Technology property changed:" << property;
        checkOnlineStatus();
    }
}

void NetworkMonitor::checkOnlineStatus()
{
    QDBusInterface manager("net.connman", "/", "net.connman.Manager", m_conn);
    if (manager.isValid()) {
        QVariant onlineProp = manager.property("Online");
        if (onlineProp.isValid()) {
            bool wasOnline = m_isOnline;
            m_isOnline = onlineProp.toBool();
            if (m_isOnline && !wasOnline) {
                emit networkAvailable();
            } else if (!m_isOnline && wasOnline) {
                emit networkLost();
            }
        }
    }
}

void NetworkMonitor::watchTechnology(const QString &path)
{
    if (m_watchedTechnologies.contains(path)) {
        return;
    }

    m_conn.connect("net.connman", path, "net.connman.Technology",
                   "PropertyChanged", this,
                   SLOT(onTechnologyPropertyChanged(QString, QDBusVariant)));
    m_watchedTechnologies.append(path);
}

void NetworkMonitor::unwatchTechnology(const QString &path)
{
    m_conn.disconnect("net.connman", path, "net.connman.Technology",
                      "PropertyChanged", this,
                      SLOT(onTechnologyPropertyChanged(QString, QDBusVariant)));
    m_watchedTechnologies.removeAll(path);
}
