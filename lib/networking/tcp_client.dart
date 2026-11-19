import 'dart:async';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:image/image.dart' as img;

import '../ffi/virtual_camera_bridge.dart';
import 'audio_client.dart';
import 'pairing.dart';

class TcpClient {
  Socket? _socket;

  final StreamController<Uint8List> _frameController =
      StreamController<Uint8List>.broadcast();

  Stream<Uint8List> get frames => _frameController.stream;

  final List<int> _buffer = [];

  VoidCallback? onDisconnected;

  bool _disconnecting = false;
  final AudioClient _audio = AudioClient();

  // True once the phone has accepted the pairing key.
  bool _ready = false;

  /// The pairing key shown on the phone. null until the user has entered one.
  PairingKey? key;

  /// True when the last [connect] did not go ahead because the pairing key was
  /// missing or the phone refused it.
  bool pairingRejected = false;

  bool get hasKey => key != null;

  bool get connected => _socket != null && _ready;

  Future<void> connect(String ip) async {
    if (connected) {
      await disconnect();
    }

    pairingRejected = false;

    final pairingKey = key;

    if (pairingKey == null) {
      pairingRejected = true;
      debugPrint("No pairing key entered yet");
      return;
    }

    try {
      _buffer.clear();

      final socket = await Socket.connect(
        ip,
        5001,
        timeout: const Duration(seconds: 5),
      );

      _socket = socket;

      // Initialize the native virtual camera
      await VirtualCameraBridge.instance.load();

      final handshake = PairingClient(key: pairingKey, send: socket.add);
      SecureReader? reader;
      final outcome = Completer<PairingResult>();
      var authenticated = false;

      socket.listen(
        (Uint8List data) {
          if (!authenticated) {
            final rest = handshake.feed(data);

            if (handshake.result == PairingResult.pending) return;

            if (!outcome.isCompleted) outcome.complete(handshake.result);

            if (handshake.result != PairingResult.accepted) return;

            authenticated = true;
            _ready = true;
            reader = SecureReader(handshake.keys!);

            if (rest.isNotEmpty) _onSecureData(reader!, rest);
            return;
          }

          _onSecureData(reader!, data);
        },
        onDone: () {
          if (!authenticated) {
            if (!outcome.isCompleted) outcome.complete(PairingResult.failed);
            return;
          }

          debugPrint("SERVER CLOSED THE CONNECTION");
          _handleDisconnect();
        },
        onError: (e, st) {
          debugPrint("TCP ERROR: $e");
          debugPrint("$st");

          if (!authenticated) {
            if (!outcome.isCompleted) outcome.complete(PairingResult.failed);
            return;
          }

          _handleDisconnect();
        },
        cancelOnError: true,
      );

      final result = await outcome.future.timeout(
        const Duration(seconds: 6),
        onTimeout: () => PairingResult.failed,
      );

      if (result != PairingResult.accepted) {
        try {
          socket.destroy();
        } catch (_) {}

        _socket = null;
        _ready = false;

        if (result == PairingResult.rejected) {
          // Do not report this as a dropped connection: that would start the
          // automatic reconnect, which would keep trying the wrong code.
          pairingRejected = true;
          debugPrint("The phone did not accept the pairing key");
          return;
        }

        debugPrint("Could not complete the connection to $ip:5001");
        _handleDisconnect();
        return;
      }

      debugPrint("Connected to $ip:5001");
      unawaited(_audio.start(ip, pairingKey));
    } catch (e) {
      debugPrint("Failed to connect: $e");
      _handleDisconnect();
    }
  }

  void _handleDisconnect() {
    _audio.stop();
    if (_disconnecting) return;

    _disconnecting = true;

    debugPrint("Disconnected from server");

    try {
      _socket?.destroy();
    } catch (_) {}

    _socket = null;
    _ready = false;
    _buffer.clear();

    onDisconnected?.call();

    _disconnecting = false;
  }

  // Decrypts what the phone sent and hands each packet to [_onData]. Anything
  // that fails the integrity check closes the connection.
  void _onSecureData(SecureReader reader, Uint8List data) {
    try {
      for (final plain in reader.add(data)) {
        _onData(plain);
      }
    } on SecureChannelException catch (e) {
      debugPrint("Encrypted stream error: $e");
      _handleDisconnect();
    }
  }

  void _onData(Uint8List data) {
    _buffer.addAll(data);

    while (true) {
      if (_buffer.length < 4) return;

      final header = ByteData.sublistView(
        Uint8List.fromList(_buffer.sublist(0, 4)),
      );

      final frameLength = header.getUint32(0, Endian.big);

      if (_buffer.length < frameLength + 4) return;

      final jpeg = Uint8List.fromList(_buffer.sublist(4, 4 + frameLength));

      // Flutter preview
      _frameController.add(jpeg);

      if (VirtualCameraBridge.instance.hasJpeg) {
        // Native path: the DLL decodes and fits the JPEG, Dart does no pixel work.
        VirtualCameraBridge.instance.sendJpeg(jpeg);
        _buffer.removeRange(0, frameLength + 4);
        continue;
      }

      // Send frame to native virtual camera
      try {
        final decoded = img.decodeJpg(jpeg);

        if (decoded != null) {
          // The native side needs exactly 1280x720 RGBA (4 bytes per pixel).
          // decodeJpg returns 3-channel RGB, so convert to 4 channels,
          // and resize if the sender uses another resolution.
          var frame = decoded;
          if (frame.width != 1280 || frame.height != 720) {
            frame = img.copyResize(frame, width: 1280, height: 720);
          }
          final rgba = frame.convert(numChannels: 4, alpha: 255).toUint8List();

          VirtualCameraBridge.instance.sendFrame(
            rgba,
            frame.width,
            frame.height,
          );
        }
      } catch (e) {
        debugPrint("Virtual Camera Error: $e");
      }

      _buffer.removeRange(0, frameLength + 4);
    }
  }

  Future<void> disconnect() async {
    _audio.stop();
    if (_disconnecting) return;

    _disconnecting = true;

    try {
      await _socket?.flush();
    } catch (_) {}

    try {
      await _socket?.close();
    } catch (_) {}

    try {
      _socket?.destroy();
    } catch (_) {}

    _socket = null;
    _ready = false;
    _buffer.clear();

    _disconnecting = false;
  }

  Future<void> dispose() async {
    await disconnect();

    // Shut down the native virtual camera
    VirtualCameraBridge.instance.dispose();

    await _frameController.close();
  }
}
