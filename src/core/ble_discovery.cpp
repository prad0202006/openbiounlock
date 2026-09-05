#include "ble_discovery.hpp"
#include <QBluetoothUuid>

namespace openbiounlock {
BleDiscovery::BleDiscovery(QObject *parent) : QObject(parent) {
    connect(&agent_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BleDiscovery::discovered);
    connect(&agent_, &QBluetoothDeviceDiscoveryAgent::finished, this, &BleDiscovery::finished);
    connect(&agent_, &QBluetoothDeviceDiscoveryAgent::canceled, this, &BleDiscovery::finished);
    connect(&agent_, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &BleDiscovery::error);
}
void BleDiscovery::start() { if (!agent_.isActive()) agent_.start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod); }
void BleDiscovery::stop() { if (agent_.isActive()) agent_.stop(); }
void BleDiscovery::discovered(const QBluetoothDeviceInfo &device) { if (device.coreConfigurations().testFlag(QBluetoothDeviceInfo::LowEnergyCoreConfiguration)) emit deviceFound(device.deviceUuid().toString(), device.name(), device.rssi()); }
void BleDiscovery::finished() {}
void BleDiscovery::error(QBluetoothDeviceDiscoveryAgent::Error error) { emit discoveryError(agent_.errorString().isEmpty() ? QStringLiteral("Bluetooth discovery failed: %1").arg(int(error)) : agent_.errorString()); }
}