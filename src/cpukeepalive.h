#ifndef CPUKEEPALIVE_H
#define CPUKEEPALIVE_H

#include <QObject>
#include <QDBusConnection>

class CpuKeepalive : public QObject {
    Q_OBJECT

public:
    explicit CpuKeepalive(QObject *parent = nullptr);
    ~CpuKeepalive();

    void start();
    void stop();
    bool isActive() const;

private:
    QDBusConnection m_conn;
    QString m_cookie;
    int m_refCount;
};

#endif // CPUKEEPALIVE_H
