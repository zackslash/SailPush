#ifndef NETWORKMONITOR_H
#define NETWORKMONITOR_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusVariant>
#include <QDBusObjectPath>

class NetworkMonitor : public QObject {
    Q_OBJECT

public:
    explicit NetworkMonitor(QObject *parent = nullptr);
    ~NetworkMonitor();

    bool isOnline() const;

signals:
    void networkAvailable();
    void networkLost();

private slots:
    void onManagerPropertyChanged(const QString &property, const QDBusVariant &value);
    void onTechnologyPropertyChanged(const QString &property, const QDBusVariant &value);
    void onTechnologyAdded(const QDBusObjectPath &path);
    void onTechnologyRemoved(const QDBusObjectPath &path);

private:
    void checkOnlineStatus();
    void watchTechnology(const QString &path);
    void unwatchTechnology(const QString &path);

    QDBusConnection m_conn;
    bool m_isOnline;
    QStringList m_watchedTechnologies;
};

#endif // NETWORKMONITOR_H
