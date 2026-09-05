#include "credential.hpp"
#include <QSettings>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#pragma comment(lib, "Crypt32.lib")
#elif defined(OPENBIOUNLOCK_HAS_LIBSECRET)
#include <libsecret/secret.h>
#endif

namespace openbiounlock {
namespace { void fail(QString *error, const QString &message) { if (error) *error = message; } }
bool CredentialStore::write(const QString &name, const QByteArray &secret, QString *error) const {
#ifdef Q_OS_WIN
    DATA_BLOB input{static_cast<DWORD>(secret.size()), reinterpret_cast<BYTE *>(const_cast<char *>(secret.constData()))}, output{};
    if (!CryptProtectData(&input, reinterpret_cast<LPCWSTR>(name.utf16()), nullptr, nullptr, nullptr, CRYPTPROTECT_LOCAL_MACHINE, &output)) { fail(error, QStringLiteral("Windows DPAPI encryption failed")); return false; }
    QByteArray encoded(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData)); LocalFree(output.pbData); QSettings settings(QStringLiteral("OpenBioUnlock"), QStringLiteral("Desktop")); settings.setValue(name, encoded.toBase64()); return true;
#elif defined(OPENBIOUNLOCK_HAS_LIBSECRET)
    static const SecretSchema schema = {"com.openbiounlock.Credential", SECRET_SCHEMA_NONE, {{"name", SECRET_SCHEMA_ATTRIBUTE_STRING}, {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}}}; GError *localError = nullptr; const QByteArray encoded = secret.toBase64(); const bool ok = secret_password_store_sync(&schema, SECRET_COLLECTION_DEFAULT, name.toUtf8().constData(), encoded.constData(), nullptr, &localError, "name", name.toUtf8().constData(), nullptr); if (!ok) { fail(error, localError ? QString::fromUtf8(localError->message) : QStringLiteral("Secret Service write failed")); if (localError) g_error_free(localError); } return ok;
#else
    Q_UNUSED(name); Q_UNUSED(secret); fail(error, QStringLiteral("no supported OS credential vault is available")); return false;
#endif
}
QByteArray CredentialStore::read(const QString &name, QString *error) const {
#ifdef Q_OS_WIN
    const QByteArray encoded = QSettings(QStringLiteral("OpenBioUnlock"), QStringLiteral("Desktop")).value(name).toByteArray(); DATA_BLOB input{static_cast<DWORD>(encoded.size()), reinterpret_cast<BYTE *>(const_cast<char *>(encoded.constData()))}, output{}; if (encoded.isEmpty() || !CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) { fail(error, QStringLiteral("Windows DPAPI decryption failed")); return {}; } QByteArray result(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData)); LocalFree(output.pbData); return result;
#elif defined(OPENBIOUNLOCK_HAS_LIBSECRET)
    static const SecretSchema schema = {"com.openbiounlock.Credential", SECRET_SCHEMA_NONE, {{"name", SECRET_SCHEMA_ATTRIBUTE_STRING}, {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}}}; GError *localError = nullptr; gchar *value = secret_password_lookup_sync(&schema, nullptr, &localError, "name", name.toUtf8().constData(), nullptr); if (!value) { if (localError) { fail(error, QString::fromUtf8(localError->message)); g_error_free(localError); } return {}; } QByteArray result = QByteArray::fromBase64(value); secret_password_free(value); return result;
#else
    Q_UNUSED(name); fail(error, QStringLiteral("no supported OS credential vault is available")); return {};
#endif
}
bool CredentialStore::remove(const QString &name, QString *error) const {
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral("OpenBioUnlock"), QStringLiteral("Desktop")); settings.remove(name); return true;
#elif defined(OPENBIOUNLOCK_HAS_LIBSECRET)
    static const SecretSchema schema = {"com.openbiounlock.Credential", SECRET_SCHEMA_NONE, {{"name", SECRET_SCHEMA_ATTRIBUTE_STRING}, {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}}}; GError *localError = nullptr; const bool ok = secret_password_clear_sync(&schema, nullptr, &localError, "name", name.toUtf8().constData(), nullptr); if (!ok && localError) { fail(error, QString::fromUtf8(localError->message)); g_error_free(localError); } return ok;
#else
    Q_UNUSED(name); fail(error, QStringLiteral("no supported OS credential vault is available")); return false;
#endif
}
}