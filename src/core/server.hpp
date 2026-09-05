#pragma once
#include "crypto.hpp"
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QTcpServer>
#include <QUdpSocket>

class QTcpSocket;
class QTimer;
namespace openbiounlock {
class Server final : public QObject {
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    bool start(quint16 port = 43295);
    void stop();
    bool isRunning() const;
    quint16 port() const;
    QByteArray pairingPayload(const QString &host = QString());
Q_SIGNALS:
    void logMessage(const QString &message);
    void statusChanged(const QString &status);
    void deviceChanged(const QString &deviceId, bool connected);
private Q_SLOTS:
    void acceptConnections();
    void readDatagrams();
    void broadcastDiscovery();
    void readClient();
    void clientDisconnected();
private:
    void processLine(QTcpSocket *socket, const QByteArray &line);
    QJsonObject encryptedResponse(const QString &deviceId, const QJsonObject &payload);
    bool decryptRequest(const QJsonObject &envelope, QString *deviceId, QJsonObject *payload, QString *error);
    QTcpServer tcpServer_;
    QUdpSocket udpSocket_;
    QTimer *discoveryTimer_ = nullptr;
    QHash<QString, QByteArray> pairedKeys_;
    QHash<QString, QByteArray> sessionKeys_;
    QHash<QTcpSocket *, QByteArray> buffers_;
    crypto::ReplayGuard replayGuard_;
    quint16 port_ = 43295;
    QByteArray pcId_;
    crypto::X25519KeyPair x25519KeyPair_;
    QString pairingCode_;
    qint64 pairingCreatedAt_ = 0;
};
}