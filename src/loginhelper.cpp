#include "loginhelper.h"
#include "daemon.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcLoginHelper, "com.zackslash.sailpush.login")

static QString generateDeviceName()
{
    return QStringLiteral("SailfishOS");
}

LoginHelper::LoginHelper(QObject *parent)
    : QObject(parent)
    , m_client(new SailPushClient(this))
    , m_store(new CredentialStore(Daemon::dataPath(), this))
    , m_loggingIn(false)
    , m_registering(false)
    , m_needsTwoFactor(false)
{
    QString secret, deviceId, userKey, deviceName;
    if (!m_store->load(secret, deviceId, userKey, deviceName)) {
        CredentialStore::LoadError err = m_store->lastError();
        if (err == CredentialStore::LoadError::OldFormat) {
            m_migrationReason = tr("App was upgraded — saved credentials use an older format and cannot be migrated. Please log in again.");
        } else if (err == CredentialStore::LoadError::DecryptionFailed) {
            m_migrationReason = tr("Saved credentials could not be decrypted. This can happen after a system update. Please log in again.");
        }
    }

    connect(m_client, &SailPushClient::loginSuccess, this, &LoginHelper::onLoginSuccess);
    connect(m_client, &SailPushClient::loginFailed, this, &LoginHelper::onLoginFailed);
    connect(m_client, &SailPushClient::twoFactorRequired, this, &LoginHelper::onTwoFactorRequired);
    connect(m_client, &SailPushClient::deviceRegistered, this, &LoginHelper::onDeviceRegistered);
    connect(m_client, &SailPushClient::deviceRegistrationFailed, this, &LoginHelper::onDeviceRegistrationFailed);
}

void LoginHelper::login(const QString &email, const QString &password, const QString &twofa)
{
    setLoggingIn(true);
    setErrorString(QString());
    setNeedsTwoFactor(false);

    qCInfo(lcLoginHelper) << "Attempting login for" << email;
    m_client->login(email, password, twofa);
}

void LoginHelper::cancel()
{
    setLoggingIn(false);
    setRegistering(false);
    setNeedsTwoFactor(false);
    setErrorString(QString());

    m_pendingUserKey.clear();
    m_pendingSecret.clear();
    m_pendingDeviceName.clear();
}

void LoginHelper::logout()
{
    qCInfo(lcLoginHelper) << "Logging out, clearing credentials";
    m_store->clear();
    emit credentialsChanged();
}

void LoginHelper::setMigrationReason(const QString &reason)
{
    if (m_migrationReason != reason) {
        m_migrationReason = reason;
        emit migrationReasonChanged();
    }
}

void LoginHelper::onLoginSuccess(const QString &userKey, const QString &secret)
{
    qCInfo(lcLoginHelper) << "Login successful, registering device";
    m_pendingUserKey = userKey;
    m_pendingSecret = secret;
    setLoggingIn(false);
    setRegistering(true);

    m_pendingDeviceName = generateDeviceName();
    m_client->registerDevice(secret, m_pendingDeviceName);
}

void LoginHelper::onLoginFailed(const QString &error)
{
    qCWarning(lcLoginHelper) << "Login failed:" << error;
    setLoggingIn(false);
    setErrorString(error);
    emit loginFailed(error);

}

void LoginHelper::onTwoFactorRequired()
{
    qCInfo(lcLoginHelper) << "Two-factor authentication required";
    setNeedsTwoFactor(true);
    setLoggingIn(false);
}

void LoginHelper::onDeviceRegistered(const QString &deviceId)
{
    qCInfo(lcLoginHelper) << "Device registered:" << deviceId;
    setRegistering(false);

    bool saved = m_store->save(m_pendingSecret, deviceId, m_pendingUserKey, m_pendingDeviceName);
    if (saved) {
        qCInfo(lcLoginHelper) << "Credentials saved successfully";
        if (!m_migrationReason.isEmpty()) {
            m_migrationReason.clear();
            emit migrationReasonChanged();
        }
        emit credentialsChanged();
        emit credentialsSaved();
    } else {
        setErrorString(tr("Failed to save credentials"));
        emit loginFailed(tr("Failed to save credentials"));
    }

    m_pendingUserKey.clear();
    m_pendingSecret.clear();
    m_pendingDeviceName.clear();
}

void LoginHelper::onDeviceRegistrationFailed(const QString &error)
{
    qCWarning(lcLoginHelper) << "Device registration failed:" << error;
    setRegistering(false);
    setErrorString(error);
    emit loginFailed(error);

    m_pendingUserKey.clear();
    m_pendingSecret.clear();
    m_pendingDeviceName.clear();
}

void LoginHelper::setLoggingIn(bool value)
{
    if (m_loggingIn != value) {
        m_loggingIn = value;
        emit loggingInChanged();
    }
}

void LoginHelper::setRegistering(bool value)
{
    if (m_registering != value) {
        m_registering = value;
        emit registeringChanged();
    }
}

void LoginHelper::setNeedsTwoFactor(bool value)
{
    if (m_needsTwoFactor != value) {
        m_needsTwoFactor = value;
        emit needsTwoFactorChanged();
    }
}

void LoginHelper::setErrorString(const QString &value)
{
    if (m_errorString != value) {
        m_errorString = value;
        emit errorStringChanged();
    }
}
