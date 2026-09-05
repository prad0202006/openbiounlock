#include "crypto.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
namespace openbiounlock::crypto {
namespace { void setError(QString *error, const QString &value) { if (error) *error = value; } }
X25519KeyPair generateX25519KeyPair(QString *error) {
    EVP_PKEY_CTX *context = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
    if (!context || EVP_PKEY_keygen_init(context) != 1) { EVP_PKEY_CTX_free(context); setError(error, QStringLiteral("X25519 key generation initialization failed")); return {}; }
    EVP_PKEY *key = nullptr;
    if (EVP_PKEY_keygen(context, &key) != 1) { EVP_PKEY_CTX_free(context); setError(error, QStringLiteral("X25519 key generation failed")); return {}; }
    QByteArray privateKey(32, Qt::Uninitialized), publicKey(32, Qt::Uninitialized); size_t privateLength = 32, publicLength = 32;
    const bool ok = EVP_PKEY_get_raw_private_key(key, reinterpret_cast<unsigned char *>(privateKey.data()), &privateLength) == 1 && EVP_PKEY_get_raw_public_key(key, reinterpret_cast<unsigned char *>(publicKey.data()), &publicLength) == 1;
    EVP_PKEY_free(key); EVP_PKEY_CTX_free(context);
    if (!ok) { setError(error, QStringLiteral("X25519 key export failed")); return {}; } return {privateKey, publicKey};
}
QByteArray x25519SharedSecret(const QByteArray &privateKey, const QByteArray &peerPublicKey, QString *error) {
    if (privateKey.size() != 32 || peerPublicKey.size() != 32) { setError(error, QStringLiteral("X25519 keys must be 32 bytes")); return {}; }
    EVP_PKEY *local = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr, reinterpret_cast<const unsigned char *>(privateKey.constData()), 32); EVP_PKEY *peer = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr, reinterpret_cast<const unsigned char *>(peerPublicKey.constData()), 32); EVP_PKEY_CTX *context = local ? EVP_PKEY_CTX_new(local, nullptr) : nullptr; QByteArray secret(32, Qt::Uninitialized); size_t length = 32;
    const bool ok = context && peer && EVP_PKEY_derive_init(context) == 1 && EVP_PKEY_derive_set_peer(context, peer) == 1 && EVP_PKEY_derive(context, reinterpret_cast<unsigned char *>(secret.data()), &length) == 1; EVP_PKEY_CTX_free(context); EVP_PKEY_free(peer); EVP_PKEY_free(local);
    if (!ok) { setError(error, QStringLiteral("X25519 shared secret derivation failed")); return {}; } return secret;
}
GcmCiphertext encryptAes256Gcm(const QByteArray &key, const QByteArray &nonce, const QByteArray &plaintext, const QByteArray &associatedData, QString *error) {
    if (key.size() != 32 || nonce.isEmpty()) { setError(error, QStringLiteral("AES-256-GCM requires a 32-byte key and nonce")); return {}; }
    EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new(); QByteArray result(plaintext.size() + 16, Qt::Uninitialized); int written = 0, total = 0; bool ok = context && EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 && EVP_EncryptInit_ex(context, nullptr, nullptr, reinterpret_cast<const unsigned char *>(key.constData()), reinterpret_cast<const unsigned char *>(nonce.constData())) == 1;
    if (ok && !associatedData.isEmpty()) ok = EVP_EncryptUpdate(context, nullptr, &written, reinterpret_cast<const unsigned char *>(associatedData.constData()), associatedData.size()) == 1;
    if (ok && !plaintext.isEmpty()) { ok = EVP_EncryptUpdate(context, reinterpret_cast<unsigned char *>(result.data()), &written, reinterpret_cast<const unsigned char *>(plaintext.constData()), plaintext.size()) == 1; total = written; }
    if (ok) { ok = EVP_EncryptFinal_ex(context, reinterpret_cast<unsigned char *>(result.data()) + total, &written) == 1; total += written; }
    QByteArray tag(16, Qt::Uninitialized); if (ok) ok = EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) == 1; EVP_CIPHER_CTX_free(context);
    if (!ok) { setError(error, QStringLiteral("AES-256-GCM encryption failed")); return {}; } result.resize(total); return {result, tag};
}
QByteArray decryptAes256Gcm(const QByteArray &key, const QByteArray &nonce, const GcmCiphertext &ciphertext, const QByteArray &associatedData, QString *error) {
    if (key.size() != 32 || nonce.isEmpty() || ciphertext.tag.size() != 16) { setError(error, QStringLiteral("invalid AES-256-GCM input")); return {}; }
    EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new(); QByteArray result(ciphertext.ciphertext.size(), Qt::Uninitialized); int written = 0, total = 0; bool ok = context && EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 && EVP_DecryptInit_ex(context, nullptr, nullptr, reinterpret_cast<const unsigned char *>(key.constData()), reinterpret_cast<const unsigned char *>(nonce.constData())) == 1;
    if (ok && !associatedData.isEmpty()) ok = EVP_DecryptUpdate(context, nullptr, &written, reinterpret_cast<const unsigned char *>(associatedData.constData()), associatedData.size()) == 1;
    if (ok && !ciphertext.ciphertext.isEmpty()) { ok = EVP_DecryptUpdate(context, reinterpret_cast<unsigned char *>(result.data()), &written, reinterpret_cast<const unsigned char *>(ciphertext.ciphertext.constData()), ciphertext.ciphertext.size()) == 1; total = written; }
    if (ok) ok = EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, 16, const_cast<char *>(ciphertext.tag.constData())) == 1;
    if (ok) { ok = EVP_DecryptFinal_ex(context, reinterpret_cast<unsigned char *>(result.data()) + total, &written) == 1; total += written; } EVP_CIPHER_CTX_free(context);
    if (!ok) { setError(error, QStringLiteral("AES-256-GCM authentication failed")); return {}; } result.resize(total); return result;
}
bool verifyEd25519(const QByteArray &publicKey, const QByteArray &message, const QByteArray &signature, QString *error) {
    if (publicKey.size() != 32 || signature.size() != 64) { setError(error, QStringLiteral("invalid Ed25519 key or signature length")); return false; }
    EVP_PKEY *key = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, reinterpret_cast<const unsigned char *>(publicKey.constData()), 32); EVP_MD_CTX *context = EVP_MD_CTX_new(); const bool ok = key && context && EVP_DigestVerifyInit(context, nullptr, nullptr, nullptr, key) == 1 && EVP_DigestVerify(context, reinterpret_cast<const unsigned char *>(signature.constData()), 64, reinterpret_cast<const unsigned char *>(message.constData()), message.size()) == 1; EVP_MD_CTX_free(context); EVP_PKEY_free(key); if (!ok) setError(error, QStringLiteral("signature verification failed")); return ok;
}
bool isFreshTimestamp(qint64 timestamp, qint64 now) { return qAbs(timestamp - now) <= 30; }
bool ReplayGuard::accept(const QByteArray &nonce, qint64 timestamp) { if (nonce.isEmpty() || !isFreshTimestamp(timestamp)) return false; QMutexLocker locker(&mutex_); const qint64 now = QDateTime::currentSecsSinceEpoch(); if (now - lastCleanup_ > 60) { seen_.clear(); lastCleanup_ = now; } const QByteArray token = nonce + QByteArray::number(timestamp); if (seen_.contains(token)) return false; seen_.insert(token); return true; }
}