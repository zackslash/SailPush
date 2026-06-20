#ifndef ICREDENTIALSTORE_H
#define ICREDENTIALSTORE_H

#include <QObject>
#include <QString>

class ICredentialStore : public QObject {

public:
    enum class LoadError {
        None,
        SecretNotFound,
        InvalidFormat,
        DecryptionFailed,
        BackendUnavailable
    };

    explicit ICredentialStore(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ICredentialStore() = default;

    virtual bool save(const QString &secret, const QString &deviceId) = 0;
    virtual bool load(QString &secret, QString &deviceId) = 0;
    virtual bool clear() = 0;
    virtual bool hasCredentials() const = 0;
    virtual LoadError lastError() const = 0;
};

#endif // ICREDENTIALSTORE_H
