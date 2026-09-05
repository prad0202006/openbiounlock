#include "server.hpp"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QTimer>

namespace openbiounlock {
namespace { QJsonObject errorResponse(const QString &message) { return {{QStringLiteral("type"), QStringLiteral("error")}, {QStringLiteral("message"), message}}; } }
namespace { QByteArray timestampBytes(qint64 timestamp) { QByteArray result; for (int shift = 7; shift >= 0; --shift) result.append(char((timestamp >> (shift * 8)) & 0xff)); return result; } }
Server::Server(QObject *parent) : QObject(parent), pcId_(QByteArray::number(QRandomGenerator::global()->generate64(), 16)) { connect(&tcpServer_, &QTcpServer::newConnection, this, &Server::acceptConnections); connect(&udpSocket_, &QUdpSocket::readyRead, this, &Server::readDatagrams); }
bool Server::start(quint16 port) {
    if (isRunning()) return true; port_ = port;
    if (!tcpServer_.listen(QHostAddress::LocalHost, port_)) { emit logMessage(QStringLiteral("TCP listener failed: %1").arg(tcpServer_.errorString())); return false; }
    if (!udpSocket_.bind(QHostAddress::AnyIPv4, port_, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) { emit logMessage(QStringLiteral("UDP discovery listener failed: %1").arg(udpSocket_.errorString())); tcpServer_.close(); return false; }
    discoveryTimer_ = new QTimer(this); discoveryTimer_->setInterval(5000); connect(discoveryTimer_, &QTimer::timeout, this, &Server::broadcastDiscovery); discoveryTimer_->start(); broadcastDiscovery(); emit statusChanged(QStringLiteral("Listening on TCP/UDP %1").arg(port_)); emit logMessage(QStringLiteral("server started on port %1").arg(port_)); return true;
}
void Server::stop() { if (discoveryTimer_) { discoveryTimer_->stop(); discoveryTimer_->deleteLater(); discoveryTimer_ = nullptr; } for (QTcpSocket *socket : buffers_.keys()) socket->disconnectFromHost(); buffers_.clear(); tcpServer_.close(); udpSocket_.close(); emit statusChanged(QStringLiteral("Stopped")); }
bool Server::isRunning() const { return tcpServer_.isListening(); }
quint16 Server::port() const { return port_; }
void Server::acceptConnections() { while (QTcpSocket *socket = tcpServer_.nextPendingConnection()) { buffers_.insert(socket, {}); connect(socket, &QTcpSocket::readyRead, this, &Server::readClient); connect(socket, &QTcpSocket::disconnected, this, &Server::clientDisconnected); emit logMessage(QStringLiteral("client connected from %1").arg(socket->peerAddress().toString())); } }
void Server::readClient() { auto *socket = qobject_cast<QTcpSocket *>(sender()); if (!socket) return; QByteArray &buffer = buffers_[socket]; buffer += socket->readAll(); int end = -1; while ((end = buffer.indexOf('\n')) >= 0) { const QByteArray line = buffer.left(end).trimmed(); buffer.remove(0, end + 1); if (!line.isEmpty()) processLine(socket, line); } }
void Server::clientDisconnected() { auto *socket = qobject_cast<QTcpSocket *>(sender()); if (!socket) return; buffers_.remove(socket); emit logMessage(QStringLiteral("client disconnected")); socket->deleteLater(); }
void Server::processLine(QTcpSocket *socket, const QByteArray &line) {
    QJsonParseError parseError{}; const QJsonDocument document = QJsonDocument::fromJson(line, &parseError); if (parseError.error != QJsonParseError::NoError || !document.isObject()) { socket->write(QJsonDocument(errorResponse(QStringLiteral("invalid JSON request"))).toJson(QJsonDocument::Compact) + '\n'); return; }
    const QJsonObject request = document.object(); const QString type = request.value(QStringLiteral("type")).toString(); QJsonObject response;
    if (type == QStringLiteral("pair")) { const QString id = request.value(QStringLiteral("device_id")).toString(); const QByteArray key = QByteArray::fromHex(request.value(QStringLiteral("public_key")).toString().toLatin1()); if (id.isEmpty() || key.size() != 32 || request.value(QStringLiteral("pairing_code")).toString().isEmpty()) response = errorResponse(QStringLiteral("device id, 32-byte public key, and pairing code are required")); else { pairedKeys_[id] = key; emit deviceChanged(id, true); response = {{QStringLiteral("type"), QStringLiteral("paired")}, {QStringLiteral("accepted"), true}}; emit logMessage(QStringLiteral("paired device %1").arg(id)); } }
    else if (type == QStringLiteral("challenge")) { QByteArray nonce; for (int index = 0; index < 4; ++index) nonce.append(QByteArray::number(QRandomGenerator::global()->generate64(), 16).leftJustified(16, '0', true).toLatin1()); const qint64 timestamp = QDateTime::currentSecsSinceEpoch(); response = {{QStringLiteral("type"), QStringLiteral("challenge")}, {QStringLiteral("challenge_id"), QString::fromLatin1(nonce.toHex())}, {QStringLiteral("nonce"), QString::fromLatin1(nonce.toHex())}, {QStringLiteral("timestamp"), timestamp}}; }
    else if (type == QStringLiteral("verify")) { const QString id = request.value(QStringLiteral("device_id")).toString(); const QByteArray nonce = QByteArray::fromHex(request.value(QStringLiteral("nonce")).toString().toLatin1()); const qint64 timestamp = request.value(QStringLiteral("timestamp")).toVariant().toLongLong(); const QByteArray signature = QByteArray::fromHex(request.value(QStringLiteral("signature")).toString().toLatin1()); QByteArray message = nonce + timestampBytes(timestamp); QString error; const bool authorized = pairedKeys_.contains(id) && replayGuard_.accept(nonce, timestamp) && crypto::verifyEd25519(pairedKeys_.value(id), message, signature, &error); response = {{QStringLiteral("type"), QStringLiteral("authorized")}, {QStringLiteral("authorized"), authorized}}; emit logMessage(authorized ? QStringLiteral("device %1 authorized").arg(id) : QStringLiteral("authorization rejected: %1").arg(error)); }
    else response = errorResponse(QStringLiteral("unsupported request type"));
    socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n');
}
void Server::readDatagrams() { while (udpSocket_.hasPendingDatagrams()) { QByteArray datagram; datagram.resize(int(udpSocket_.pendingDatagramSize())); QHostAddress address; quint16 port; udpSocket_.readDatagram(datagram.data(), datagram.size(), &address, &port); Q_UNUSED(port); QJsonParseError error{}; const QJsonDocument document = QJsonDocument::fromJson(datagram, &error); if (error.error == QJsonParseError::NoError && document.object().value(QStringLiteral("type")).toString() == QStringLiteral("discover")) broadcastDiscovery(); } }
void Server::broadcastDiscovery() { const QJsonObject packet{{QStringLiteral("type"), QStringLiteral("openbiounlock"), {QStringLiteral("pc_id"), QString::fromLatin1(pcId_.toHex())}, {QStringLiteral("port"), port_}}; const QByteArray data = QJsonDocument(packet).toJson(QJsonDocument::Compact); udpSocket_.writeDatagram(data, QHostAddress::Broadcast, port_); }
}