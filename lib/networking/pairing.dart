import 'dart:convert';
import 'dart:math';
import 'dart:typed_data';

import 'package:crypto/crypto.dart';
import 'package:pointycastle/export.dart'
    show ChaCha7539Engine, KeyParameter, ParametersWithIV;

/// Pairing and encryption for TCP ports 5001 (video) and 5002 (audio).
///
/// This file is identical in the phone and the Windows app.
///
/// **Pairing key.** The phone makes a random 80-bit key once and keeps it. It
/// is shown as 16 characters ("ABCD-EFGH-JKMN-PQRS"). You type it into the
/// Windows app once; the PC remembers it. It is far too long to guess, which is
/// what makes encryption possible (a short PIN could be guessed offline from a
/// recording of the handshake).
///
/// **Handshake** (each connection, so every connection gets new keys):
///  1. Phone -> PC: 16 random bytes (server nonce).
///  2. PC -> Phone: 16 random bytes (client nonce) + 32-byte proof
///     HMAC-SHA256(authKey, 0x01 | serverNonce | clientNonce).
///  3. Phone -> PC: 32-byte proof HMAC-SHA256(authKey, 0x02 | ...).
///  Both keys come from HKDF-style HMAC-SHA256 over the pairing key and both
///  nonces. The pairing key is never sent. Each side proves it knows the key.
///
/// **Records** after the handshake: length (4 bytes) | ciphertext | tag
/// (16 bytes). The ciphertext is ChaCha20 (a fresh nonce per record from a
/// counter); the tag is HMAC-SHA256 over counter, length and ciphertext
/// (encrypt-then-MAC). A changed, dropped, repeated or reordered record is
/// detected and the connection is closed.
///
/// Not provided: forward secrecy (someone who steals the key AND recorded the
/// traffic can decrypt it) and hiding of message sizes and timing.
const int pairingKeyBytes = 10;
const int pairingNonceLength = 16;
const int pairingProofLength = 32;
const int secureTagLength = 16;
const int secureMaxRecord = 4 * 1024 * 1024;

const String _alphabet = '0123456789ABCDEFGHJKMNPQRSTVWXYZ';

enum PairingResult { pending, accepted, rejected, failed }

class SecureChannelException implements Exception {
  SecureChannelException(this.message);

  final String message;

  @override
  String toString() => 'SecureChannelException: $message';
}

/// The shared secret, shown to people as 16 characters.
class PairingKey {
  PairingKey._(this.bytes);

  final Uint8List bytes;

  static PairingKey generate([Random? random]) {
    final r = random ?? Random.secure();

    return PairingKey._(
      Uint8List.fromList(
        List<int>.generate(pairingKeyBytes, (_) => r.nextInt(256)),
      ),
    );
  }

  /// "ABCD-EFGH-JKMN-PQRS"
  String get display {
    final s = _encode(bytes);

    return '${s.substring(0, 4)}-${s.substring(4, 8)}-'
        '${s.substring(8, 12)}-${s.substring(12, 16)}';
  }

  /// Reads what a person typed. Ignores case, spaces and dashes; accepts
  /// O for 0 and I or L for 1. Returns null if it is not a valid key.
  static PairingKey? parse(String text) {
    final cleaned = text
        .toUpperCase()
        .replaceAll(RegExp(r'[\s-]'), '')
        .replaceAll('O', '0')
        .replaceAll('I', '1')
        .replaceAll('L', '1');

    if (cleaned.length != 16) return null;

    final out = <int>[];
    var buffer = 0;
    var bits = 0;

    for (var i = 0; i < cleaned.length; i++) {
      final value = _alphabet.indexOf(cleaned[i]);

      if (value < 0) return null;

      buffer = (buffer << 5) | value;
      bits += 5;

      if (bits >= 8) {
        out.add((buffer >> (bits - 8)) & 255);
        bits -= 8;
        buffer &= (1 << bits) - 1;
      }
    }

    if (out.length != pairingKeyBytes) return null;

    return PairingKey._(Uint8List.fromList(out));
  }

  static String _encode(Uint8List bytes) {
    final out = StringBuffer();
    var buffer = 0;
    var bits = 0;

    for (final b in bytes) {
      buffer = (buffer << 8) | b;
      bits += 8;

      while (bits >= 5) {
        out.write(_alphabet[(buffer >> (bits - 5)) & 31]);
        bits -= 5;
      }

      buffer &= (1 << bits) - 1;
    }

    return out.toString();
  }
}

class SessionKeys {
  SessionKeys(this.authKey, this.encKey, this.macKey);

  final Uint8List authKey;
  final Uint8List encKey;
  final Uint8List macKey;
}

SessionKeys deriveSessionKeys(
  PairingKey key,
  List<int> serverNonce,
  List<int> clientNonce,
) {
  // HKDF extract, then expand (each output is one 32-byte block).
  final prk = Hmac(sha256, [...serverNonce, ...clientNonce])
      .convert(key.bytes)
      .bytes;

  Uint8List expand(String label) => Uint8List.fromList(
        Hmac(sha256, prk).convert([...utf8.encode(label), 1]).bytes,
      );

  return SessionKeys(
    expand('lenslinks v2 auth'),
    expand('lenslinks v2 enc'),
    expand('lenslinks v2 mac'),
  );
}

List<int> _proof(
  Uint8List authKey,
  int label,
  List<int> serverNonce,
  List<int> clientNonce,
) =>
    Hmac(sha256, authKey)
        .convert([label, ...serverNonce, ...clientNonce]).bytes;

Uint8List _randomBytes(Random random, int length) {
  final bytes = Uint8List(length);

  for (var i = 0; i < length; i++) {
    bytes[i] = random.nextInt(256);
  }

  return bytes;
}

// Compares without stopping at the first difference.
bool _sameBytes(List<int> a, List<int> b) {
  if (a.length != b.length) return false;

  var difference = 0;

  for (var i = 0; i < a.length; i++) {
    difference |= a[i] ^ b[i];
  }

  return difference == 0;
}

/// The phone's side of the handshake (one per connection).
class PairingServer {
  PairingServer({
    required this.key,
    required this.send,
    Random? random,
  }) : serverNonce = _randomBytes(
          random ?? Random.secure(),
          pairingNonceLength,
        );

  final PairingKey key;
  final void Function(List<int>) send;
  final Uint8List serverNonce;

  final List<int> _buffer = [];

  PairingResult result = PairingResult.pending;

  /// Set when [result] is accepted.
  SessionKeys? keys;

  /// Sends the challenge. Call once, right after the PC connects.
  void begin() => send(serverNonce);

  /// Feeds bytes from the PC. Returns bytes that arrived after the handshake.
  Uint8List feed(List<int> data) {
    if (result != PairingResult.pending) {
      return Uint8List.fromList(data);
    }

    _buffer.addAll(data);

    const needed = pairingNonceLength + pairingProofLength;

    if (_buffer.length < needed) return Uint8List(0);

    final clientNonce = _buffer.sublist(0, pairingNonceLength);
    final given = _buffer.sublist(pairingNonceLength, needed);
    final rest = Uint8List.fromList(_buffer.sublist(needed));

    _buffer.clear();

    final derived = deriveSessionKeys(key, serverNonce, clientNonce);
    final expected = _proof(derived.authKey, 1, serverNonce, clientNonce);

    if (_sameBytes(given, expected)) {
      keys = derived;
      result = PairingResult.accepted;
      send(_proof(derived.authKey, 2, serverNonce, clientNonce));
    } else {
      result = PairingResult.rejected;
      send(Uint8List(pairingProofLength));
    }

    return rest;
  }
}

/// The PC's side of the handshake (one per connection).
class PairingClient {
  PairingClient({
    required this.key,
    required this.send,
    Random? random,
  }) : _random = random ?? Random.secure();

  final PairingKey key;
  final void Function(List<int>) send;
  final Random _random;

  final List<int> _buffer = [];

  Uint8List? _serverNonce;
  Uint8List? _clientNonce;
  SessionKeys? _derived;

  PairingResult result = PairingResult.pending;

  /// Set when [result] is accepted.
  SessionKeys? keys;

  /// Feeds bytes from the phone. Returns bytes that arrived after the
  /// handshake (the start of the encrypted stream).
  Uint8List feed(List<int> data) {
    if (result != PairingResult.pending) {
      return Uint8List.fromList(data);
    }

    _buffer.addAll(data);

    if (_clientNonce == null) {
      if (_buffer.length < pairingNonceLength) return Uint8List(0);

      final serverNonce =
          Uint8List.fromList(_buffer.sublist(0, pairingNonceLength));

      _buffer.removeRange(0, pairingNonceLength);

      final clientNonce = _randomBytes(_random, pairingNonceLength);
      final derived = deriveSessionKeys(key, serverNonce, clientNonce);

      _serverNonce = serverNonce;
      _clientNonce = clientNonce;
      _derived = derived;

      send([
        ...clientNonce,
        ..._proof(derived.authKey, 1, serverNonce, clientNonce),
      ]);
    }

    if (_buffer.length < pairingProofLength) return Uint8List(0);

    final given = _buffer.sublist(0, pairingProofLength);
    final rest = Uint8List.fromList(_buffer.sublist(pairingProofLength));

    _buffer.clear();

    final derived = _derived!;
    final expected = _proof(derived.authKey, 2, _serverNonce!, _clientNonce!);

    if (_sameBytes(given, expected)) {
      keys = derived;
      result = PairingResult.accepted;
    } else {
      result = PairingResult.rejected;
    }

    return rest;
  }
}

Uint8List _chacha(Uint8List encKey, int sequence, Uint8List data) {
  // A different nonce for every record, so no keystream is ever reused.
  final nonce = Uint8List(12);

  ByteData.sublistView(nonce).setUint64(4, sequence, Endian.big);

  final cipher = ChaCha7539Engine()
    ..init(true, ParametersWithIV(KeyParameter(encKey), nonce));

  return cipher.process(data);
}

Uint8List _tag(Uint8List macKey, int sequence, Uint8List ciphertext) {
  final message = Uint8List(12 + ciphertext.length);
  final view = ByteData.sublistView(message);

  view.setUint64(0, sequence, Endian.big);
  view.setUint32(8, ciphertext.length, Endian.big);
  message.setRange(12, message.length, ciphertext);

  return Uint8List.fromList(
    Hmac(sha256, macKey).convert(message).bytes.sublist(0, secureTagLength),
  );
}

/// Encrypts records for one connection. Records must be sent in the order
/// they are sealed, and every sealed record must be sent.
class SecureWriter {
  SecureWriter(SessionKeys keys)
      : _encKey = keys.encKey,
        _macKey = keys.macKey;

  final Uint8List _encKey;
  final Uint8List _macKey;

  int _sequence = 0;

  Uint8List seal(Uint8List plain) {
    final sequence = _sequence++;

    final ciphertext = _chacha(_encKey, sequence, plain);

    final record = Uint8List(4 + ciphertext.length + secureTagLength);

    ByteData.sublistView(record).setUint32(0, ciphertext.length, Endian.big);
    record.setRange(4, 4 + ciphertext.length, ciphertext);
    record.setRange(
      4 + ciphertext.length,
      record.length,
      _tag(_macKey, sequence, ciphertext),
    );

    return record;
  }
}

/// Checks and decrypts records for one connection.
class SecureReader {
  SecureReader(SessionKeys keys)
      : _encKey = keys.encKey,
        _macKey = keys.macKey;

  final Uint8List _encKey;
  final Uint8List _macKey;
  final List<int> _buffer = [];

  int _sequence = 0;

  /// Feeds received bytes. Returns the plain data of every complete record.
  /// Throws [SecureChannelException] if anything is wrong; the connection must
  /// then be closed.
  List<Uint8List> add(List<int> data) {
    _buffer.addAll(data);

    final out = <Uint8List>[];

    while (_buffer.length >= 4) {
      final length = (_buffer[0] << 24) |
          (_buffer[1] << 16) |
          (_buffer[2] << 8) |
          _buffer[3];

      if (length > secureMaxRecord) {
        throw SecureChannelException('record too large');
      }

      final total = 4 + length + secureTagLength;

      if (_buffer.length < total) break;

      final ciphertext = Uint8List.fromList(_buffer.sublist(4, 4 + length));
      final tag = _buffer.sublist(4 + length, total);

      _buffer.removeRange(0, total);

      if (!_sameBytes(tag, _tag(_macKey, _sequence, ciphertext))) {
        throw SecureChannelException('record failed the integrity check');
      }

      out.add(_chacha(_encKey, _sequence, ciphertext));
      _sequence++;
    }

    return out;
  }
}

/// Counts wrong keys per PC address and blocks a PC that keeps trying.
class PairingGuard {
  PairingGuard._();

  static final PairingGuard instance = PairingGuard._();

  static const int maxFailures = 5;
  static const Duration lockout = Duration(minutes: 1);

  final Map<String, int> _failures = {};
  final Map<String, DateTime> _lockedUntil = {};

  bool isLocked(String ip) {
    final until = _lockedUntil[ip];

    if (until == null) return false;

    if (DateTime.now().isBefore(until)) return true;

    _lockedUntil.remove(ip);
    _failures.remove(ip);

    return false;
  }

  void recordFailure(String ip) {
    final count = (_failures[ip] ?? 0) + 1;

    _failures[ip] = count;

    if (count >= maxFailures) {
      _lockedUntil[ip] = DateTime.now().add(lockout);
    }
  }

  void recordSuccess(String ip) {
    _failures.remove(ip);
    _lockedUntil.remove(ip);
  }
}
