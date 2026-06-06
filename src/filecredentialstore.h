#ifndef FILECREDENTIALSTORE_H
#define FILECREDENTIALSTORE_H

#include "icredentialstore.h"

class FileCredentialStore : public ICredentialStore {
    Q_OBJECT

public:
    explicit FileCredentialStore(const QString &dataPath, QObject *parent = nullptr);
    ~FileCredentialStore() override;

    bool save(const QString &secret, const QString &deviceId, const QString &userKey, const QString &deviceName) override;
    bool load(QString &secret, QString &deviceId, QString &userKey, QString &deviceName) override;
    bool clear() override;
    bool hasCredentials() const override;
    LoadError lastError() const override { return m_lastError; }

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

#endif // FILECREDENTIALSTORE_H
