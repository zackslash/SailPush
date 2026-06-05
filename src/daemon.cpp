#include "daemon.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QDir>

Q_LOGGING_CATEGORY(lcDaemon, "com.zackslash.sailpush.daemon")

static const int WS_DISCONNECT_TIMEOUT_MS = 30000;
static const int DEFAULT_POLLING_INTERVAL_MS = 5 * 60 * 1000;

QString Daemon::dataPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + "/com.zackslash/sailpush";
}

QString Daemon::cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
           + "/com.zackslash/sailpush";
}

Daemon::Daemon(QObject *parent)
    : QObject(parent)
    , m_client(new SailPushClient(this))
    , m_wsManager(new WebSocketManager(this))
    , m_store(new MessageStore(dataPath(), this))
    , m_credentials(new CredentialStore(dataPath(), this))
    , m_dbus(nullptr)
    , m_notificationManager(new NotificationManager(this))
    , m_networkMonitor(new NetworkMonitor(this))
    , m_cpuKeepalive(new CpuKeepalive(this))
    , m_soundPlayer(new SoundPlayer(this))
    , m_pollingTimer(new QTimer(this))
    , m_wsDisconnectTimer(new QTimer(this))
    , m_startupSyncDone(false)
    , m_credentialsLoaded(false)
    , m_pollingEnabled(true)
    , m_pollingIntervalMs(DEFAULT_POLLING_INTERVAL_MS)
    , m_syncInProgress(false)
    , m_preventDeepSleep(false)
    , m_systemNotifications(true)
{
    QDir(dataPath()).mkpath(".");
    QDir(cachePath()).mkpath(".");

    m_store->load();

    m_dbus = new DbusInterface(m_store, m_wsManager, this);

    loadSettings();

    m_pollingTimer->setSingleShot(false);
    m_wsDisconnectTimer->setSingleShot(true);
    m_wsDisconnectTimer->setInterval(WS_DISCONNECT_TIMEOUT_MS);

    connect(m_client, &SailPushClient::deviceRegistered, this, &Daemon::onDeviceRegistered);
    connect(m_client, &SailPushClient::deviceRegistrationFailed, this, &Daemon::onDeviceRegistrationFailed);
    connect(m_client, &SailPushClient::messagesDownloaded, this, &Daemon::onMessagesDownloaded);
    connect(m_client, &SailPushClient::messagesDownloadFailed, this, &Daemon::onMessagesDownloadFailed);
    connect(m_client, &SailPushClient::messagesDeleted, this, &Daemon::onMessagesDeleted);
    connect(m_client, &SailPushClient::messageDeleteFailed, this, &Daemon::onMessageDeleteFailed);
    connect(m_client, &SailPushClient::emergencyAcknowledged, this, &Daemon::onEmergencyAcknowledged);
    connect(m_client, &SailPushClient::emergencyAckFailed, this, &Daemon::onEmergencyAckFailed);

    connect(m_wsManager, &WebSocketManager::newMessageAvailable, this, &Daemon::onWsNewMessageAvailable);
    connect(m_wsManager, &WebSocketManager::stateChanged, this, &Daemon::onWsStateChanged);
    connect(m_wsManager, &WebSocketManager::connectionError, this, &Daemon::onWsError);
    connect(m_wsManager, &WebSocketManager::sessionClosedByServer, this, &Daemon::onWsSessionClosed);
    connect(m_wsManager, &WebSocketManager::reloadRequested, this, &Daemon::onWsReloadRequested);

    connect(m_dbus, &DbusInterface::requestSync, this, &Daemon::onDbusRequestSync);
    connect(m_dbus, &DbusInterface::requestReloadCredentials, this, &Daemon::onDbusRequestReloadCredentials);
    connect(m_dbus, &DbusInterface::requestReloadSettings, this, &Daemon::onDbusRequestReloadSettings);
    connect(m_dbus, &DbusInterface::requestAcknowledge, this, &Daemon::onDbusRequestAcknowledge);
    connect(m_dbus, &DbusInterface::requestDeleteMessage, this, &Daemon::onDbusRequestDeleteMessage);
    connect(m_dbus, &DbusInterface::requestQuit, this, &Daemon::onDbusRequestQuit);
    connect(m_dbus, &DbusInterface::requestOpenMessage, this, &Daemon::onDbusRequestOpenMessage);
    connect(m_dbus, &DbusInterface::requestUpdateSettings, this, &Daemon::onDbusRequestUpdateSettings);

    connect(m_networkMonitor, &NetworkMonitor::networkAvailable, this, &Daemon::onNetworkAvailable);
    connect(m_networkMonitor, &NetworkMonitor::networkLost, this, &Daemon::onNetworkLost);

    connect(m_pollingTimer, &QTimer::timeout, this, &Daemon::onPollingTimeout);
    connect(m_wsDisconnectTimer, &QTimer::timeout, this, &Daemon::onWsDisconnectTimeout);

    connect(m_notificationManager, &NotificationManager::notificationActionInvoked,
            this, &Daemon::onNotificationAction);
}

Daemon::~Daemon()
{
    stop();
}

void Daemon::start()
{
    qCInfo(lcDaemon) << "Daemon starting";

    if (!m_dbus->registerService()) {
        qCWarning(lcDaemon) << "Failed to register D-Bus service, continuing anyway";
    }

    QString secret, deviceId, userKey, deviceName;
    if (m_credentials->load(secret, deviceId, userKey, deviceName)) {
        m_secret = secret;
        m_deviceId = deviceId;
        m_credentialsLoaded = true;
        qCInfo(lcDaemon) << "Credentials loaded, performing startup sync";
        performSync();
    } else {
        qCInfo(lcDaemon) << "No credentials found, waiting for UI to configure";
    }
    updateDiagnostics();
}

void Daemon::stop()
{
    qCInfo(lcDaemon) << "Daemon stopping";
    m_pollingTimer->stop();
    m_wsDisconnectTimer->stop();
    m_wsManager->disconnectFromServer();
    m_dbus->unregisterService();
    m_store->save();
}

void Daemon::loadSettings()
{
    QSettings settings("com.zackslash", "sailpush");

    m_pollingEnabled = settings.value("pollingFallback", true).toBool();
    int intervalIndex = settings.value("pollingIntervalIndex", 1).toInt();

    switch (intervalIndex) {
    case 0: m_pollingIntervalMs = 60 * 1000; break;
    case 1: m_pollingIntervalMs = 5 * 60 * 1000; break;
    case 2: m_pollingIntervalMs = 15 * 60 * 1000; break;
    case 3: m_pollingIntervalMs = 30 * 60 * 1000; break;
    default: m_pollingIntervalMs = DEFAULT_POLLING_INTERVAL_MS; break;
    }

    qCInfo(lcDaemon) << "Settings loaded: polling=" << m_pollingEnabled << "interval=" << m_pollingIntervalMs;

    m_preventDeepSleep = settings.value("preventDeepSleep", false).toBool();
    m_systemNotifications = settings.value("systemNotifications", true).toBool();
}

void Daemon::performSync()
{
    if (m_syncInProgress) {
        qCInfo(lcDaemon) << "Sync already in progress, skipping";
        return;
    }

    if (m_secret.isEmpty() || m_deviceId.isEmpty()) {
        qCWarning(lcDaemon) << "Cannot sync: no credentials";
        return;
    }

    qCInfo(lcDaemon) << "Performing message sync";
    m_syncInProgress = true;
    updateDiagnostics();
    if (m_preventDeepSleep) {
        m_cpuKeepalive->start();
    }
    m_client->downloadMessages(m_secret, m_deviceId);

    if (!m_startupSyncDone) {
        m_startupSyncDone = true;
    }
}

void Daemon::deleteMessagesUpTo(const QString &highestId)
{
    if (m_secret.isEmpty() || m_deviceId.isEmpty()) {
        qCWarning(lcDaemon) << "Cannot delete messages: no credentials";
        return;
    }

    qCInfo(lcDaemon) << "Deleting messages up to" << highestId;
    m_client->deleteMessages(m_secret, m_deviceId, highestId);
}

void Daemon::publishNotificationForMessage(const Message &msg, bool isNew)
{
    if (msg.isSilent()) {
        qCInfo(lcDaemon) << "Skipping notification for silent message:" << msg.id;
        return;
    }

    qCInfo(lcDaemon) << "Publishing notification for message:" << msg.id;
    m_notificationManager->publishNotification(msg, m_store->unreadCount(), isNew);

    if (!msg.sound.isEmpty()) {
        QString soundDir = cachePath() + "/sounds";
        m_soundPlayer->play(msg.sound, soundDir);
    }
}

void Daemon::handleEmergencyMessage(const Message &msg)
{
    if (msg.isEmergency() && !msg.acked) {
        qCInfo(lcDaemon) << "Emergency message detected:" << msg.id;
        publishNotificationForMessage(msg, true);
    }
}

void Daemon::onDeviceRegistered(const QString &deviceId)
{
    qCInfo(lcDaemon) << "Device registered:" << deviceId;
    m_deviceId = deviceId;
}

void Daemon::onDeviceRegistrationFailed(const QString &error)
{
    qCWarning(lcDaemon) << "Device registration failed:" << error;
}

void Daemon::onMessagesDownloaded(const QList<Message> &messages)
{
    m_syncInProgress = false;
    qCInfo(lcDaemon) << "Downloaded" << messages.size() << "messages";
    if (m_preventDeepSleep) {
        m_cpuKeepalive->stop();
    }

    QList<Message> newMessages;
    for (const Message &msg : messages) {
        if (!m_store->containsMessage(msg.id)) {
            m_store->addMessage(msg);
            newMessages.append(msg);
            handleEmergencyMessage(msg);
        }
    }

    if (!newMessages.isEmpty()) {
        m_store->save();
    }

    if (!messages.isEmpty()) {
        deleteMessagesUpTo(messages.first().id);
    }

    if (!newMessages.isEmpty() && m_startupSyncDone) {
        for (const Message &msg : newMessages) {
            if (m_systemNotifications) {
                publishNotificationForMessage(msg, true);
            }
            m_dbus->onMessageReceived(msg);
        }
    }

    if (!m_wsManager->isConnected()) {
        qCInfo(lcDaemon) << "Connecting WebSocket after sync";
        m_wsManager->connectToServer(m_deviceId, m_secret);
    }
    updateDiagnostics();
}

void Daemon::onMessagesDownloadFailed(const QString &error)
{
    m_syncInProgress = false;
    qCWarning(lcDaemon) << "Message download failed:" << error;
    if (m_preventDeepSleep) {
        m_cpuKeepalive->stop();
    }

    // Detect invalid credentials (Pushover returns 401/403 for bad secret/device)
    if (error.contains("credentials rejected", Qt::CaseInsensitive)) {
        m_credentialError = tr("Credentials rejected by server. Please re-login.");
        qCWarning(lcDaemon) << m_credentialError;
        // Clear stored credentials so UI shows login page
        m_credentials->clear();
        m_credentialsLoaded = false;
        m_secret.clear();
        m_deviceId.clear();
        m_wsManager->disconnectFromServer();
        m_dbus->notifyCredentialsInvalidated(m_credentialError);
    }

    // WebSocket connection is independent of message download — try connecting anyway
    if (m_credentialsLoaded && !m_wsManager->isConnected()) {
        qCInfo(lcDaemon) << "Connecting WebSocket despite download failure";
        m_wsManager->connectToServer(m_deviceId, m_secret);
    }
    updateDiagnostics();
}

void Daemon::onMessagesDeleted()
{
    qCInfo(lcDaemon) << "Messages deleted from server";
    if (!m_pendingDeleteIds.isEmpty()) {
        for (const QString &id : m_pendingDeleteIds) {
            m_store->removeMessage(id);
        }
        m_store->save();
        m_dbus->notifyUnreadCountChanged();
        m_pendingDeleteIds.clear();
    }
}

void Daemon::onMessageDeleteFailed(const QString &error)
{
    qCWarning(lcDaemon) << "Message delete failed:" << error;
}

void Daemon::onEmergencyAcknowledged()
{
    qCInfo(lcDaemon) << "Emergency message acknowledged";
}

void Daemon::onEmergencyAckFailed(const QString &error)
{
    qCWarning(lcDaemon) << "Emergency acknowledge failed:" << error;
}

void Daemon::onWsNewMessageAvailable()
{
    qCInfo(lcDaemon) << "WebSocket: new message available, triggering sync";
    performSync();
}

void Daemon::onWsStateChanged(WebSocketManager::ConnectionState state)
{
    qCInfo(lcDaemon) << "WebSocket state changed:" << static_cast<int>(state);

    if (m_wsManager->isConnected()) {
        m_wsDisconnectTimer->stop();
        if (m_pollingEnabled) {
            m_pollingTimer->stop();
            qCInfo(lcDaemon) << "WebSocket reconnected, stopping polling fallback";
        }
    } else {
        if (m_pollingEnabled) {
            qCInfo(lcDaemon) << "WebSocket disconnected, starting disconnect timer";
            m_wsDisconnectTimer->start();
        }
    }
    updateDiagnostics();
}

void Daemon::onWsError(const QString &error)
{
    qCWarning(lcDaemon) << "WebSocket error:" << error;
    if (error.contains("Permanent", Qt::CaseInsensitive) || error.contains("re-login", Qt::CaseInsensitive)) {
        m_credentialError = tr("Session expired. Please re-login.");
        qCWarning(lcDaemon) << m_credentialError;
        // Clear stored credentials so UI shows login page
        m_credentials->clear();
        m_credentialsLoaded = false;
        m_secret.clear();
        m_deviceId.clear();
        m_wsManager->disconnectFromServer();
        m_dbus->notifyCredentialsInvalidated(m_credentialError);
    }
    updateDiagnostics();
}

void Daemon::onWsSessionClosed()
{
    qCWarning(lcDaemon) << "WebSocket session closed by server";
}

void Daemon::onWsReloadRequested()
{
    qCInfo(lcDaemon) << "WebSocket reload requested";
    m_wsManager->reconnect();
}

void Daemon::onNetworkAvailable()
{
    qCInfo(lcDaemon) << "Network available";
    if (m_credentialsLoaded && !m_wsManager->isConnected()) {
        qCInfo(lcDaemon) << "Reconnecting WebSocket";
        m_wsManager->connectToServer(m_deviceId, m_secret);
        if (m_pollingEnabled) {
            m_pollingTimer->start(m_pollingIntervalMs);
        }
    }
}

void Daemon::onNetworkLost()
{
    qCInfo(lcDaemon) << "Network lost";
    m_wsManager->disconnectFromServer();
    m_pollingTimer->stop();
}

void Daemon::onPollingTimeout()
{
    if (m_credentialsLoaded && !m_wsManager->isConnected()) {
        qCInfo(lcDaemon) << "Polling fallback: performing sync";
        performSync();
    }
}

void Daemon::onWsDisconnectTimeout()
{
    if (m_pollingEnabled && m_credentialsLoaded && !m_wsManager->isConnected()) {
        qCInfo(lcDaemon) << "WebSocket disconnected for 30s, starting polling fallback";
        m_pollingTimer->start(m_pollingIntervalMs);
    }
}

void Daemon::onDbusRequestSync()
{
    qCInfo(lcDaemon) << "D-Bus: manual sync requested";
    performSync();
}

void Daemon::onDbusRequestReloadCredentials()
{
    qCInfo(lcDaemon) << "D-Bus: reloading credentials";
    QString secret, deviceId, userKey, deviceName;
    if (m_credentials->load(secret, deviceId, userKey, deviceName)) {
        m_secret = secret;
        m_deviceId = deviceId;
        m_credentialsLoaded = true;
        m_credentialError.clear();
        performSync();
    } else {
        qCWarning(lcDaemon) << "Reload failed: no credentials found";
    }
}

void Daemon::onDbusRequestReloadSettings()
{
    qCInfo(lcDaemon) << "D-Bus: reloading settings";
    loadSettings();

    // Apply updated polling settings immediately
    if (m_pollingEnabled && m_pollingTimer->isActive()) {
        m_pollingTimer->setInterval(m_pollingIntervalMs);
        qCInfo(lcDaemon) << "Polling timer interval updated to" << m_pollingIntervalMs;
    }
    if (!m_pollingEnabled && m_pollingTimer->isActive()) {
        m_pollingTimer->stop();
        qCInfo(lcDaemon) << "Polling disabled, timer stopped";
    }
}

void Daemon::onDbusRequestUpdateSettings(const QVariantMap &settings)
{
    qCInfo(lcDaemon) << "D-Bus: settings updated from QML";

    if (settings.contains("pollingFallback")) {
        m_pollingEnabled = settings.value("pollingFallback").toBool();
    }
    if (settings.contains("pollingIntervalIndex")) {
        int intervalIndex = settings.value("pollingIntervalIndex").toInt();
        switch (intervalIndex) {
        case 0: m_pollingIntervalMs = 60 * 1000; break;
        case 1: m_pollingIntervalMs = 5 * 60 * 1000; break;
        case 2: m_pollingIntervalMs = 15 * 60 * 1000; break;
        case 3: m_pollingIntervalMs = 30 * 60 * 1000; break;
        default: m_pollingIntervalMs = DEFAULT_POLLING_INTERVAL_MS; break;
        }
    }
    if (settings.contains("preventDeepSleep")) {
        m_preventDeepSleep = settings.value("preventDeepSleep").toBool();
    }
    if (settings.contains("systemNotifications")) {
        m_systemNotifications = settings.value("systemNotifications").toBool();
    }

    qCInfo(lcDaemon) << "Settings applied: polling=" << m_pollingEnabled
                     << "interval=" << m_pollingIntervalMs
                     << "preventDeepSleep=" << m_preventDeepSleep
                     << "systemNotifications=" << m_systemNotifications;

    // Apply updated polling settings immediately
    if (m_pollingEnabled && m_pollingTimer->isActive()) {
        m_pollingTimer->setInterval(m_pollingIntervalMs);
    }
    if (!m_pollingEnabled && m_pollingTimer->isActive()) {
        m_pollingTimer->stop();
    }
}

void Daemon::onDbusRequestAcknowledge(const QString &receipt)
{
    qCInfo(lcDaemon) << "D-Bus: acknowledge emergency:" << receipt;
    m_client->acknowledgeEmergency(m_secret, receipt);
}

void Daemon::onDbusRequestDeleteMessage(const QString &messageId)
{
    qCInfo(lcDaemon) << "D-Bus: delete message:" << messageId;
    // Track for deletion after server confirms
    m_pendingDeleteIds.insert(messageId);
    m_client->deleteMessages(m_secret, m_deviceId, messageId);
}

void Daemon::onDbusRequestQuit()
{
    qCInfo(lcDaemon) << "D-Bus: quit requested";
    QCoreApplication::quit();
}

void Daemon::onDbusRequestOpenMessage(const QString &messageId)
{
    Q_UNUSED(messageId);
    qCInfo(lcDaemon) << "D-Bus: open message requested:" << messageId;
}

void Daemon::onNotificationAction(const QString &messageId, const QString &action, const QString &receipt)
{
    qCInfo(lcDaemon) << "Notification action:" << action << "message:" << messageId << "receipt:" << receipt;

    if (action == "default") {
        m_store->markAsRead(messageId);
        m_store->save();
        m_dbus->notifyUnreadCountChanged();
    } else if (action == "acknowledge") {
        if (!receipt.isEmpty()) {
            m_client->acknowledgeEmergency(m_secret, receipt);
        }
    }
}

void Daemon::updateDiagnostics()
{
    QVariantMap diag;
    diag["credentialsLoaded"] = m_credentialsLoaded;
    diag["syncInProgress"] = m_syncInProgress;
    diag["startupSyncDone"] = m_startupSyncDone;
    diag["pollingEnabled"] = m_pollingEnabled;
    diag["pollingActive"] = m_pollingTimer->isActive();
    diag["wsDisconnectTimerActive"] = m_wsDisconnectTimer->isActive();
    diag["secretPresent"] = !m_secret.isEmpty();
    diag["deviceIdPresent"] = !m_deviceId.isEmpty();
    diag["pendingDeletes"] = m_pendingDeleteIds.size();
    if (!m_credentialError.isEmpty()) {
        diag["credentialError"] = m_credentialError;
    }
    m_dbus->setExtraDiagnostics(diag);
}
