#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QHash>
#include <QDBusInterface>
#include "message.h"
#include "sailpushclient.h"

class NotificationManager : public QObject {
    Q_OBJECT

public:
    static constexpr const char *CATEGORY = "x-sailpush";
    static constexpr const char *APP_NAME = SailPushClient::APP_DISPLAY_NAME;
    static constexpr const char *APP_ICON = "icon-m-sailpush";

    explicit NotificationManager(QObject *parent = nullptr);

    void publishNotification(const Message &msg, int unreadCount, bool displayOn);
    static QString truncate(const QString &text, int maxLength);
    static QString stripHtml(const QString &html);

signals:
    void notificationActionInvoked(const QString &messageId, const QString &action, const QString &receipt);

private slots:
    void onActionInvoked(uint id, const QString &actionKey);

private:
    QVariantMap buildHints(const Message &msg, bool displayOn) const;
    QStringList buildActions(const Message &msg) const;
    void trackNotification(uint notifId, const Message &msg);

    QHash<uint, QString> m_notificationMessageMap;
    QHash<uint, QString> m_notificationReceiptMap;
    QDBusInterface *m_notificationsIface;
};

#endif // NOTIFICATIONMANAGER_H
