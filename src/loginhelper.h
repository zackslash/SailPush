#ifndef LOGINHELPER_H
#define LOGINHELPER_H

#include <QObject>
#include <QString>
#include "sailpushclient.h"
#include "credentialstore.h"

class LoginHelper : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool loggingIn READ loggingIn NOTIFY loggingInChanged)
    Q_PROPERTY(bool registering READ registering NOTIFY registeringChanged)
    Q_PROPERTY(bool needsTwoFactor READ needsTwoFactor NOTIFY needsTwoFactorChanged)
    Q_PROPERTY(bool hasCredentials READ hasCredentials NOTIFY credentialsChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString migrationReason READ migrationReason NOTIFY migrationReasonChanged)

public:
    explicit LoginHelper(QObject *parent = nullptr);

    bool loggingIn() const { return m_loggingIn; }
    bool registering() const { return m_registering; }
    bool needsTwoFactor() const { return m_needsTwoFactor; }
    bool hasCredentials() const { return m_store->hasCredentials(); }
    QString errorString() const { return m_errorString; }
    QString migrationReason() const { return m_migrationReason; }

    Q_INVOKABLE void login(const QString &email, const QString &password, const QString &twofa = QString());
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void setMigrationReason(const QString &reason);

signals:
    void loggingInChanged();
    void registeringChanged();
    void needsTwoFactorChanged();
    void credentialsChanged();
    void errorStringChanged();
    void migrationReasonChanged();
    void loginFailed(const QString &error);
    void credentialsSaved();

private slots:
    void onLoginSuccess(const QString &userKey, const QString &secret);
    void onLoginFailed(const QString &error);
    void onTwoFactorRequired();
    void onDeviceRegistered(const QString &deviceId);
    void onDeviceRegistrationFailed(const QString &error);

private:
    void setLoggingIn(bool value);
    void setRegistering(bool value);
    void setNeedsTwoFactor(bool value);
    void setErrorString(const QString &value);

    SailPushClient *m_client;
    CredentialStore *m_store;

    bool m_loggingIn;
    bool m_registering;
    bool m_needsTwoFactor;
    QString m_errorString;
    QString m_migrationReason;
    QString m_pendingUserKey;
    QString m_pendingSecret;
    QString m_pendingDeviceName;
};

#endif // LOGINHELPER_H
