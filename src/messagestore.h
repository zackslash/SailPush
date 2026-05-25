#ifndef MESSAGESTORE_H
#define MESSAGESTORE_H

#include <QObject>
#include <QList>
#include "message.h"

class MessageStore : public QObject {
    Q_OBJECT

public:
    static constexpr int DEFAULT_MAX_MESSAGES = 500;

    explicit MessageStore(const QString &dataPath, QObject *parent = nullptr);

    bool load();
    bool save() const;

    QList<Message> messages() const { return m_messages; }
    QList<Message> unreadMessages() const;
    int unreadCount() const;
    int totalCount() const { return m_messages.size(); }

    void addMessage(const Message &msg);
    void markAsRead(const QString &id);
    void markAllAsRead();
    void removeMessage(const QString &id);

    QString highestMessageId() const;
    int maxMessages() const { return m_maxMessages; }

    bool containsMessage(const QString &id) const;

private:
    void trimMessages();
    QString storagePath() const;

    QList<Message> m_messages;
    QString m_dataPath;
    int m_maxMessages;
};

#endif // MESSAGESTORE_H
