#pragma once
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QObject>

namespace openbiounlock {
class BleDiscovery final : public QObject {
    Q_OBJECT
public:
    explicit BleDiscovery(QObject *parent = nullptr);
    void start();
    void stop();
Q_SIGNALS:
    void deviceFound(const QString &identifier, const QString &name, qint16 signalStrength);
    void discoveryError(const QString &message);
private Q_SLOTS:
    void discovered(const QBluetoothDeviceInfo &device);
    void finished();
    void error(QBluetoothDeviceDiscoveryAgent::Error error);
private:
    QBluetoothDeviceDiscoveryAgent agent_;
};
}