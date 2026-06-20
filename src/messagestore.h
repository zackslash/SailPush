#ifndef MESSAGESTORE_H
#define MESSAGESTORE_H

#include <QObject>
#include <QList>
#include <QSet>
#include "message.h"

class MessageStore : public QObject {
    Q_OBJECT

public:
    static constexpr int DEFAULT_MAX_MESSAGES = 500;

    explicit MessageStore(const QString &dataPath, QObject *parent = nullptr);

    bool load();
    bool save() const;

    QList<Message> messages() const { return m_messages; }
    int unreadCount() const;
    int totalCount() const { return m_messages.size(); }

    void addMessage(const Message &msg);
    void markAsRead(const QString &id);
    void markAllAsRead();
    void removeMessage(const QString &id);

    bool containsMessage(const QString &id) const;

private:
    void trimMessages();
    QString storagePath() const;

    QList<Message> m_messages;
    QSet<QString> m_idSet;
    QString m_dataPath;
    int m_maxMessages;
};

#endif // MESSAGESTORE_H
