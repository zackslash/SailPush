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
{
}

void SailPushClient::login(const QString &email, const QString &password, const QString &twofa)
{
    QUrlQuery query;
    query.addQueryItem("email", email);
    query.addQueryItem("password", password);
    if (!twofa.isEmpty()) {
        query.addQueryItem("twofa", twofa);
    }
    sendRequest(QUrl(loginUrl()), query.toString(QUrl::FullyEncoded).toUtf8(), Action::Login);
}

void SailPushClient::registerDevice(const QString &secret, const QString &deviceName)
{
    QUrlQuery query;
    query.addQueryItem("secret", secret);
    query.addQueryItem("name", deviceName);
    query.addQueryItem("os", "O");
    sendRequest(QUrl(registerUrl()), query.toString(QUrl::FullyEncoded).toUtf8(), Action::Register);
}

void SailPushClient::downloadMessages(const QString &secret, const QString &deviceId)
{
    sendRequest(QUrl(messagesUrl(secret, deviceId)), QByteArray(), Action::Download);
}

void SailPushClient::deleteMessages(const QString &secret, const QString &deviceId, const QString &highestMessageId)
{
    QUrlQuery query;
    query.addQueryItem("secret", secret);
    query.addQueryItem("message", highestMessageId);
    sendRequest(QUrl(deleteUrl(deviceId)), query.toString(QUrl::FullyEncoded).toUtf8(), Action::Delete);
}

void SailPushClient::acknowledgeEmergency(const QString &secret, const QString &receipt)
{
    QUrlQuery query;
    query.addQueryItem("secret", secret);
    sendRequest(QUrl(acknowledgeUrl(receipt)), query.toString(QUrl::FullyEncoded).toUtf8(), Action::Acknowledge);
}

void SailPushClient::onReplyFinished(QNetworkReply *reply)
{
    Action action = static_cast<Action>(reply->property("action").toInt());
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    reply->deleteLater();

    // Check for auth failures before generic error handling — Qt treats HTTP 4xx as NoError
    if ((httpStatus == 401 || httpStatus == 403) && (action == Action::Download || action == Action::Delete || action == Action::Acknowledge)) {
        qCWarning(lcSailPushClient) << "HTTP" << httpStatus << "for action" << static_cast<int>(action) << "— credentials rejected";
        emitError(action, QStringLiteral("HTTP %1: credentials rejected").arg(httpStatus));
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(lcSailPushClient) << "Network error:" << reply->errorString();
        emitError(action, reply->errorString());
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(lcSailPushClient) << "JSON parse error:" << parseError.errorString();
        emitError(action, tr("Invalid server response"));
        return;
    }

    QJsonObject root = doc.object();
    int status = root.value("status").toInt(0);

    if (action == Action::Login) {
        if (httpStatus == 412) {
            emit twoFactorRequired();
            return;
        }
        if (status == 1) {
            emit loginSuccess(root.value("id").toString(), root.value("secret").toString());
        } else {
            QString errorStr = extractErrorMessage(root);
            emit loginFailed(errorStr.isEmpty() ? tr("Login failed") : errorStr);
        }
    } else if (action == Action::Register) {
        if (status == 1) {
            emit deviceRegistered(root.value("id").toString());
        } else {
            QString errorStr = extractErrorMessage(root);
            emit deviceRegistrationFailed(errorStr.isEmpty() ? tr("Registration failed") : errorStr);
        }
    } else if (action == Action::Download) {
        if (status == 1) {
            QList<Message> messages;
            QJsonArray msgs = root.value("messages").toArray();
            for (const QJsonValue &v : msgs) {
                messages.append(Message::fromJson(v.toObject()));
            }
            emit messagesDownloaded(messages);
        } else {
            QString errorStr = extractErrorMessage(root);
            emit messagesDownloadFailed(errorStr.isEmpty() ? tr("Download failed") : errorStr);
        }
    } else if (action == Action::Delete) {
        if (status == 1) {
            emit messagesDeleted();
        } else {
            QString errorStr = extractErrorMessage(root);
            emit messageDeleteFailed(errorStr.isEmpty() ? tr("Delete failed") : errorStr);
        }
    } else if (action == Action::Acknowledge) {
        if (status == 1) {
            emit emergencyAcknowledged();
        } else {
            QString errorStr = extractErrorMessage(root);
            emit emergencyAckFailed(errorStr.isEmpty() ? tr("Acknowledge failed") : errorStr);
        }
    }
}

QNetworkReply* SailPushClient::sendRequest(const QUrl &url, const QByteArray &postData, Action action)
{
    QNetworkRequest request(url);
    if (!postData.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    }
    request.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT);

    QNetworkReply *reply = postData.isEmpty()
        ? m_networkManager->get(request)
        : m_networkManager->post(request, postData);
    QTimer::singleShot(REQUEST_TIMEOUT_MS, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    reply->setProperty("action", static_cast<int>(action));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
    return reply;
}

void SailPushClient::emitError(Action action, const QString &message)
{
    switch (action) {
    case Action::Login:       emit loginFailed(message); break;
    case Action::Register:    emit deviceRegistrationFailed(message); break;
    case Action::Download:    emit messagesDownloadFailed(message); break;
    case Action::Delete:      emit messageDeleteFailed(message); break;
    case Action::Acknowledge: emit emergencyAckFailed(message); break;
    }
}

QString SailPushClient::extractErrorMessage(const QJsonObject &root)
{
    QJsonArray errors = root.value("errors").toArray();
    QString errorStr;
    for (const QJsonValue &e : errors) {
        if (!errorStr.isEmpty()) errorStr += "; ";
        errorStr += e.toString();
    }
    return errorStr;
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

