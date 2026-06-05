#include "cpukeepalive.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>
#include <QUuid>

Q_LOGGING_CATEGORY(lcCpuKeepalive, "com.zackslash.sailpush.keepalive")

CpuKeepalive::CpuKeepalive(QObject *parent)
    : QObject(parent)
    , m_conn(QDBusConnection::systemBus())
    , m_refCount(0)
{
}

CpuKeepalive::~CpuKeepalive()
{
    if (m_refCount > 0) {
        stop();
    }
}

void CpuKeepalive::start()
{
    if (m_refCount == 0) {
        QString name = "com.zackslash.sailpush." + QUuid::createUuid().toString().mid(1, 8);
        QDBusInterface mce("com.nokia.mce", "/com/nokia/mce/request",
                           "com.nokia.mce.request", m_conn);
        if (mce.isValid()) {
            QDBusReply<bool> reply = mce.call("req_cpu_keepalive_start", name);
            if (reply.isValid() && reply.value()) {
                m_cookie = name;
                m_refCount++;
                qCInfo(lcCpuKeepalive) << "CPU keepalive started:" << m_cookie;
            } else {
                qCWarning(lcCpuKeepalive) << "Failed to start CPU keepalive:" << reply.error().message();
            }
        } else {
            qCWarning(lcCpuKeepalive) << "MCE D-Bus interface not available";
        }
    } else {
        m_refCount++;
        qCInfo(lcCpuKeepalive) << "CPU keepalive ref count:" << m_refCount;
    }
}

void CpuKeepalive::stop()
{
    if (m_refCount > 0) {
        m_refCount--;
        if (m_refCount == 0 && !m_cookie.isEmpty()) {
            QDBusInterface mce("com.nokia.mce", "/com/nokia/mce/request",
                               "com.nokia.mce.request", m_conn);
            if (mce.isValid()) {
                QDBusReply<void> reply = mce.call("req_cpu_keepalive_stop", m_cookie);
                if (reply.isValid()) {
                    qCInfo(lcCpuKeepalive) << "CPU keepalive stopped, cookie:" << m_cookie;
                } else {
                    qCWarning(lcCpuKeepalive) << "Failed to stop CPU keepalive:" << reply.error().message();
                }
            }
            m_cookie.clear();
        }
    }
}

bool CpuKeepalive::isActive() const
{
    return m_refCount > 0 && !m_cookie.isEmpty();
}
