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

  void dispose() {
    if (!_loaded) return;

    _shutdown();

    _loaded = false;
  }
}
