#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include "icredentialstore.h"
#include <Sailfish/Secrets/secretmanager.h>
#include <Sailfish/Secrets/secret.h>

class CredentialStore : public ICredentialStore {
    Q_OBJECT

public:
    explicit CredentialStore(const QString &dataPath, QObject *parent = nullptr);
    ~CredentialStore() override;

    bool save(const QString &secret, const QString &deviceId) override;
    bool load(QString &secret, QString &deviceId) override;
    bool clear() override;
    bool hasCredentials() const override;
    LoadError lastError() const override { return m_lastError; }

private:
    bool ensureCollection();
    bool checkSecretExists();

    static const QString COLLECTION_NAME;
    static const QString SECRET_NAME;

    Sailfish::Secrets::SecretManager m_manager;
    LoadError m_lastError;
    bool m_hasCreds;
    bool m_collectionReady;
};

#endif // CREDENTIALSTORE_H
