#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "credentialstore.h"

class TestCredentialStore : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testNoCredentialsInitially();
    void testSaveAndLoad();
    void testHasCredentials();
    void testClear();
    void testOverwrite();
    void testEmptyValues();
    void testLoadNonexistent();
    void testLastErrorNoneOnSuccess();
    void testLastErrorFileNotFound();
    void testLastErrorOldFormat();
    void testFilePermissionsAfterSave();

private:
    QTemporaryDir m_tempDir;
    CredentialStore *m_store;
};

void TestCredentialStore::init()
{
    // Clear any leftover credentials from previous test
    QFile::remove(m_tempDir.path() + "/credentials.json");
    m_store = new CredentialStore(m_tempDir.path(), this);
}

void TestCredentialStore::cleanup()
{
    delete m_store;
    m_store = nullptr;
}

void TestCredentialStore::testNoCredentialsInitially()
{
    QVERIFY(!m_store->hasCredentials());
}

void TestCredentialStore::testSaveAndLoad()
{
    QVERIFY(m_store->save("secret123", "device456", "user789", "MyDevice"));

    QString secret, deviceId, userKey, deviceName;
    QVERIFY(m_store->load(secret, deviceId, userKey, deviceName));

    QCOMPARE(secret, QString("secret123"));
    QCOMPARE(deviceId, QString("device456"));
    QCOMPARE(userKey, QString("user789"));
    QCOMPARE(deviceName, QString("MyDevice"));
}

void TestCredentialStore::testHasCredentials()
{
    QVERIFY(!m_store->hasCredentials());

    m_store->save("secret", "device", "user", "name");
    QVERIFY(m_store->hasCredentials());
}

void TestCredentialStore::testClear()
{
    m_store->save("secret", "device", "user", "name");
    QVERIFY(m_store->hasCredentials());

    QVERIFY(m_store->clear());
    QVERIFY(!m_store->hasCredentials());
}

void TestCredentialStore::testOverwrite()
{
    m_store->save("secret1", "device1", "user1", "name1");

    QString secret, deviceId, userKey, deviceName;
    m_store->load(secret, deviceId, userKey, deviceName);
    QCOMPARE(secret, QString("secret1"));

    m_store->save("secret2", "device2", "user2", "name2");
    m_store->load(secret, deviceId, userKey, deviceName);
    QCOMPARE(secret, QString("secret2"));
    QCOMPARE(deviceId, QString("device2"));
}

void TestCredentialStore::testEmptyValues()
{
    // Empty strings cannot be encrypted — save succeeds but load will fail
    // because HMAC verification fails on empty ciphertext
    m_store->save("", "", "", "");

    QString secret, deviceId, userKey, deviceName;
    QVERIFY(!m_store->load(secret, deviceId, userKey, deviceName));
}

void TestCredentialStore::testLoadNonexistent()
{
    CredentialStore *emptyStore = new CredentialStore("/nonexistent/path/that/does/not/exist", this);
    QString secret, deviceId, userKey, deviceName;
    QVERIFY(!emptyStore->load(secret, deviceId, userKey, deviceName));
    QCOMPARE(emptyStore->lastError(), CredentialStore::LoadError::FileNotFound);
    delete emptyStore;
}

void TestCredentialStore::testLastErrorNoneOnSuccess()
{
    m_store->save("secret", "device", "user", "name");

    QString secret, deviceId, userKey, deviceName;
    QVERIFY(m_store->load(secret, deviceId, userKey, deviceName));
    QCOMPARE(m_store->lastError(), CredentialStore::LoadError::None);
}

void TestCredentialStore::testLastErrorFileNotFound()
{
    QString secret, deviceId, userKey, deviceName;
    QVERIFY(!m_store->load(secret, deviceId, userKey, deviceName));
    QCOMPARE(m_store->lastError(), CredentialStore::LoadError::FileNotFound);
}

void TestCredentialStore::testLastErrorOldFormat()
{
    // Write a v1-format credentials file (version < 2)
    QJsonObject obj;
    obj.insert("version", 1);
    obj.insert("secret", "encrypted");
    obj.insert("device_id", "encrypted");
    obj.insert("user_key", "encrypted");
    obj.insert("device_name", "encrypted");
    QJsonDocument doc(obj);

    QString path = m_tempDir.path() + "/credentials.json";
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();

    QString secret, deviceId, userKey, deviceName;
    QVERIFY(!m_store->load(secret, deviceId, userKey, deviceName));
    QCOMPARE(m_store->lastError(), CredentialStore::LoadError::OldFormat);
}

void TestCredentialStore::testFilePermissionsAfterSave()
{
    m_store->save("secret", "device", "user", "name");
    QFile::Permissions perms = QFile::permissions(m_tempDir.path() + "/credentials.json");
    // Qt expands ReadOwner/WriteOwner to include ReadUser/WriteUser in the getter
    QFile::Permissions expected = QFile::ReadOwner | QFile::WriteOwner
                                | QFile::ReadUser | QFile::WriteUser;
    QCOMPARE(perms, expected);
}

QTEST_MAIN(TestCredentialStore)
#include "tst_credentialstore.moc"
