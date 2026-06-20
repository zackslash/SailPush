#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QTimer>
#include <QSettings>
#include <QStandardPaths>
#include "sailpushclient.h"
#include "websocketmanager.h"
#include "messagestore.h"
#include "icredentialstore.h"
#include "dbusinterface.h"
#include "notificationmanager.h"
#include "networkmonitor.h"
#include "cpukeepalive.h"
#include "soundplayer.h"

class Daemon : public QObject {
    Q_OBJECT

public:
    static QString dataPath();
    static QString cachePath();
    explicit Daemon(QObject *parent = nullptr);
    ~Daemon();

    void start();
    void stop();

private slots:
    void onDeviceRegistered(const QString &deviceId);
    void onDeviceRegistrationFailed(const QString &error);
    void onMessagesDownloaded(const QList<Message> &messages);
    void onMessagesDownloadFailed(const QString &error);
    void onMessagesDeleted();
    void onMessageDeleteFailed(const QString &error);
    void onEmergencyAcknowledged();
    void onEmergencyAckFailed(const QString &error);
    void onWsNewMessageAvailable();
    void onWsStateChanged(WebSocketManager::ConnectionState state);
    void onWsError(const QString &error);
    void onWsSessionClosed();
    void onWsReloadRequested();
    void onNetworkAvailable();
    void onNetworkLost();
    void onDbusRequestSync();
    void onDbusRequestReloadCredentials();
    void onDbusRequestReloadSettings();
    void onDbusRequestUpdateSettings(const QVariantMap &settings);
    void onDbusRequestAcknowledge(const QString &receipt);
    void onDbusRequestDeleteMessage(const QString &messageId);
    void onDbusRequestQuit();
    void onDbusRequestOpenMessage(const QString &messageId);
    void onPollingTimeout();
    void onWsDisconnectTimeout();
    void onNotificationAction(const QString &messageId, const QString &action, const QString &receipt);

private:
    void loadSettings();
    void performSync();
    void deleteMessagesUpTo(const QString &highestId);
    void publishNotificationForMessage(const Message &msg, bool isNew);
    bool isUiRunning();
    void updateDiagnostics();

    SailPushClient *m_client;
    WebSocketManager *m_wsManager;
    MessageStore *m_store;
    ICredentialStore *m_credentials;
    DbusInterface *m_dbus;
    NotificationManager *m_notificationManager;
    NetworkMonitor *m_networkMonitor;
    CpuKeepalive *m_cpuKeepalive;
    SoundPlayer *m_soundPlayer;

    QTimer *m_pollingTimer;
    QTimer *m_wsDisconnectTimer;

    QString m_secret;
    QString m_deviceId;

    bool m_startupSyncDone;
    bool m_credentialsLoaded;
    bool m_pollingEnabled;
    int m_pollingIntervalMs;
    bool m_syncInProgress;
    bool m_preventDeepSleep;
    bool m_systemNotifications;
    QString m_credentialError;
};

#endif // DAEMON_H
