#pragma once
#include <QByteArray>
#include <QString>

namespace openbiounlock {
class CredentialStore {
public:
    bool write(const QString &name, const QByteArray &secret, QString *error = nullptr) const;
    QByteArray read(const QString &name, QString *error = nullptr) const;
    bool remove(const QString &name, QString *error = nullptr) const;
};
}