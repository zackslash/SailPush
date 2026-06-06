#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QObject>
#include <QString>

class CredentialStore : public QObject {
    Q_OBJECT

public:
    enum class LoadError {
        None,
        FileNotFound,
        OldFormat,
        DecryptionFailed
    };

    explicit CredentialStore(const QString &dataPath, QObject *parent = nullptr);
    ~CredentialStore() override;

    bool save(const QString &secret, const QString &deviceId, const QString &userKey, const QString &deviceName);
    bool load(QString &secret, QString &deviceId, QString &userKey, QString &deviceName);
    bool clear();
    bool hasCredentials() const;

    LoadError lastError() const { return m_lastError; }

private:
    QString storagePath() const;
    QByteArray encrypt(const QByteArray &data) const;
    QByteArray decrypt(const QByteArray &data) const;
    QByteArray deriveKey() const;
    QString machineId() const;

    QString m_dataPath;

    LoadError m_lastError;

    mutable QByteArray m_cachedKey;
    mutable bool m_keyCached;
};

#endif // CREDENTIALSTORE_H
