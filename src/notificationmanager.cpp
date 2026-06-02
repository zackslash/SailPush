#include "notificationmanager.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnection>
#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(lcNotification, "net.sailpush.sailfish.notification")

static const QString DBUS_SERVICE = QStringLiteral("org.freedesktop.Notifications");
static const QString DBUS_PATH = QStringLiteral("/org/freedesktop/Notifications");
static const QString DBUS_IFACE = QStringLiteral("org.freedesktop.Notifications");

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_notificationsIface(new QDBusInterface(DBUS_SERVICE, DBUS_PATH, DBUS_IFACE,
                                              QDBusConnection::sessionBus(), this))
{
    QDBusConnection::sessionBus().connect(DBUS_SERVICE, DBUS_PATH, DBUS_IFACE,
                                          "ActionInvoked", this,
                                          SLOT(onActionInvoked(uint, QString)));
}

void NotificationManager::publishNotification(const Message &msg, int unreadCount, bool displayOn)
{
    if (!m_notificationsIface->isValid()) {
        qCWarning(lcNotification) << "Notification service not available";
        return;
    }

    QString summary = msg.displayName();
    const QString plainBody = msg.html ? stripHtml(msg.message) : msg.message;

    QVariantMap hints = buildHints(msg, displayOn);
    hints.insert("x-nemo-preview-summary", summary);
    hints.insert("x-nemo-preview-body", truncate(plainBody, 100));
    hints.insert("category", CATEGORY);

    if (unreadCount > 0) {
        hints.insert("x-nemo-item-count", unreadCount);
    }

    if (msg.isEmergency()) {
        hints.insert("urgency", 2);
        hints.insert("x-nemo-display-on", true);
    } else if (msg.isHighPriority()) {
        hints.insert("urgency", 2);
    } else if (msg.isLowPriority()) {
        hints.insert("urgency", 0);
    }

    QStringList actions = buildActions(msg);

    uint replacesId = 0;
    // Reuse existing notification for same message (e.g., emergency re-delivery)
    for (auto it = m_notificationMessageMap.constBegin(); it != m_notificationMessageMap.constEnd(); ++it) {
        if (it.value() == msg.id) {
            replacesId = it.key();
            break;
        }
    }
    int expireTimeout = -1;

    QDBusReply<uint> reply = m_notificationsIface->call("Notify",
        APP_NAME,
        replacesId,
        APP_ICON,
        summary,
        plainBody,
        actions,
        hints,
        expireTimeout
    );

    if (reply.isValid()) {
        uint id = reply.value();
        qCInfo(lcNotification) << "Notification published with ID:" << id;
        trackNotification(id, msg);

    } else {
        qCWarning(lcNotification) << "Failed to publish notification:" << reply.error().message();
    }
}

void NotificationManager::onActionInvoked(uint id, const QString &actionKey)
{
    if (m_notificationMessageMap.contains(id)) {
        QString messageId = m_notificationMessageMap.take(id);
        QString receipt = m_notificationReceiptMap.take(id);
        qCInfo(lcNotification) << "Notification action:" << actionKey << "for message:" << messageId;
        emit notificationActionInvoked(messageId, actionKey, receipt);
    }
}

QString NotificationManager::truncate(const QString &text, int maxLength)
{
    if (text.length() <= maxLength) return text;
    return text.left(maxLength - 3) + "...";
}

QString NotificationManager::stripHtml(const QString &html)
{
    static const QRegularExpression brRe("<br\\s*/?>", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression pRe("</p>", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tagRe("<[^>]*>");
    static const QRegularExpression hexEntityRe("&#x([0-9a-fA-F]+);");
    static const QRegularExpression decEntityRe("&#(\\d+);");

    QString text = html;
    text.replace(brRe, "\n");
    text.replace(pRe, "\n\n");
    text.replace(tagRe, "");
    // Named entities
    text.replace("&amp;", "&");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&quot;", "\"");
    text.replace("&apos;", "'");
    text.replace("&nbsp;", " ");
    text.replace("&copy;", "\u00A9");
    text.replace("&mdash;", "\u2014");
    text.replace("&ndash;", "\u2013");
    text.replace("&hellip;", "\u2026");
    // Numeric entities — safe to loop since replacements are single chars
    QRegularExpressionMatch m;
    while ((m = hexEntityRe.match(text)).hasMatch()) {
        text.replace(m.capturedStart(), m.capturedLength(),
                     QChar(m.captured(1).toUInt(nullptr, 16)));
    }
    while ((m = decEntityRe.match(text)).hasMatch()) {
        text.replace(m.capturedStart(), m.capturedLength(),
                     QChar(m.captured(1).toUInt()));
    }
    return text;
}

QVariantMap NotificationManager::buildHints(const Message &msg, bool displayOn) const
{
    QVariantMap hints;

    if (displayOn) {
        hints.insert("x-nemo-display-on", true);
    }

    if (!msg.sound.isEmpty()) {
        hints.insert("x-nemo-feedback", msg.sound);
    }

    if (msg.html) {
        hints.insert("x-nemo-html-body", msg.message);
    }

    return hints;
}

QStringList NotificationManager::buildActions(const Message &msg) const
{
    QStringList actions;
    actions.append("default");
    actions.append(tr("Open"));

    if (msg.isEmergency() && !msg.acked) {
        actions.append("acknowledge");
        actions.append(tr("Acknowledge"));
    }

    actions.append("ignore");
    actions.append(tr("Dismiss"));

    return actions;
}

void NotificationManager::trackNotification(uint notifId, const Message &msg)
{
    m_notificationMessageMap.insert(notifId, msg.id);
    if (!msg.receipt.isEmpty()) {
        m_notificationReceiptMap.insert(notifId, msg.receipt);
    }
}
