import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import 'native_bindings.dart';

class VirtualCameraBridge {
  VirtualCameraBridge._();

  static final instance = VirtualCameraBridge._();

  late final DynamicLibrary _library;

  late final LLInitialize _initialize;
  late final LLShutdown _shutdown;
  late final LLWriteFrame _writeFrame;
  LLWriteJpeg? _writeJpeg;
  LLSetRotation? _setRotation;

  bool _loaded = false;

  Future<void> load() async {
    if (_loaded) return;

    _library = DynamicLibrary.open("LensLinksNative.dll");

    _initialize = _library.lookupFunction<LLInitializeNative, LLInitialize>(
      "LL_Initialize",
    );

    _shutdown = _library.lookupFunction<LLShutdownNative, LLShutdown>(
      "LL_Shutdown",
    );

    _writeFrame = _library.lookupFunction<LLWriteFrameNative, LLWriteFrame>(
      "LL_WriteFrame",
    );

    try {
      _writeJpeg = _library.lookupFunction<LLWriteJpegNative, LLWriteJpeg>(
        "LL_WriteJpeg",
      );
    } catch (_) {
      _writeJpeg = null;
    }

    try {
      _setRotation = _library.lookupFunction<LLSetRotationNative, LLSetRotation>(
        "LL_SetRotation",
      );
    } catch (_) {
      _setRotation = null;
    }

    _initialize();

    _loaded = true;
  }

  /// Receives RAW RGBA pixels
  void sendFrame(Uint8List rgba, int width, int height) {
    if (!_loaded) return;

    final ptr = malloc<Uint8>(rgba.length);

    ptr.asTypedList(rgba.length).setAll(0, rgba);

    _writeFrame(ptr, rgba.length, width, height);

    malloc.free(ptr);
  }

  /// Rotates what the virtual camera sends: 0, 90, 180 or 270 degrees clockwise.
  /// Returns false if the native library is too old to support it.
  bool setRotation(int degrees) {
    final set = _setRotation;
    if (!_loaded || set == null) return false;
    set(degrees);
    return true;
  }

  bool get hasJpeg => _loaded && _writeJpeg != null;

  /// Hands a JPEG straight to the native side, which decodes it and fits it
  /// into the camera frame (letterboxed).
  void sendJpeg(Uint8List jpeg) {
    final write = _writeJpeg;
    if (!_loaded || write == null) return;

    final ptr = malloc<Uint8>(jpeg.length);
    try {
      ptr.asTypedList(jpeg.length).setAll(0, jpeg);
      write(ptr, jpeg.length);
    } finally {
      malloc.free(ptr);
    }
  }

  void dispose() {
    if (!_loaded) return;

    _shutdown();

    _loaded = false;
  }
}
