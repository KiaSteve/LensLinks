import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/foundation.dart';
import '../ffi/virtual_camera_bridge.dart';

/// Receives the phone's microphone audio on TCP port 5002.
///
/// Format: PCM, 16-bit little-endian, 44100 Hz, mono, in chunks, each
/// preceded by its length as a 4-byte big-endian integer.
class AudioClient {
  static const int port = 5002;

  Socket? _socket;
  final List<int> _buffer = [];

  int _chunks = 0;
  int _bytes = 0;
  int _statStartMs = 0;

  bool get connected => _socket != null;

  /// Connects to the phone. Never throws: an older phone app without audio
  /// simply refuses the connection.
  Future<void> start(String ip) async {
    stop();

    try {
      final socket = await Socket.connect(
        ip,
        port,
        timeout: const Duration(seconds: 3),
      );

      socket.setOption(SocketOption.tcpNoDelay, true);

      _socket = socket;
      _buffer.clear();
      _chunks = 0;
      _bytes = 0;
      _statStartMs = DateTime.now().millisecondsSinceEpoch;

      socket.listen(
        _onData,
        onDone: () {
          debugPrint("Audio connection closed");
          if (identical(_socket, socket)) stop();
        },
        onError: (Object e) {
          debugPrint("Audio error: $e");
          if (identical(_socket, socket)) stop();
        },
        cancelOnError: true,
      );

      debugPrint("Audio connected to $ip:$port");
    } catch (e) {
      debugPrint("Audio not available ($e)");
    }
  }

  void stop() {
    final socket = _socket;

    _socket = null;
    _buffer.clear();

    try {
      socket?.destroy();
    } catch (_) {}
  }

  void _onData(Uint8List data) {
    _buffer.addAll(data);

    while (true) {
      if (_buffer.length < 4) return;

      final length =
          (_buffer[0] << 24) | (_buffer[1] << 16) | (_buffer[2] << 8) | _buffer[3];

      // Nothing sane is bigger than a megabyte: the stream is out of step.
      if (length > (1 << 20)) {
        debugPrint("Audio stream out of sync, closing it");
        stop();
        return;
      }

      if (_buffer.length < 4 + length) return;

      final chunk = Uint8List.fromList(_buffer.sublist(4, 4 + length));

      _buffer.removeRange(0, 4 + length);

      _onChunk(chunk);
    }
  }

  void _onChunk(Uint8List chunk) {
    _chunks++;
    VirtualCameraBridge.instance.sendAudio(chunk);
    _bytes += chunk.length;

    final nowMs = DateTime.now().millisecondsSinceEpoch;

    if (nowMs - _statStartMs >= 5000) {
      final secs = (nowMs - _statStartMs) / 1000;

      debugPrint(
        "AUDIO: ${(_chunks / secs).toStringAsFixed(1)} chunks/s, "
        "${(_bytes / secs / 1000).toStringAsFixed(1)} KB/s (about 88 expected)",
      );

      _chunks = 0;
      _bytes = 0;
      _statStartMs = nowMs;
    }
  }
}
