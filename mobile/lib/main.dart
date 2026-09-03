import 'dart:typed_data';
import 'dart:convert';
import 'dart:io';
import 'dart:async';
import 'package:flutter/material.dart';
import 'package:qr_code_scanner/qr_code_scanner.dart';
import 'crypto_service.dart';
import 'ble_service.dart';

void main() => runApp(const OpenBioApp());

class OpenBioApp extends StatelessWidget {
  const OpenBioApp({super.key});
  @override Widget build(BuildContext context) => MaterialApp(title: 'OpenBioUnlock', theme: ThemeData(colorSchemeSeed: Colors.teal, useMaterial3: true), home: const HomePage());
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});
  @override State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final crypto = CryptoService();
  final ble = BleService();
  bool proximity = false;
  String status = 'Ready';
  StreamSubscription<ProximityEvent>? proximitySubscription;

  Future<void> testBiometric() async {
    try {
      final signature = await crypto.signChallenge(Uint8List(32), DateTime.now().millisecondsSinceEpoch ~/ 1000);
      setState(() => status = 'Signed ${signature.length}-byte challenge');
    } catch (error) { setState(() => status = error.toString()); }
  }

  Future<void> toggleProximity(bool value) async {
    setState(() => proximity = value);
    if (value) {
      proximitySubscription = ble.devices.listen((event) {
        if (mounted) setState(() => status = '${event.device.platformName.isEmpty ? event.device.remoteId : event.device.platformName}: ${event.nearby ? 'nearby' : 'away'}');
      });
      await ble.startProximityScan();
    } else {
      await proximitySubscription?.cancel();
      await ble.stop();
    }
  }

  Future<void> pairFromQr(BuildContext context) async {
    final result = await Navigator.push<PairingPayload>(context, MaterialPageRoute(builder: (_) => const PairingScanner()));
    if (result == null) return;
    try {
      final key = await crypto.publicKey();
      final socket = await Socket.connect(result.host, result.port, timeout: const Duration(seconds: 8));
      final publicKey = key.bytes.map((byte) => byte.toRadixString(16).padLeft(2, '0')).join();
      final request = jsonEncode({'type': 'pair', 'device_id': publicKey, 'public_key': publicKey, 'pairing_code': result.pairingCode});
      socket.write('$request\n');
      final response = await socket.cast<List<int>>().transform(utf8.decoder).transform(const LineSplitter()).first.timeout(const Duration(seconds: 8));
      await socket.close();
      final accepted = jsonDecode(response)['accepted'] == true;
      if (mounted) setState(() => status = accepted ? 'PC paired' : 'Pairing rejected');
    } catch (error) { if (mounted) setState(() => status = 'Pairing failed: $error'); }
  }

  @override void dispose() { proximitySubscription?.cancel(); ble.dispose(); super.dispose(); }

  @override Widget build(BuildContext context) => Scaffold(
    appBar: AppBar(title: const Text('OpenBioUnlock')),
    body: ListView(padding: const EdgeInsets.all(20), children: [
      Card(child: ListTile(leading: const Icon(Icons.verified_user), title: const Text('Biometric approval'), subtitle: Text(status), trailing: FilledButton(onPressed: testBiometric, child: const Text('Test')))),
      SwitchListTile(title: const Text('Proximity unlock'), subtitle: const Text('Approve nearby paired PCs'), value: proximity, onChanged: toggleProximity),
      ListTile(leading: const Icon(Icons.qr_code_scanner), title: const Text('Pair a PC'), onTap: () => pairFromQr(context)),
      const Divider(),
      const ListTile(title: Text('Paired accounts'), subtitle: Text('No accounts paired yet')),
    ]),
  );
}

class PairingPayload {
  const PairingPayload({required this.host, required this.port, required this.pcKey, required this.pairingCode});
  final String host;
  final int port;
  final String pcKey;
  final String pairingCode;

  factory PairingPayload.fromRaw(String raw) {
    final value = jsonDecode(raw) as Map<String, dynamic>;
    final host = value['host'];
    final port = value['port'];
    final key = value['x25519_public_key'];
    final code = value['pairing_code'];
    if (value['version'] != 1 || host is! String || port is! int || port < 1 || port > 65535 || key is! String || key.length != 64 || code is! String || code.isEmpty) throw const FormatException('invalid OpenBioUnlock pairing code');
    return PairingPayload(host: host, port: port, pcKey: key, pairingCode: code);
  }
}

class PairingScanner extends StatefulWidget {
  const PairingScanner({super.key});
  @override State<PairingScanner> createState() => _PairingScannerState();
}

class _PairingScannerState extends State<PairingScanner> {
  final key = GlobalKey(debugLabel: 'pairing-camera');
  bool handled = false;
  @override Widget build(BuildContext context) => Scaffold(appBar: AppBar(title: const Text('Scan PC pairing code')), body: QRView(key: key, onQRViewCreated: (controller) {
    controller.scannedDataStream.listen((scan) {
      if (handled || scan.code == null) return;
      try { final payload = PairingPayload.fromRaw(scan.code!); handled = true; controller.pauseCamera(); if (mounted) Navigator.pop(context, payload); }
      catch (_) { if (mounted) ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('Invalid pairing code'))); }
    });
  }));
}
