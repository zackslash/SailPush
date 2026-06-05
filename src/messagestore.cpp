#include "messagestore.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QFileInfo>

Q_LOGGING_CATEGORY(lcMessageStore, "com.zackslash.sailpush.store")

MessageStore::MessageStore(const QString &dataPath, QObject *parent)
    : QObject(parent)
    , m_dataPath(dataPath)
    , m_maxMessages(DEFAULT_MAX_MESSAGES)
{
}

bool MessageStore::load()
{
    QString path = storagePath();
    QFile file(path);
    if (!file.exists()) {
        qCInfo(lcMessageStore) << "No existing message store at" << path;
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcMessageStore) << "Failed to open message store:" << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(lcMessageStore) << "Failed to parse message store:" << parseError.errorString();
        return false;
    }

    m_messages.clear();
    QJsonArray array = doc.array();
    for (const QJsonValue &v : array) {
        m_messages.append(Message::fromJson(v.toObject()));
    }

    qCInfo(lcMessageStore) << "Loaded" << m_messages.size() << "messages from store";
    return true;
}

bool MessageStore::save() const
{
    QString path = storagePath();
    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCWarning(lcMessageStore) << "Failed to create directory:" << dir.path();
            return false;
        }
    }

    QJsonArray array;
    for (const Message &msg : m_messages) {
        array.append(msg.toJson());
    }

    QJsonDocument doc(array);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcMessageStore) << "Failed to save message store:" << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();

    qCInfo(lcMessageStore) << "Saved" << m_messages.size() << "messages to store";
    return true;
}

QList<Message> MessageStore::unreadMessages() const
{
    QList<Message> unread;
    for (const Message &msg : m_messages) {
        if (!msg.read) {
            unread.append(msg);
        }
    }
    return unread;
}

int MessageStore::unreadCount() const
{
    int count = 0;
    for (const Message &msg : m_messages) {
        if (!msg.read) {
            count++;
        }
    }
    return count;
}

void MessageStore::addMessage(const Message &msg)
{
    if (containsMessage(msg.id)) {
        return;
    }
    m_messages.prepend(msg);
    trimMessages();
}

void MessageStore::markAsRead(const QString &id)
{
    for (int i = 0; i < m_messages.size(); ++i) {
        if (m_messages[i].id == id) {
            Message msg = m_messages[i];
            msg.read = true;
            m_messages[i] = msg;
            break;
        }
    }
}

void MessageStore::markAllAsRead()
{
    for (int i = 0; i < m_messages.size(); ++i) {
        Message msg = m_messages[i];
        msg.read = true;
        m_messages[i] = msg;
    }
}

void MessageStore::removeMessage(const QString &id)
{
    for (int i = 0; i < m_messages.size(); ++i) {
        if (m_messages[i].id == id) {
            m_messages.removeAt(i);
            break;
        }
    }
}

QString MessageStore::highestMessageId() const
{
    if (m_messages.isEmpty()) {
        return QString();
    }
    return m_messages.first().id;
}

bool MessageStore::containsMessage(const QString &id) const
{
    for (const Message &msg : m_messages) {
        if (msg.id == id) {
            return true;
        }
    }
    return false;
}

void MessageStore::trimMessages()
{
    while (m_messages.size() > m_maxMessages) {
        m_messages.removeLast();
    }
}

QString MessageStore::storagePath() const
{
    return QDir(m_dataPath).filePath("messages.json");
}
