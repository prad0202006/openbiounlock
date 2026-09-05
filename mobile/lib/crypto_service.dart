import 'dart:typed_data';
import 'dart:convert';
import 'dart:math';
import 'package:cryptography/cryptography.dart';
import 'package:local_auth/local_auth.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';

class CryptoService {
  final LocalAuthentication _biometrics = LocalAuthentication();
  final Ed25519 _ed25519 = Ed25519();
  final X25519 _x25519 = X25519();
  final AesGcm _aes = AesGcm.with256bits();
  final FlutterSecureStorage _storage = const FlutterSecureStorage();
  SimpleKeyPair? _keyPair;
  SimpleKeyPair? _exchangeKeyPair;
  SecretKey? _sessionKey;

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
    final exchangeSeed = await _storage.read(key: 'openbiounlock.x25519.seed');
    if (exchangeSeed == null) {
      _exchangeKeyPair = await _x25519.newKeyPair();
      await _storage.write(key: 'openbiounlock.x25519.seed', value: _hex(await _exchangeKeyPair!.extractPrivateKeyBytes()));
    } else {
      final seed = _fromHex(exchangeSeed);
      if (seed.length != 32) throw StateError('stored exchange key has invalid length');
      _exchangeKeyPair = await _x25519.newKeyPairFromSeed(seed);
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

  Future<SimplePublicKey> exchangePublicKey() async { await initialize(); return _exchangeKeyPair!.extractPublicKey(); }

  Future<void> establishSession(String workstationKey) async {
    await initialize();
    final keyBytes = _fromHex(workstationKey);
    if (keyBytes.length != 32) throw const FormatException('invalid workstation exchange key');
    final remote = SimplePublicKey(keyBytes, type: KeyPairType.x25519);
    _sessionKey = await _x25519.sharedSecretKey(keyPair: _exchangeKeyPair!, remotePublicKey: remote);
  }

  Future<Map<String, dynamic>> encryptEnvelope(String deviceId, Map<String, dynamic> payload) async {
    final sessionKey = _sessionKey;
    if (sessionKey == null) throw StateError('secure session has not been established');
    final nonce = Uint8List.fromList(List<int>.generate(12, (_) => Random.secure().nextInt(256)));
    final aad = Uint8List.fromList(utf8.encode(deviceId));
    final box = await _aes.encrypt(Uint8List.fromList(utf8.encode(jsonEncode(payload))), secretKey: sessionKey, nonce: nonce, aad: aad);
    return {'type': 'secure', 'device_id': deviceId, 'nonce': _hex(nonce), 'ciphertext': _hex(box.cipherText), 'tag': _hex(box.mac.bytes)};
  }

  Future<Map<String, dynamic>> decryptEnvelope(String deviceId, Map<String, dynamic> envelope) async {
    final sessionKey = _sessionKey;
    if (sessionKey == null || envelope['type'] != 'secure' || envelope['device_id'] != deviceId) throw const FormatException('invalid secure envelope');
    final box = SecretBox(_fromHex(envelope['ciphertext'] as String), nonce: _fromHex(envelope['nonce'] as String), mac: Mac(_fromHex(envelope['tag'] as String)));
    final clear = await _aes.decrypt(box, secretKey: sessionKey, aad: Uint8List.fromList(utf8.encode(deviceId)));
    final value = jsonDecode(utf8.decode(clear));
    if (value is! Map<String, dynamic>) throw const FormatException('invalid decrypted payload');
    return value;
  }

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
