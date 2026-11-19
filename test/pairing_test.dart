import 'dart:convert';
import 'dart:math';
import 'dart:typed_data';

import 'package:crypto/crypto.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lenslinks/networking/pairing.dart';

({PairingServer server, PairingClient client}) runHandshake(
  PairingKey serverKey,
  PairingKey clientKey,
) {
  final toClient = <int>[];
  final toServer = <int>[];

  final server = PairingServer(
    key: serverKey,
    send: toClient.addAll,
    random: Random(1),
  );
  final client = PairingClient(
    key: clientKey,
    send: toServer.addAll,
    random: Random(2),
  );

  server.begin();

  client.feed(List<int>.of(toClient));
  toClient.clear();

  server.feed(List<int>.of(toServer));
  toServer.clear();

  client.feed(List<int>.of(toClient));

  return (server: server, client: client);
}

Uint8List randomBytes(int length, int seed) {
  final random = Random(seed);

  return Uint8List.fromList(List<int>.generate(length, (_) => random.nextInt(256)));
}

void main() {
  test('HMAC-SHA256 library matches the published test vector', () {
    final mac = Hmac(sha256, utf8.encode('key')).convert(
      utf8.encode('The quick brown fox jumps over the lazy dog'),
    );

    expect(
      mac.toString(),
      'f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8',
    );
  });

  group('pairing key', () {
    test('is 16 characters and reads back the same', () {
      final key = PairingKey.generate(Random(7));

      expect(
        key.display,
        matches(RegExp(r'^[0-9A-HJKMNP-TV-Z]{4}(-[0-9A-HJKMNP-TV-Z]{4}){3}$')),
      );

      expect(PairingKey.parse(key.display)!.bytes, key.bytes);
      expect(
        PairingKey.parse(key.display.toLowerCase().replaceAll('-', ' '))!.bytes,
        key.bytes,
      );
    });

    test('rejects text that is not a key', () {
      expect(PairingKey.parse(''), isNull);
      expect(PairingKey.parse('ABCD-EFGH'), isNull);
      expect(PairingKey.parse('UUUU-UUUU-UUUU-UUUU'), isNull);
      expect(PairingKey.parse('ABCD-EFGH-JKMN-PQRS-TVWX'), isNull);
    });

    test('two generated keys differ', () {
      expect(
        PairingKey.generate().bytes,
        isNot(PairingKey.generate().bytes),
      );
    });
  });

  group('handshake', () {
    test('succeeds with the same key and both sides get the same keys', () {
      final key = PairingKey.generate();
      final result = runHandshake(key, key);

      expect(result.server.result, PairingResult.accepted);
      expect(result.client.result, PairingResult.accepted);
      expect(result.server.keys!.encKey, result.client.keys!.encKey);
      expect(result.server.keys!.macKey, result.client.keys!.macKey);
    });

    test('fails on both sides with a different key', () {
      final result = runHandshake(PairingKey.generate(), PairingKey.generate());

      expect(result.server.result, PairingResult.rejected);
      expect(result.client.result, PairingResult.rejected);
      expect(result.server.keys, isNull);
      expect(result.client.keys, isNull);
    });

    test('every connection gets different encryption keys', () {
      final key = PairingKey.generate();

      final first = runHandshake(key, key);
      final toClient = <int>[];
      final toServer = <int>[];
      final server = PairingServer(key: key, send: toClient.addAll);
      final client = PairingClient(key: key, send: toServer.addAll);

      server.begin();
      client.feed(toClient);
      toClient.clear();
      server.feed(toServer);
      client.feed(toClient);

      expect(client.result, PairingResult.accepted);
      expect(client.keys!.encKey, isNot(first.client.keys!.encKey));
    });

    test('waits for a split answer', () {
      final key = PairingKey.generate();
      final toServer = <int>[];
      final toClient = <int>[];

      final server = PairingServer(key: key, send: toClient.addAll);
      final client = PairingClient(key: key, send: toServer.addAll);

      server.begin();
      client.feed(toClient);

      final answer = List<int>.of(toServer);

      server.feed(answer.sublist(0, 20));
      expect(server.result, PairingResult.pending);

      server.feed(answer.sublist(20));
      expect(server.result, PairingResult.accepted);
    });
  });

  group('encrypted records', () {
    late SessionKeys keys;

    setUp(() {
      final key = PairingKey.generate();
      keys = runHandshake(key, key).server.keys!;
    });

    test('round trip, including a large payload', () {
      final writer = SecureWriter(keys);
      final reader = SecureReader(keys);

      for (final size in [0, 1, 100, 200000]) {
        final plain = randomBytes(size, size);
        final out = reader.add(writer.seal(plain));

        expect(out.length, 1);
        expect(out.single, plain);
      }
    });

    test('the plain data is not visible in the record', () {
      final writer = SecureWriter(keys);
      final plain = Uint8List.fromList(List<int>.filled(1000, 65));
      final record = writer.seal(plain);

      expect(record.sublist(4, 1004), isNot(plain));
      expect(record.length, 4 + 1000 + secureTagLength);
    });

    test('works when bytes arrive in small pieces and several records join', () {
      final writer = SecureWriter(keys);
      final reader = SecureReader(keys);

      final a = randomBytes(300, 1);
      final b = randomBytes(50, 2);

      final stream = [...writer.seal(a), ...writer.seal(b)];
      final got = <Uint8List>[];

      for (var i = 0; i < stream.length; i += 7) {
        got.addAll(
          reader.add(stream.sublist(i, min(i + 7, stream.length))),
        );
      }

      expect(got, [a, b]);
    });

    test('a changed byte is detected', () {
      final writer = SecureWriter(keys);
      final reader = SecureReader(keys);
      final record = writer.seal(randomBytes(100, 3));

      record[10] ^= 1;

      expect(() => reader.add(record), throwsA(isA<SecureChannelException>()));
    });

    test('a repeated record is detected', () {
      final writer = SecureWriter(keys);
      final reader = SecureReader(keys);
      final record = writer.seal(randomBytes(100, 4));

      reader.add(record);

      expect(() => reader.add(record), throwsA(isA<SecureChannelException>()));
    });

    test('reordered records are detected', () {
      final writer = SecureWriter(keys);
      final reader = SecureReader(keys);

      writer.seal(randomBytes(10, 5));
      final second = writer.seal(randomBytes(10, 6));

      expect(() => reader.add(second), throwsA(isA<SecureChannelException>()));
    });

    test('a record sealed with other keys is refused', () {
      final key = PairingKey.generate();
      final other = runHandshake(key, key).server.keys!;
      final record = SecureWriter(other).seal(randomBytes(100, 7));

      expect(
        () => SecureReader(keys).add(record),
        throwsA(isA<SecureChannelException>()),
      );
    });

    test('an absurd length is refused', () {
      final reader = SecureReader(keys);

      expect(
        () => reader.add([0x7f, 0xff, 0xff, 0xff]),
        throwsA(isA<SecureChannelException>()),
      );
    });
  });

  test('a PC is locked out after five wrong keys and freed by a success', () {
    final guard = PairingGuard.instance;
    const ip = '192.0.2.10';

    for (var i = 0; i < PairingGuard.maxFailures - 1; i++) {
      guard.recordFailure(ip);
      expect(guard.isLocked(ip), isFalse);
    }

    guard.recordFailure(ip);
    expect(guard.isLocked(ip), isTrue);

    guard.recordSuccess(ip);
    expect(guard.isLocked(ip), isFalse);
  });
}
