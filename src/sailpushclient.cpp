#include "sailpushclient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(lcSailPushClient, "com.zackslash.sailpush.client")

#ifdef GIT_VERSION
static const QString APP_VERSION = QStringLiteral(GIT_VERSION);
#else
static const QString APP_VERSION = QStringLiteral("dev");
#endif
static const QString USER_AGENT = QStringLiteral("%1/%2 (SailfishOS; Qt/%3)").arg(SailPushClient::APP_DISPLAY_NAME, APP_VERSION, qVersion());
static const int REQUEST_TIMEOUT_MS = 30000;

SailPushClient::SailPushClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_userAgent(USER_AGENT)
{
}

void SailPushClient::login(const QString &email, const QString &password, const QString &twofa)
{
    QUrl url(loginUrl());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    QUrlQuery query;
    query.addQueryItem("email", email);
    query.addQueryItem("password", password);
    if (!twofa.isEmpty()) {
        query.addQueryItem("twofa", twofa);
    }

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    QTimer::singleShot(REQUEST_TIMEOUT_MS, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    reply->setProperty("action", "login");
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
}

void SailPushClient::registerDevice(const QString &secret, const QString &deviceName)
{
    QUrl url(registerUrl());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    QUrlQuery query;
    query.addQueryItem("secret", secret);
    query.addQueryItem("name", deviceName);
    query.addQueryItem("os", "O");

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    QTimer::singleShot(REQUEST_TIMEOUT_MS, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    reply->setProperty("action", "register");
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
}

void SailPushClient::downloadMessages(const QString &secret, const QString &deviceId)
{
    QUrl url(messagesUrl(secret, deviceId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    QNetworkReply *reply = m_networkManager->get(request);
    QTimer::singleShot(REQUEST_TIMEOUT_MS, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    reply->setProperty("action", "download");
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
}

void SailPushClient::deleteMessages(const QString &secret, const QString &deviceId, const QString &highestMessageId)
{
    QUrl url(deleteUrl(deviceId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    QUrlQuery query;
    query.addQueryItem("secret", secret);
    query.addQueryItem("message", highestMessageId);

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    QTimer::singleShot(REQUEST_TIMEOUT_MS, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    reply->setProperty("action", "delete");
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
}

void SailPushClient::acknowledgeEmergency(const QString &secret, const QString &receipt)
{
    QUrl url(acknowledgeUrl(receipt));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    QUrlQuery query;
    query.addQueryItem("secret", secret);

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    QTimer::singleShot(REQUEST_TIMEOUT_MS, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    reply->setProperty("action", "acknowledge");
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
}

void SailPushClient::onReplyFinished(QNetworkReply *reply)
{
    QString action = reply->property("action").toString();
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    reply->deleteLater();

    // Check for auth failures before generic error handling — Qt treats HTTP 4xx as NoError
    if ((httpStatus == 401 || httpStatus == 403) && (action == "download" || action == "delete" || action == "acknowledge")) {
        qCWarning(lcSailPushClient) << "HTTP" << httpStatus << "for action" << action << "— credentials rejected";
        QString errMsg = QStringLiteral("HTTP %1: credentials rejected").arg(httpStatus);
        if (action == "download") emit messagesDownloadFailed(errMsg);
        else if (action == "delete") emit messageDeleteFailed(errMsg);
        else if (action == "acknowledge") emit emergencyAckFailed(errMsg);
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(lcSailPushClient) << "Network error:" << reply->errorString();
        if (action == "login") {
            emit loginFailed(reply->errorString());
        } else if (action == "register") {
            emit deviceRegistrationFailed(reply->errorString());
        } else if (action == "download") {
            emit messagesDownloadFailed(reply->errorString());
        } else if (action == "delete") {
            emit messageDeleteFailed(reply->errorString());
        } else if (action == "acknowledge") {
            emit emergencyAckFailed(reply->errorString());
        }
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(lcSailPushClient) << "JSON parse error:" << parseError.errorString();
        if (action == "login") emit loginFailed(tr("Invalid server response"));
        else if (action == "register") emit deviceRegistrationFailed(tr("Invalid server response"));
        else if (action == "download") emit messagesDownloadFailed(tr("Invalid server response"));
        else if (action == "delete") emit messageDeleteFailed(tr("Invalid server response"));
        else if (action == "acknowledge") emit emergencyAckFailed(tr("Invalid server response"));
        return;
    }

    QJsonObject root = doc.object();
    int status = root.value("status").toInt(0);

    if (action == "login") {
        if (httpStatus == 412) {
            emit twoFactorRequired();
            return;
        }
        if (status == 1) {
            emit loginSuccess(root.value("id").toString(), root.value("secret").toString());
        } else {
            QJsonArray errors = root.value("errors").toArray();
            QString errorStr;
            for (const QJsonValue &e : errors) {
                if (!errorStr.isEmpty()) errorStr += "; ";
                errorStr += e.toString();
            }
            emit loginFailed(errorStr.isEmpty() ? tr("Login failed") : errorStr);
        }
    } else if (action == "register") {
        if (status == 1) {
            emit deviceRegistered(root.value("id").toString());
        } else {
            QJsonArray errors = root.value("errors").toArray();
            QString errorStr;
            for (const QJsonValue &e : errors) {
                if (!errorStr.isEmpty()) errorStr += "; ";
                errorStr += e.toString();
            }
            emit deviceRegistrationFailed(errorStr.isEmpty() ? tr("Registration failed") : errorStr);
        }
    } else if (action == "download") {
        if (status == 1) {
            QList<Message> messages;
            QJsonArray msgs = root.value("messages").toArray();
            for (const QJsonValue &v : msgs) {
                messages.append(Message::fromJson(v.toObject()));
            }
            emit messagesDownloaded(messages);
        } else {
            QJsonArray errors = root.value("errors").toArray();
            QString errorStr;
            for (const QJsonValue &e : errors) {
                if (!errorStr.isEmpty()) errorStr += "; ";
                errorStr += e.toString();
            }
            emit messagesDownloadFailed(errorStr.isEmpty() ? tr("Download failed") : errorStr);
        }
    } else if (action == "delete") {
        if (status == 1) {
            emit messagesDeleted();
        } else {
            QJsonArray errors = root.value("errors").toArray();
            QString errorStr;
            for (const QJsonValue &e : errors) {
                if (!errorStr.isEmpty()) errorStr += "; ";
                errorStr += e.toString();
            }
            emit messageDeleteFailed(errorStr.isEmpty() ? tr("Delete failed") : errorStr);
        }
    } else if (action == "acknowledge") {
        if (status == 1) {
            emit emergencyAcknowledged();
        } else {
            QJsonArray errors = root.value("errors").toArray();
            QString errorStr;
            for (const QJsonValue &e : errors) {
                if (!errorStr.isEmpty()) errorStr += "; ";
                errorStr += e.toString();
            }
            emit emergencyAckFailed(errorStr.isEmpty() ? tr("Acknowledge failed") : errorStr);
        }
    }
}

QString SailPushClient::loginUrl() { return QStringLiteral("%1/users/login.json").arg(API_BASE); }
QString SailPushClient::registerUrl() { return QStringLiteral("%1/devices.json").arg(API_BASE); }
// Note: secret is in query string — avoid logging this URL
QString SailPushClient::messagesUrl(const QString &secret, const QString &deviceId) {
    return QStringLiteral("%1/messages.json?secret=%2&device_id=%3").arg(API_BASE, secret, deviceId);
}
QString SailPushClient::deleteUrl(const QString &deviceId) {
    return QStringLiteral("%1/devices/%2/update_highest_message.json").arg(API_BASE, deviceId);
}
QString SailPushClient::acknowledgeUrl(const QString &receipt) {
    return QStringLiteral("%1/receipts/%2/acknowledge.json").arg(API_BASE, receipt);
}

