#include "loginhelper.h"
#include "credentialstore.h"
#include "daemon.h"
#include <QLoggingCategory>
#include <QtConcurrent>
#include <QFutureWatcher>

Q_LOGGING_CATEGORY(lcLoginHelper, "com.zackslash.sailpush.login")

namespace {
struct LoadResult {
    bool hasCreds = false;
    ICredentialStore::LoadError error = ICredentialStore::LoadError::None;
};

QString generateDeviceName()
{
    return QStringLiteral("SailfishOS");
}
} // namespace

LoginHelper::LoginHelper(QObject *parent)
    : QObject(parent)
    , m_client(new SailPushClient(this))
    , m_loggingIn(false)
    , m_registering(false)
    , m_needsTwoFactor(false)
    , m_hasCredentials(false)
    , m_credentialsLoading(true)
{
    connect(m_client, &SailPushClient::loginSuccess, this, &LoginHelper::onLoginSuccess);
    connect(m_client, &SailPushClient::loginFailed, this, &LoginHelper::onLoginFailed);
    connect(m_client, &SailPushClient::twoFactorRequired, this, &LoginHelper::onTwoFactorRequired);
    connect(m_client, &SailPushClient::deviceRegistered, this, &LoginHelper::onDeviceRegistered);
    connect(m_client, &SailPushClient::deviceRegistrationFailed, this, &LoginHelper::onDeviceRegistrationFailed);

    loadCredentialsAsync();
}

void LoginHelper::loadCredentialsAsync()
{
    qCInfo(lcLoginHelper) << "Loading credentials asynchronously";
    const QString dataPath = Daemon::dataPath();

    auto *watcher = new QFutureWatcher<LoadResult>(this);
    connect(watcher, &QFutureWatcher<LoadResult>::finished, this, [this, watcher]() {
        const LoadResult result = watcher->result();
        watcher->deleteLater();

        m_hasCredentials = result.hasCreds;

        if (!result.hasCreds) {
            const auto err = result.error;
            if (err == ICredentialStore::LoadError::InvalidFormat) {
                m_migrationReason = tr("App was upgraded — saved credentials use an older format and cannot be migrated. Please log in again.");
            } else if (err == ICredentialStore::LoadError::DecryptionFailed) {
                m_migrationReason = tr("Saved credentials could not be decrypted. This can happen after a system update. Please log in again.");
            } else if (err == ICredentialStore::LoadError::BackendUnavailable) {
                m_migrationReason = tr("Secrets service is not available. Please restart the device and try again.");
            }
            if (!m_migrationReason.isEmpty()) {
                emit migrationReasonChanged();
            }
        }

        // Emit credentialsChanged BEFORE flipping credentialsLoading so QML's
        // onHasCredentialsChanged guard (!credentialsLoading) skips the page
        // replace; the actual initial page push happens in
        // onCredentialsLoadingChanged → pushInitialPage().
        emit credentialsChanged();
        setCredentialsLoading(false);
    });

    watcher->setFuture(QtConcurrent::run([dataPath]() {
        LoadResult r;
        // CredentialStore is created and destroyed entirely on this worker
        // thread. waitForFinished() spins its own local QEventLoop (via
        // QDBusPendingCall) so no pre-existing event loop is needed. The UI
        // thread stays responsive throughout.
        CredentialStore store(dataPath);
        QString secret, deviceId;
        r.hasCreds = store.load(secret, deviceId);
        r.error = store.lastError();
        return r;
    }));
}

void LoginHelper::login(const QString &email, const QString &password, const QString &twofa)
{
    setLoggingIn(true);
    setErrorString(QString());
    setNeedsTwoFactor(false);

    qCInfo(lcLoginHelper) << "Attempting login";
    m_client->login(email, password, twofa);
}

void LoginHelper::cancel()
{
    setLoggingIn(false);
    setRegistering(false);
    setNeedsTwoFactor(false);
    setErrorString(QString());

    m_pendingSecret.clear();
}

void LoginHelper::logout()
{
    qCInfo(lcLoginHelper) << "Logging out, clearing credentials";
    // Optimistic UI update — flip immediately so the user sees feedback.
    setHasCredentials(false);

    // Fire-and-forget the blocking clear() on a worker thread.
    const QString dataPath = Daemon::dataPath();
    QtConcurrent::run([dataPath]() {
        CredentialStore store(dataPath);
        store.clear();
    });
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
    Q_UNUSED(userKey)
    m_pendingSecret = secret;
    setLoggingIn(false);
    setRegistering(true);

    m_client->registerDevice(secret, generateDeviceName());
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

    // Save credentials on a worker thread to avoid blocking the UI.
    const QString secret = m_pendingSecret;
    m_pendingSecret.clear();
    const QString dataPath = Daemon::dataPath();

    auto *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
        const bool saved = watcher->result();
        watcher->deleteLater();

        if (saved) {
            qCInfo(lcLoginHelper) << "Credentials saved successfully";
            if (!m_migrationReason.isEmpty()) {
                m_migrationReason.clear();
                emit migrationReasonChanged();
            }
            setHasCredentials(true);
            emit credentialsSaved();
        } else {
            setErrorString(tr("Failed to save credentials"));
            emit loginFailed(tr("Failed to save credentials"));
        }
    });

    watcher->setFuture(QtConcurrent::run([dataPath, secret, deviceId]() {
        CredentialStore store(dataPath);
        return store.save(secret, deviceId);
    }));
}

void LoginHelper::onDeviceRegistrationFailed(const QString &error)
{
    qCWarning(lcLoginHelper) << "Device registration failed:" << error;
    setRegistering(false);
    setErrorString(error);
    emit loginFailed(error);

    m_pendingSecret.clear();
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

void LoginHelper::setHasCredentials(bool value)
{
    if (m_hasCredentials != value) {
        m_hasCredentials = value;
        emit credentialsChanged();
    }
}

void LoginHelper::setCredentialsLoading(bool value)
{
    if (m_credentialsLoading != value) {
        m_credentialsLoading = value;
        emit credentialsLoadingChanged();
    }
}
