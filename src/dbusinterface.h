#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QObject>
#include <QVariantMap>
#include "message.h"
#include "messagestore.h"
#include "websocketmanager.h"

class DbusInterface : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.zackslash.sailpush")
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)

public:
    static constexpr const char *SERVICE_NAME = "com.zackslash.sailpush";
    static constexpr const char *OBJECT_PATH = "/com/zackslash/sailpush";
    static constexpr const char *INTERFACE_NAME = "com.zackslash.sailpush";

    explicit DbusInterface(MessageStore *store, WebSocketManager *wsManager,
                           const QString &cachePath, QObject *parent = nullptr);

    bool registerService();
    void unregisterService();

    int unreadCount() const;
    QString connectionState() const;
    bool isRunning() const { return m_registered; }

public slots:
    Q_SCRIPTABLE QVariantList GetMessages();
    Q_SCRIPTABLE int GetMessageCount();
    Q_SCRIPTABLE int GetUnreadCount();
    Q_SCRIPTABLE bool GetAutoStartEnabled();
    Q_SCRIPTABLE void SetAutoStartEnabled(bool enabled);
    Q_SCRIPTABLE void AcknowledgeEmergency(const QString &receipt);
    Q_SCRIPTABLE void DeleteMessage(const QString &messageId);
    Q_SCRIPTABLE QString GetConnectionState();
    Q_SCRIPTABLE QVariantMap GetSettings();
    Q_SCRIPTABLE void UpdateSettings(const QVariantMap &settings);
    Q_SCRIPTABLE void TriggerSync();
    Q_SCRIPTABLE void ReloadCredentials();
    Q_SCRIPTABLE void ReloadSettings();
    Q_SCRIPTABLE void MarkAsRead(const QString &messageId);
    Q_SCRIPTABLE void MarkAllAsRead();
    Q_SCRIPTABLE void OpenMessage(const QString &messageId);
    Q_SCRIPTABLE QString GetPendingOpenMessage();
    Q_SCRIPTABLE void Quit();
    Q_SCRIPTABLE QString GetDiagnostics();

    // Internal (not exported to D-Bus)
    void onMessageReceived(const Message &msg);
    void notifyUnreadCountChanged();
    void setExtraDiagnostics(const QVariantMap &diagnostics);
    void notifyCredentialsInvalidated(const QString &reason);
    void notifyOpenMessageRequested(const QString &messageId);

signals:
    void MessageReceived(const QVariantMap &message);
    void ConnectionStateChanged(const QString &state);
    void CredentialsInvalidated(const QString &reason);
    void unreadCountChanged();
    void connectionStateChanged();
    void isRunningChanged();
    void requestOpenMessage(const QString &messageId);
    void requestSync();
    void requestReloadCredentials();
    void requestReloadSettings();
    void requestAcknowledge(const QString &receipt);
    void requestDeleteMessage(const QString &messageId);
    void requestQuit();

    void requestUpdateSettings(const QVariantMap &settings);

    // Signal emitted when the daemon wants the UI to open a specific message
    // (e.g., after a notification tap)
    void OpenMessageRequested(const QString &messageId);

private slots:
    void onWsStateChanged(WebSocketManager::ConnectionState state);

private:
    QVariantMap messageToMap(const Message &msg) const;
    QString wsStateToString(WebSocketManager::ConnectionState state) const;

    void refreshAutoStartCache();

    MessageStore *m_store;
    WebSocketManager *m_wsManager;
    bool m_registered;
    bool m_autoStartEnabled;
    QVariantMap m_extraDiagnostics;
    QString m_cachePath;
};

#endif // DBUSINTERFACE_H
