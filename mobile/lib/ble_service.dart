import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:async';

final openBioUnlockService = Guid('8c4f1000-7f7b-4c42-a6be-6f5b4b7e0001');
final openBioUnlockControlCharacteristic = Guid('8c4f1001-7f7b-4c42-a6be-6f5b4b7e0001');

class ProximityEvent {
  const ProximityEvent(this.device, this.rssi, this.nearby);
  final BluetoothDevice device;
  final int rssi;
  final bool nearby;
}

class BleService {
  final _devices = StreamController<ProximityEvent>.broadcast(sync: true);
  StreamSubscription<List<ScanResult>>? _subscription;
  final Map<DeviceIdentifier, bool> _nearby = {};
  Stream<ProximityEvent> get devices => _devices.stream;

  Future<void> startProximityScan({int nearThreshold = -70, int farThreshold = -82}) async {
    await _subscription?.cancel();
    await FlutterBluePlus.stopScan();
    _subscription = FlutterBluePlus.onScanResults.listen((results) {
      for (final result in results) {
        if (!result.advertisementData.serviceUuids.contains(openBioUnlockService)) continue;
        final previous = _nearby[result.device.remoteId] ?? false;
        final nearby = previous ? result.rssi >= farThreshold : result.rssi >= nearThreshold;
        _nearby[result.device.remoteId] = nearby;
        _devices.add(ProximityEvent(result.device, result.rssi, nearby));
      }
    });
    await FlutterBluePlus.startScan(withServices: [openBioUnlockService], continuousUpdates: true);
  }

  Future<void> stop() async {
    await FlutterBluePlus.stopScan();
    await _subscription?.cancel();
    _subscription = null;
    _nearby.clear();
  }

  Future<void> connect(BluetoothDevice device) async {
    if (device.isConnected) return;
    try {
      await device.connect(timeout: const Duration(seconds: 10));
    } catch (error) {
      if (!error.toString().contains('already_connected')) rethrow;
    }
  }

  Future<List<BluetoothService>> connectAndDiscover(BluetoothDevice device) async {
    await connect(device);
    final services = await device.discoverServices();
    final hasOpenBioUnlock = services.any((service) => service.uuid == openBioUnlockService && service.characteristics.any((characteristic) => characteristic.uuid == openBioUnlockControlCharacteristic));
    if (!hasOpenBioUnlock) throw StateError('device does not expose the OpenBioUnlock GATT service');
    return services;
  }

  Future<void> dispose() async {
    await stop();
    await _devices.close();
  }
}
