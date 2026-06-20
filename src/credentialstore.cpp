#include "credentialstore.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QByteArray>

#include <Sailfish/Secrets/createcollectionrequest.h>
#include <Sailfish/Secrets/storesecretrequest.h>
#include <Sailfish/Secrets/storedsecretrequest.h>
#include <Sailfish/Secrets/deletesecretrequest.h>
#include <Sailfish/Secrets/result.h>

Q_LOGGING_CATEGORY(lcCredentialStore, "com.zackslash.sailpush.credentials")

const QString CredentialStore::COLLECTION_NAME = QStringLiteral("SailPushCredentials");
const QString CredentialStore::SECRET_NAME = QStringLiteral("credentials");

CredentialStore::CredentialStore(const QString &dataPath, QObject *parent)
    : ICredentialStore(parent)
    , m_lastError(LoadError::None)
    , m_hasCreds(false)
    , m_collectionReady(false)
{
    Q_UNUSED(dataPath)

    if (!m_manager.isInitialized()) {
        qCWarning(lcCredentialStore) << "sailfish-secrets daemon not available";
        m_lastError = LoadError::BackendUnavailable;
        return;
    }

    if (!ensureCollection()) {
        qCWarning(lcCredentialStore) << "Failed to ensure collection exists";
        return;
    }

    // Check if credentials already exist
    m_hasCreds = checkSecretExists();
}

CredentialStore::~CredentialStore()
{
}

bool CredentialStore::ensureCollection()
{
    if (m_collectionReady) {
        return true;
    }

    Sailfish::Secrets::CreateCollectionRequest ccr;
    ccr.setManager(&m_manager);
    ccr.setCollectionName(COLLECTION_NAME);
    ccr.setAccessControlMode(Sailfish::Secrets::SecretManager::NoAccessControlMode);
    ccr.setCollectionLockType(Sailfish::Secrets::CreateCollectionRequest::DeviceLock);
    ccr.setDeviceLockUnlockSemantic(Sailfish::Secrets::SecretManager::DeviceLockKeepUnlocked);
    ccr.setStoragePluginName(Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    ccr.setEncryptionPluginName(Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    ccr.startRequest();
    ccr.waitForFinished();

    if (ccr.result().errorCode() == Sailfish::Secrets::Result::NoError) {
        m_collectionReady = true;
        qCInfo(lcCredentialStore) << "Collection created:" << COLLECTION_NAME;
        return true;
    }

    // Already exists — that's fine
    if (ccr.result().errorCode() == Sailfish::Secrets::Result::CollectionAlreadyExistsError) {
        m_collectionReady = true;
        qCInfo(lcCredentialStore) << "Collection already exists:" << COLLECTION_NAME;
        return true;
    }

    qCWarning(lcCredentialStore) << "Collection creation failed: code=" << ccr.result().errorCode()
                                 << "msg=" << ccr.result().errorMessage();
    m_lastError = LoadError::BackendUnavailable;
    return false;
}

bool CredentialStore::checkSecretExists()
{
    Sailfish::Secrets::Secret::Identifier ident(SECRET_NAME, COLLECTION_NAME,
                                                 Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    Sailfish::Secrets::StoredSecretRequest gsr;
    gsr.setManager(&m_manager);
    gsr.setIdentifier(ident);
    gsr.setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
    gsr.startRequest();
    gsr.waitForFinished();

    return gsr.result().errorCode() == Sailfish::Secrets::Result::NoError;
}

bool CredentialStore::save(const QString &secret, const QString &deviceId)
{
    if (!m_manager.isInitialized()) {
        m_lastError = LoadError::BackendUnavailable;
        return false;
    }

    if (!ensureCollection()) {
        return false;
    }

    // Serialize credentials to JSON
    QJsonObject obj;
    obj.insert("secret", secret);
    obj.insert("device_id", deviceId);
    QByteArray jsonData = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Delete existing secret first (save() must succeed even if secret already exists)
    Sailfish::Secrets::Secret::Identifier ident(SECRET_NAME, COLLECTION_NAME,
                                                 Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    Sailfish::Secrets::DeleteSecretRequest dsr;
    dsr.setManager(&m_manager);
    dsr.setIdentifier(ident);
    dsr.setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
    dsr.startRequest();
    dsr.waitForFinished();
    // Ignore result — secret may not exist yet

    // Create secret object
    Sailfish::Secrets::Secret secretObj(ident);
    secretObj.setData(jsonData);
    secretObj.setType(Sailfish::Secrets::Secret::TypeBlob);

    // Store the secret
    Sailfish::Secrets::StoreSecretRequest ssr;
    ssr.setManager(&m_manager);
    ssr.setSecretStorageType(Sailfish::Secrets::StoreSecretRequest::CollectionSecret);
    ssr.setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
    ssr.setSecret(secretObj);
    ssr.startRequest();
    ssr.waitForFinished();

    // Zero the intermediate JSON data immediately (before error check)
    jsonData.fill(0);

    if (ssr.result().errorCode() != Sailfish::Secrets::Result::NoError) {
        qCWarning(lcCredentialStore) << "Failed to store secret: code=" << ssr.result().errorCode()
                                     << "msg=" << ssr.result().errorMessage();
        m_lastError = LoadError::BackendUnavailable;
        return false;
    }

    m_hasCreds = true;
    m_lastError = LoadError::None;
    qCInfo(lcCredentialStore) << "Credentials saved to sailfish-secrets";
    return true;
}

bool CredentialStore::load(QString &secret, QString &deviceId)
{
    m_lastError = LoadError::None;

    if (!m_manager.isInitialized()) {
        m_lastError = LoadError::BackendUnavailable;
        return false;
    }

    if (!ensureCollection()) {
        return false;
    }

    // Retrieve the secret
    Sailfish::Secrets::Secret::Identifier ident(SECRET_NAME, COLLECTION_NAME,
                                                 Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    Sailfish::Secrets::StoredSecretRequest gsr;
    gsr.setManager(&m_manager);
    gsr.setIdentifier(ident);
    gsr.setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
    gsr.startRequest();
    gsr.waitForFinished();

    if (gsr.result().errorCode() != Sailfish::Secrets::Result::NoError) {
        if (gsr.result().errorCode() == Sailfish::Secrets::Result::InvalidSecretError) {
            qCInfo(lcCredentialStore) << "Secret not found in sailfish-secrets";
        } else {
            qCWarning(lcCredentialStore) << "Failed to load secret: code=" << gsr.result().errorCode()
                                         << "msg=" << gsr.result().errorMessage();
        }
        m_lastError = LoadError::SecretNotFound;
        m_hasCreds = false;
        return false;
    }

    // Deserialize JSON
    QByteArray jsonData = gsr.secret().data();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    // Zero the intermediate data immediately
    jsonData.fill(0);

    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(lcCredentialStore) << "Failed to parse stored credentials:" << parseError.errorString();
        m_lastError = LoadError::InvalidFormat;
        m_hasCreds = false;
        return false;
    }

    QJsonObject obj = doc.object();
    secret = obj.value("secret").toString();
    deviceId = obj.value("device_id").toString();

    if (secret.isEmpty() || deviceId.isEmpty()) {
        qCWarning(lcCredentialStore) << "Stored credentials have empty required fields";
        m_lastError = LoadError::InvalidFormat;
        m_hasCreds = false;
        return false;
    }

    m_hasCreds = true;
    m_lastError = LoadError::None;
    qCInfo(lcCredentialStore) << "Credentials loaded from sailfish-secrets";
    return true;
}

bool CredentialStore::clear()
{
    if (!m_manager.isInitialized()) {
        m_lastError = LoadError::BackendUnavailable;
        return false;
    }

    Sailfish::Secrets::Secret::Identifier ident(SECRET_NAME, COLLECTION_NAME,
                                                 Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    Sailfish::Secrets::DeleteSecretRequest dsr;
    dsr.setManager(&m_manager);
    dsr.setIdentifier(ident);
    dsr.setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
    dsr.startRequest();
    dsr.waitForFinished();

    if (dsr.result().errorCode() != Sailfish::Secrets::Result::NoError) {
        // InvalidSecretError means secret doesn't exist — treat as success (already cleared)
        if (dsr.result().errorCode() != Sailfish::Secrets::Result::InvalidSecretError) {
            qCWarning(lcCredentialStore) << "Failed to delete secret:" << dsr.result().errorMessage();
            m_lastError = LoadError::BackendUnavailable;
            return false;
        }
    }

    m_hasCreds = false;
    m_lastError = LoadError::None;
    qCInfo(lcCredentialStore) << "Credentials cleared from sailfish-secrets";
    return true;
}

bool CredentialStore::hasCredentials() const
{
    return m_hasCreds;
}
