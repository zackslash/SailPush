#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>

struct Message {
    QString id;
    QString umid;
    QString title;
    QString message;
    QString app;
    QString aid;
    QString icon;
    qint64 date;
    int priority;
    QString sound;
    QString url;
    QString urlTitle;
    bool acked;
    QString receipt;
    bool html;
    bool read;
    qint64 localTimestamp;

    Message()
        : date(0), priority(0), acked(false), html(false), read(false), localTimestamp(0) {}

    static Message fromJson(const QJsonObject &obj) {
        Message msg;
        auto numericFallback = [](const QJsonValue &v) -> QString {
            return v.isString() ? v.toString() : v.toVariant().toString();
        };
        msg.id = obj.value("id_str").toString(numericFallback(obj.value("id")));
        msg.umid = obj.value("umid_str").toString(numericFallback(obj.value("umid")));
        msg.title = obj.value("title").toString();
        msg.message = obj.value("message").toString();
        msg.app = obj.value("app").toString();
        msg.aid = obj.value("aid_str").toString(numericFallback(obj.value("aid")));
        msg.icon = obj.value("icon").toString();
        msg.date = obj.value("date").toVariant().toLongLong();
        msg.priority = obj.value("priority").toInt(0);
        msg.sound = obj.value("sound").toString();
        msg.url = obj.value("url").toString();
        msg.urlTitle = obj.value("url_title").toString();
        msg.acked = obj.value("acked").toInt(0) != 0;
        msg.receipt = obj.value("receipt").toString();
        msg.html = obj.value("html").toInt(0) != 0;
        msg.read = obj.value("read").toBool(false);
        msg.localTimestamp = obj.value("local_timestamp").toVariant().toLongLong();
        if (msg.localTimestamp == 0) {
            msg.localTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000;
        }
        return msg;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj.insert("id", id);
        obj.insert("umid", umid);
        obj.insert("title", title);
        obj.insert("message", message);
        obj.insert("app", app);
        obj.insert("aid", aid);
        obj.insert("icon", icon);
        obj.insert("date", static_cast<qint64>(date));
        obj.insert("priority", priority);
        obj.insert("sound", sound);
        obj.insert("url", url);
        obj.insert("url_title", urlTitle);
        obj.insert("acked", acked ? 1 : 0);
        obj.insert("receipt", receipt);
        obj.insert("html", html ? 1 : 0);
        obj.insert("read", read);
        obj.insert("local_timestamp", static_cast<qint64>(localTimestamp));
        return obj;
    }

    bool isEmergency() const { return priority >= 2; }
    bool isHighPriority() const { return priority >= 1; }
    bool isLowPriority() const { return priority <= -1; }
    bool isSilent() const { return priority <= -2; }

    QString displayName() const {
        return title.isEmpty() ? app : title;
    }
};

#endif // MESSAGE_H
