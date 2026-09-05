#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QSet>
#include <QString>
namespace openbiounlock::crypto {
struct X25519KeyPair { QByteArray privateKey; QByteArray publicKey; };
struct GcmCiphertext { QByteArray ciphertext; QByteArray tag; };
X25519KeyPair generateX25519KeyPair(QString *error = nullptr);
QByteArray x25519SharedSecret(const QByteArray &privateKey, const QByteArray &peerPublicKey, QString *error = nullptr);
GcmCiphertext encryptAes256Gcm(const QByteArray &key, const QByteArray &nonce, const QByteArray &plaintext, const QByteArray &associatedData = {}, QString *error = nullptr);
QByteArray decryptAes256Gcm(const QByteArray &key, const QByteArray &nonce, const GcmCiphertext &ciphertext, const QByteArray &associatedData = {}, QString *error = nullptr);
bool verifyEd25519(const QByteArray &publicKey, const QByteArray &message, const QByteArray &signature, QString *error = nullptr);
bool isFreshTimestamp(qint64 timestamp, qint64 now = QDateTime::currentSecsSinceEpoch());
class ReplayGuard {
public: bool accept(const QByteArray &nonce, qint64 timestamp);
private: QMutex mutex_; QSet<QByteArray> seen_; qint64 lastCleanup_ = 0;
};
}