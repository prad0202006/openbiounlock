import 'dart:typed_data';
import 'package:cryptography/cryptography.dart';
import 'package:local_auth/local_auth.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';

class CryptoService {
  final LocalAuthentication _biometrics = LocalAuthentication();
  final Ed25519 _ed25519 = Ed25519();
  final FlutterSecureStorage _storage = const FlutterSecureStorage();
  SimpleKeyPair? _keyPair;

  Future<void> initialize() async {
    if (_keyPair != null) return;
    final encodedSeed = await _storage.read(key: 'openbiounlock.ed25519.seed');
    if (encodedSeed == null) {
      final generated = await _ed25519.newKeyPair();
      final seed = await generated.extractPrivateKeyBytes();
      await _storage.write(key: 'openbiounlock.ed25519.seed', value: _hex(seed));
      _keyPair = generated;
    } else {
      final seed = _fromHex(encodedSeed);
      if (seed.length != 32) throw StateError('stored signing key has invalid length');
      _keyPair = await _ed25519.newKeyPairFromSeed(seed);
    }
  }

  Future<Uint8List> signChallenge(Uint8List nonce, int timestamp) async {
    if (nonce.length != 32) throw ArgumentError('nonce must be 32 bytes');
    await initialize();
    final authenticated = await _biometrics.authenticate(localizedReason: 'Approve workstation unlock');
    if (!authenticated) throw StateError('biometric approval denied');
    final payload = BytesBuilder()..add(nonce)..add(_u64be(timestamp));
    final signature = await _ed25519.sign(payload.toBytes(), keyPair: _keyPair!);
    return Uint8List.fromList(signature.bytes);
  }

  Future<SimplePublicKey> publicKey() async { await initialize(); return _keyPair!.extractPublicKey(); }

  Uint8List _u64be(int value) {
    final data = ByteData(8)..setUint64(0, value, Endian.big);
    return data.buffer.asUint8List();
  }

  String _hex(List<int> bytes) => bytes.map((byte) => byte.toRadixString(16).padLeft(2, '0')).join();

  Uint8List _fromHex(String value) {
    if (value.length.isOdd) throw const FormatException('invalid signing key encoding');
    final bytes = <int>[];
    for (var index = 0; index < value.length; index += 2) {
      final byte = int.tryParse(value.substring(index, index + 2), radix: 16);
      if (byte == null) throw const FormatException('invalid signing key encoding');
      bytes.add(byte);
    }
    return Uint8List.fromList(bytes);
  }
}
