#include "filecredentialstore.h"
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

Q_LOGGING_CATEGORY(lcFileCredentialStore, "com.zackslash.sailpush.filecredentials")

static const QString CREDENTIALS_FILE = QStringLiteral("credentials.json");
static const int IV_SIZE = 16;

FileCredentialStore::FileCredentialStore(const QString &dataPath, QObject *parent)
    : ICredentialStore(parent)
    , m_dataPath(dataPath)
    , m_lastError(LoadError::None)
    , m_keyCached(false)
{
}

FileCredentialStore::~FileCredentialStore()
{
    m_cachedKey.fill(0);
}

bool FileCredentialStore::save(const QString &secret, const QString &deviceId, const QString &userKey, const QString &deviceName)
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
            qCWarning(lcFileCredentialStore) << "Failed to create directory:" << dir.path();
            return false;
        }
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcFileCredentialStore) << "Failed to save credentials:" << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();

    if (!QFile::setPermissions(storagePath(), QFile::ReadOwner | QFile::WriteOwner)) {
        qCWarning(lcFileCredentialStore) << "Failed to set file permissions";
    }

    qCInfo(lcFileCredentialStore) << "Credentials saved";
    return true;
}

bool FileCredentialStore::load(QString &secret, QString &deviceId, QString &userKey, QString &deviceName)
{
    m_lastError = LoadError::None;

    QFile file(storagePath());
    if (!file.exists()) {
        qCInfo(lcFileCredentialStore) << "No credentials file found";
        m_lastError = LoadError::SecretNotFound;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcFileCredentialStore) << "Failed to open credentials:" << file.errorString();
        m_lastError = LoadError::DecryptionFailed;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(lcFileCredentialStore) << "Failed to parse credentials:" << parseError.errorString();
        m_lastError = LoadError::DecryptionFailed;
        return false;
    }

    QJsonObject obj = doc.object();

    int version = obj.value("version").toInt(0);
    if (version < 2) {
        qCWarning(lcFileCredentialStore) << "Credentials encrypted with old format (v" << version << "), need re-login";
        m_lastError = LoadError::InvalidFormat;
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
        qCWarning(lcFileCredentialStore) << "Failed to decrypt some credential fields";
        m_lastError = LoadError::DecryptionFailed;
        return false;
    }

    m_lastError = LoadError::None;
    qCInfo(lcFileCredentialStore) << "Credentials loaded";
    return true;
}

bool FileCredentialStore::clear()
{
    QFile file(storagePath());
    if (file.exists()) {
        if (!file.remove()) {
            qCWarning(lcFileCredentialStore) << "Failed to remove credentials:" << file.errorString();
            return false;
        }
    }

    qCInfo(lcFileCredentialStore) << "Credentials cleared";
    return true;
}

bool FileCredentialStore::hasCredentials() const
{
    return QFile::exists(storagePath());
}

QString FileCredentialStore::storagePath() const
{
    return QDir(m_dataPath).filePath(CREDENTIALS_FILE);
}

QString FileCredentialStore::machineId() const
{
    QFile f(QStringLiteral("/etc/machine-id"));
    if (f.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(f.readAll()).trimmed();
    }
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

QByteArray FileCredentialStore::deriveKey() const
{
    if (!m_keyCached) {
        QString seed = machineId() + m_dataPath + QStringLiteral("sailpush-sailfish-key");
        m_cachedKey = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha256);
        m_keyCached = true;
    }
    return m_cachedKey;
}

QByteArray FileCredentialStore::encrypt(const QByteArray &data) const
{
    QByteArray iv = QUuid::createUuid().toRfc4122().left(IV_SIZE);
    QByteArray key = deriveKey();

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

    QByteArray encrypted;
    encrypted.reserve(data.size());
    for (int i = 0; i < data.size(); ++i) {
        encrypted.append(data[i] ^ keystream[i]);
    }

    QByteArray mac = QMessageAuthenticationCode::hash(iv + encrypted, key, QCryptographicHash::Sha256);
    QByteArray result = iv + encrypted + mac;
    return result.toBase64();
}

QByteArray FileCredentialStore::decrypt(const QByteArray &data) const
{
    QByteArray decoded = QByteArray::fromBase64(data);
    if (decoded.size() < IV_SIZE + 1 + 32) {
        return QByteArray();
    }

    QByteArray iv = decoded.left(IV_SIZE);
    QByteArray mac = decoded.right(32);
    QByteArray ciphertext = decoded.mid(IV_SIZE, decoded.size() - IV_SIZE - 32);
    QByteArray key = deriveKey();

    QByteArray expectedMac = QMessageAuthenticationCode::hash(iv + ciphertext, key, QCryptographicHash::Sha256);
    if (mac != expectedMac) {
        qCWarning(lcFileCredentialStore) << "HMAC verification failed — credentials may be tampered";
        return QByteArray();
    }

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

    QByteArray decrypted;
    decrypted.reserve(ciphertext.size());
    for (int i = 0; i < ciphertext.size(); ++i) {
        decrypted.append(ciphertext[i] ^ keystream[i]);
    }
    return decrypted;
}
