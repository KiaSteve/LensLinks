import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/foundation.dart';
import 'package:image/image.dart' as img;

import '../ffi/virtual_camera_bridge.dart';

class TcpClient {
  Socket? _socket;

  final StreamController<Uint8List> _frameController =
      StreamController<Uint8List>.broadcast();

  Stream<Uint8List> get frames => _frameController.stream;

  final List<int> _buffer = [];

  VoidCallback? onDisconnected;

  bool _disconnecting = false;

  bool get connected => _socket != null;

  Future<void> connect(String ip) async {
    if (connected) {
      await disconnect();
    }

    try {
      _buffer.clear();

      _socket = await Socket.connect(
        ip,
        5001,
        timeout: const Duration(seconds: 5),
      );

      // Initialize the native virtual camera
      await VirtualCameraBridge.instance.load();

      debugPrint("Connected to $ip:5001");

      _socket!.listen(
        _onData,
        onDone: () {
          debugPrint("SERVER CLOSED THE CONNECTION");
          _handleDisconnect();
        },
        onError: (e, st) {
          debugPrint("TCP ERROR: $e");
          debugPrint("$st");
          _handleDisconnect();
        },
        cancelOnError: true,
      );
    } catch (e) {
      debugPrint("Failed to connect: $e");
      _handleDisconnect();
    }
  }

  void _handleDisconnect() {
    if (_disconnecting) return;

    _disconnecting = true;

    debugPrint("Disconnected from server");

    try {
      _socket?.destroy();
    } catch (_) {}

    _socket = null;
    _buffer.clear();

    onDisconnected?.call();

    _disconnecting = false;
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

          debugPrint(
            "Calling native: ${frame.width}x${frame.height} (${rgba.length} bytes)",
          );

          VirtualCameraBridge.instance.sendFrame(
            rgba,
            frame.width,
            frame.height,
          );

          debugPrint("Native returned");
        }
      } catch (e) {
        debugPrint("Virtual Camera Error: $e");
      }

      _buffer.removeRange(0, frameLength + 4);
    }
  }

  Future<void> disconnect() async {
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
