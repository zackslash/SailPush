#include "credentialstore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QUuid>

Q_LOGGING_CATEGORY(lcCredentialStore, "net.sailpush.sailfish.credentials")

static const QString CREDENTIALS_FILE = QStringLiteral("credentials.json");
static const int IV_SIZE = 16;

CredentialStore::CredentialStore(const QString &dataPath, QObject *parent)
    : QObject(parent)
    , m_dataPath(dataPath)
    , m_lastError(LoadError::None)
    , m_keyCached(false)
{
}

bool CredentialStore::save(const QString &secret, const QString &deviceId, const QString &userKey, const QString &deviceName)
{
    QJsonObject obj;
    obj.insert("version", 2);
    obj.insert("secret", QString::fromUtf8(encrypt(secret.toUtf8())));
    obj.insert("device_id", QString::fromUtf8(encrypt(deviceId.toUtf8())));
    obj.insert("user_key", QString::fromUtf8(encrypt(userKey.toUtf8())));
    obj.insert("device_name", QString::fromUtf8(encrypt(deviceName.toUtf8())));

    QJsonDocument doc(obj);
    QFile file(storagePath());
    QDir dir = QFileInfo(file).dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCWarning(lcCredentialStore) << "Failed to create directory:" << dir.path();
            return false;
        }
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcCredentialStore) << "Failed to save credentials:" << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();

    qCInfo(lcCredentialStore) << "Credentials saved";
    return true;
}

bool CredentialStore::load(QString &secret, QString &deviceId, QString &userKey, QString &deviceName)
{
    m_lastError = LoadError::None;

    QFile file(storagePath());
    if (!file.exists()) {
        qCInfo(lcCredentialStore) << "No credentials file found";
        m_lastError = LoadError::FileNotFound;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcCredentialStore) << "Failed to open credentials:" << file.errorString();
        m_lastError = LoadError::DecryptionFailed;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(lcCredentialStore) << "Failed to parse credentials:" << parseError.errorString();
        m_lastError = LoadError::DecryptionFailed;
        return false;
    }

    QJsonObject obj = doc.object();

    // Check encryption version — v1 used XOR with different key derivation,
    // v2 uses HMAC-SHA256 counter mode with machine-id-bound key.
    // Old credentials cannot be decrypted with the new scheme.
    int version = obj.value("version").toInt(0);
    if (version < 2) {
        qCWarning(lcCredentialStore) << "Credentials encrypted with old format (v" << version << "), need re-login";
        m_lastError = LoadError::OldFormat;
        return false;
    }

    bool ok = true;
    auto decryptField = [&](const QString &key) -> QString {
        QByteArray encrypted = obj.value(key).toString().toUtf8();
        QByteArray decrypted = decrypt(encrypted);
        if (decrypted.isEmpty() && !encrypted.isEmpty()) {
            ok = false;
        }
        return QString::fromUtf8(decrypted);
    };

    secret = decryptField("secret");
    deviceId = decryptField("device_id");
    userKey = decryptField("user_key");
    deviceName = decryptField("device_name");

    if (!ok) {
        qCWarning(lcCredentialStore) << "Failed to decrypt some credential fields";
        m_lastError = LoadError::DecryptionFailed;
        return false;
    }

    m_lastError = LoadError::None;
    qCInfo(lcCredentialStore) << "Credentials loaded";
    return true;
}

bool CredentialStore::clear()
{
    QFile file(storagePath());
    if (file.exists()) {
        if (!file.remove()) {
            qCWarning(lcCredentialStore) << "Failed to remove credentials:" << file.errorString();
            return false;
        }
    }

    qCInfo(lcCredentialStore) << "Credentials cleared";
    return true;
}

bool CredentialStore::hasCredentials() const
{
    return QFile::exists(storagePath());
}

QString CredentialStore::storagePath() const
{
    return QDir(m_dataPath).filePath(CREDENTIALS_FILE);
}

QString CredentialStore::machineId() const
{
    QFile f(QStringLiteral("/etc/machine-id"));
    if (f.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(f.readAll()).trimmed();
    }
    // Fallback: generate a per-install unique ID stored alongside credentials
    QString fallbackPath = QDir(m_dataPath).filePath(".machine-id");
    QFile fallback(fallbackPath);
    if (fallback.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(fallback.readAll()).trimmed();
    }
    QString id = QUuid::createUuid().toString();
    if (fallback.open(QIODevice::WriteOnly)) {
        fallback.write(id.toUtf8());
        fallback.close();
    }
    return id;
}

QByteArray CredentialStore::deriveKey() const
{
    if (!m_keyCached) {
        // Derive key from machine-id (device-specific) + data path + app secret
        // This ensures credentials are bound to this device and app installation
        QString seed = machineId() + m_dataPath + QStringLiteral("sailpush-sailfish-key");
        m_cachedKey = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha256);
        m_keyCached = true;
    }
    return m_cachedKey;
}

QByteArray CredentialStore::encrypt(const QByteArray &data) const
{
    // Generate random IV for each encryption
    QByteArray iv = QUuid::createUuid().toRfc4122().left(IV_SIZE);
    QByteArray key = deriveKey();

    // HMAC-SHA256 counter mode keystream generation
    QByteArray keystream;
    keystream.reserve(data.size());
    int counter = 0;
    while (keystream.size() < data.size()) {
        QByteArray counterBytes = QByteArray::number(counter, 16).rightJustified(8, '0');
        QByteArray hmacInput = iv + counterBytes;
        QByteArray block = QMessageAuthenticationCode::hash(hmacInput, key, QCryptographicHash::Sha256);
        keystream.append(block);
        counter++;
    }

    // XOR data with keystream
    QByteArray encrypted;
    encrypted.reserve(data.size());
    for (int i = 0; i < data.size(); ++i) {
        encrypted.append(data[i] ^ keystream[i]);
    }

    // Compute HMAC of (IV + ciphertext) for authentication
    QByteArray mac = QMessageAuthenticationCode::hash(iv + encrypted, key, QCryptographicHash::Sha256);
    // Prepend IV, append MAC, then base64 encode
    QByteArray result = iv + encrypted + mac;
    return result.toBase64();
}

QByteArray CredentialStore::decrypt(const QByteArray &data) const
{
    QByteArray decoded = QByteArray::fromBase64(data);
    // IV (16) + ciphertext (>=1) + HMAC (32) = minimum 49 bytes
    if (decoded.size() < IV_SIZE + 1 + 32) {
        return QByteArray();
    }

    // Extract IV, ciphertext, and MAC
    QByteArray iv = decoded.left(IV_SIZE);
    QByteArray mac = decoded.right(32);
    QByteArray ciphertext = decoded.mid(IV_SIZE, decoded.size() - IV_SIZE - 32);
    QByteArray key = deriveKey();

    // Verify HMAC before decryption
    QByteArray expectedMac = QMessageAuthenticationCode::hash(iv + ciphertext, key, QCryptographicHash::Sha256);
    if (mac != expectedMac) {
        qCWarning(lcCredentialStore) << "HMAC verification failed — credentials may be tampered";
        return QByteArray();
    }

    // Regenerate the same keystream
    QByteArray keystream;
    keystream.reserve(ciphertext.size());
    int counter = 0;
    while (keystream.size() < ciphertext.size()) {
        QByteArray counterBytes = QByteArray::number(counter, 16).rightJustified(8, '0');
        QByteArray hmacInput = iv + counterBytes;
        QByteArray block = QMessageAuthenticationCode::hash(hmacInput, key, QCryptographicHash::Sha256);
        keystream.append(block);
        counter++;
    }

    // XOR ciphertext with keystream
    QByteArray decrypted;
    decrypted.reserve(ciphertext.size());
    for (int i = 0; i < ciphertext.size(); ++i) {
        decrypted.append(ciphertext[i] ^ keystream[i]);
    }
    return decrypted;
}
