#ifndef SAILPUSHCLIENT_H
#define SAILPUSHCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include "message.h"

class SailPushClient : public QObject {
    Q_OBJECT

public:
    static constexpr const char *API_BASE = "https://api.pushover.net/1";
    static constexpr const char *APP_DISPLAY_NAME = "SailPush";

    explicit SailPushClient(QObject *parent = nullptr);

    void login(const QString &email, const QString &password, const QString &twofa = QString());
    void registerDevice(const QString &secret, const QString &deviceName);
    void downloadMessages(const QString &secret, const QString &deviceId);
    void deleteMessages(const QString &secret, const QString &deviceId, const QString &highestMessageId);
    void acknowledgeEmergency(const QString &secret, const QString &receipt);

    static QString loginUrl();
    static QString registerUrl();
    static QString messagesUrl(const QString &secret, const QString &deviceId);
    static QString deleteUrl(const QString &deviceId);
    static QString acknowledgeUrl(const QString &receipt);

signals:
    void loginSuccess(const QString &userKey, const QString &secret);
    void loginFailed(const QString &error);
    void twoFactorRequired();
    void deviceRegistered(const QString &deviceId);
    void deviceRegistrationFailed(const QString &error);
    void messagesDownloaded(const QList<Message> &messages);
    void messagesDownloadFailed(const QString &error);
    void messagesDeleted();
    void messageDeleteFailed(const QString &error);
    void emergencyAcknowledged();
    void emergencyAckFailed(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    enum class Action {
        Login,
        Register,
        Download,
        Delete,
        Acknowledge
    };

    QNetworkReply* sendRequest(const QUrl &url, const QByteArray &postData, Action action);
    void emitError(Action action, const QString &message);
    static QString extractErrorMessage(const QJsonObject &root);

    QNetworkAccessManager *m_networkManager;
};

#endif // SAILPUSHCLIENT_H
