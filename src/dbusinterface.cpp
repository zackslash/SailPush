#include "dbusinterface.h"
#include "constants.h"
#include <QDBusConnection>
#include <QDBusError>
#include <QLoggingCategory>
#include <QProcess>
#include <QFile>

Q_LOGGING_CATEGORY(lcDbus, "com.zackslash.sailpush.dbus")

DbusInterface::DbusInterface(MessageStore *store, WebSocketManager *wsManager,
                             const QString &cachePath, QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_wsManager(wsManager)
    , m_registered(false)
    , m_autoStartEnabled(false)
    , m_cachePath(cachePath)
{
    connect(wsManager, &WebSocketManager::stateChanged,
            this, &DbusInterface::onWsStateChanged);
    refreshAutoStartCache();
}

bool DbusInterface::registerService()
{
    QDBusConnection conn = QDBusConnection::sessionBus();
    if (!conn.registerService(SERVICE_NAME)) {
        qCWarning(lcDbus) << "Failed to register D-Bus service:" << conn.lastError().message();
        return false;
    }

    if (!conn.registerObject(OBJECT_PATH, this, QDBusConnection::ExportScriptableSlots |
                                               QDBusConnection::ExportScriptableSignals |
                                               QDBusConnection::ExportScriptableProperties)) {
        qCWarning(lcDbus) << "Failed to register D-Bus object:" << conn.lastError().message();
        conn.unregisterService(SERVICE_NAME);
        return false;
    }

    m_registered = true;
    qCInfo(lcDbus) << "D-Bus service registered:" << SERVICE_NAME;
    emit isRunningChanged();
    return true;
}

void DbusInterface::unregisterService()
{
    if (m_registered) {
        QDBusConnection conn = QDBusConnection::sessionBus();
        conn.unregisterObject(OBJECT_PATH);
        conn.unregisterService(SERVICE_NAME);
        m_registered = false;
        emit isRunningChanged();
    }
}

int DbusInterface::unreadCount() const
{
    return m_store->unreadCount();
}

QString DbusInterface::connectionState() const
{
    return wsStateToString(m_wsManager->state());
}

QVariantList DbusInterface::GetMessages()
{
    QVariantList result;
    for (const Message &msg : m_store->messages()) {
        result.append(messageToMap(msg));
    }
    return result;
}

int DbusInterface::GetMessageCount()
{
    return m_store->totalCount();
}

int DbusInterface::GetUnreadCount()
{
    return unreadCount();
}

void DbusInterface::AcknowledgeEmergency(const QString &receipt)
{
    emit requestAcknowledge(receipt);
}

void DbusInterface::DeleteMessage(const QString &messageId)
{
    emit requestDeleteMessage(messageId);
}

QString DbusInterface::GetConnectionState()
{
    return connectionState();
}

QVariantMap DbusInterface::GetSettings()
{
    QVariantMap settings;
    settings.insert("unreadCount", unreadCount());
    settings.insert("connectionState", connectionState());
    settings.insert("messageCount", m_store->totalCount());
    return settings;
}

void DbusInterface::UpdateSettings(const QVariantMap &settings)
{
    emit requestUpdateSettings(settings);
}

void DbusInterface::TriggerSync()
{
    emit requestSync();
}

void DbusInterface::ReloadCredentials()
{
    emit requestReloadCredentials();
}

void DbusInterface::ReloadSettings()
{
    emit requestReloadSettings();
}

void DbusInterface::MarkAsRead(const QString &messageId)
{
    m_store->markAsRead(messageId);
    m_store->save();
    emit unreadCountChanged();
}

void DbusInterface::MarkAllAsRead()
{
    m_store->markAllAsRead();
    m_store->save();
    emit unreadCountChanged();
}

void DbusInterface::OpenMessage(const QString &messageId)
{
    emit requestOpenMessage(messageId);
}

QString DbusInterface::GetPendingOpenMessage()
{
    QString path = m_cachePath + SailPushPaths::PENDING_OPEN;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QString messageId = QString::fromUtf8(file.readAll()).trimmed();
        file.close();
        // Remove the file after reading so it's only consumed once
        if (!QFile::remove(path)) {
            qCWarning(lcDbus) << "Failed to remove pending_open file:" << path;
        }
        qCInfo(lcDbus) << "Consumed pending open message:" << messageId;
        return messageId;
    }
    return QString();
}

void DbusInterface::Quit()
{
    emit requestQuit();
}

QString DbusInterface::GetDiagnostics()
{
    QVariantMap diag;
    diag["wsState"] = wsStateToString(m_wsManager->state());
    diag["wsStateRaw"] = static_cast<int>(m_wsManager->state());
    diag["wsConnected"] = m_wsManager->isConnected();
    diag["messageCount"] = m_store->totalCount();
    diag["unreadCount"] = m_store->unreadCount();
    diag["registered"] = m_registered;
    for (auto it = m_extraDiagnostics.constBegin(); it != m_extraDiagnostics.constEnd(); ++it) {
        diag[it.key()] = it.value();
    }
    // Format as readable string
    QStringList lines;
    for (auto it = diag.constBegin(); it != diag.constEnd(); ++it) {
        lines << QString("%1: %2").arg(it.key(), it.value().toString());
    }
    return lines.join("\n");
}

QString DbusInterface::GetVersion()
{
#ifdef GIT_VERSION
    return QStringLiteral(GIT_VERSION);
#else
    return QStringLiteral("dev");
#endif
}

void DbusInterface::setExtraDiagnostics(const QVariantMap &diagnostics)
{
    m_extraDiagnostics = diagnostics;
}

void DbusInterface::notifyCredentialsInvalidated(const QString &reason)
{
    emit CredentialsInvalidated(reason);
}

void DbusInterface::notifyOpenMessageRequested(const QString &messageId)
{
    emit OpenMessageRequested(messageId);
}

void DbusInterface::notifyUnreadCountChanged()
{
    emit unreadCountChanged();
}

void DbusInterface::runSystemctl(const QStringList &args,
                                  std::function<void(int, const QString &, const QString &)> callback)
{
    QProcess *proc = new QProcess(this);
    connect(proc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this, proc, callback](int exitCode, QProcess::ExitStatus) {
        QString stdOut = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        QString stdErr = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        callback(exitCode, stdOut, stdErr);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError) {
        qCWarning(lcDbus) << "systemctl failed:" << proc->errorString();
        proc->deleteLater();
    });
    proc->start("/usr/bin/systemctl", args);
}

void DbusInterface::refreshAutoStartCache()
{
    runSystemctl({"--user", "is-enabled", "sailpush"},
        [this](int, const QString &stdOut, const QString &) {
            m_autoStartEnabled = (stdOut == "enabled");
            qCInfo(lcDbus) << "AutoStart cache refreshed:" << m_autoStartEnabled;
        });
}

bool DbusInterface::GetAutoStartEnabled()
{
    return m_autoStartEnabled;
}

void DbusInterface::SetAutoStartEnabled(bool enabled)
{
    qCInfo(lcDbus) << "Setting AutoStart to" << enabled;
    runSystemctl({"--user", enabled ? "enable" : "disable", "sailpush"},
        [this, enabled](int exitCode, const QString &stdOut, const QString &stdErr) {
            if (!stdOut.isEmpty()) qCInfo(lcDbus) << "systemctl stdout:" << stdOut;
            if (!stdErr.isEmpty()) qCWarning(lcDbus) << "systemctl stderr:" << stdErr;
            if (exitCode != 0) {
                qCWarning(lcDbus) << "systemctl exit code:" << exitCode;
            } else {
                m_autoStartEnabled = enabled;
                qCInfo(lcDbus) << "AutoStart cache updated:" << m_autoStartEnabled;
            }
        });
}

void DbusInterface::onMessageReceived(const Message &msg)
{
    emit MessageReceived(messageToMap(msg));
    emit unreadCountChanged();
}

void DbusInterface::onWsStateChanged(WebSocketManager::ConnectionState state)
{
    QString stateStr = wsStateToString(state);
    emit ConnectionStateChanged(stateStr);
    emit connectionStateChanged();
}

QVariantMap DbusInterface::messageToMap(const Message &msg) const
{
    QVariantMap map;
    map.insert("id", msg.id);
    map.insert("umid", msg.umid);
    map.insert("title", msg.title);
    map.insert("message", msg.message);
    map.insert("app", msg.app);
    map.insert("aid", msg.aid);
    map.insert("icon", msg.icon);
    map.insert("date", static_cast<qlonglong>(msg.date));
    map.insert("priority", msg.priority);
    map.insert("sound", msg.sound);
    map.insert("url", msg.url);
    map.insert("url_title", msg.urlTitle);
    map.insert("acked", msg.acked);
    map.insert("receipt", msg.receipt);
    map.insert("html", msg.html);
    map.insert("read", msg.read);
    map.insert("local_timestamp", static_cast<qlonglong>(msg.localTimestamp));
    map.insert("display_name", msg.displayName());
    map.insert("is_emergency", msg.isEmergency());
    return map;
}

QString DbusInterface::wsStateToString(WebSocketManager::ConnectionState state) const
{
    switch (state) {
    case WebSocketManager::ConnectionState::Disconnected:  return "disconnected";
    case WebSocketManager::ConnectionState::Connecting:    return "connecting";
    case WebSocketManager::ConnectionState::Connected:     return "connected";
    case WebSocketManager::ConnectionState::LoginSent:     return "ready";
    default: return "unknown";
    }
}
