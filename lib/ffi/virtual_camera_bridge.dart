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
  LLStartRecording? _startRecording;
  LLStopRecording? _stopRecording;
  LLIsRecording? _isRecording;

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

    try {
      _startRecording = _library.lookupFunction<LLStartRecordingNative, LLStartRecording>(
        "LL_StartRecording",
      );
      _stopRecording = _library.lookupFunction<LLStopRecordingNative, LLStopRecording>(
        "LL_StopRecording",
      );
      _isRecording = _library.lookupFunction<LLIsRecordingNative, LLIsRecording>(
        "LL_IsRecording",
      );
    } catch (_) {
      _startRecording = null;
      _stopRecording = null;
      _isRecording = null;
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

  bool get canRecord =>
      _loaded && _startRecording != null && _stopRecording != null;

  bool get isRecording {
    final query = _isRecording;
    return _loaded && query != null && query();
  }

  /// Starts recording what the virtual camera shows into an MP4 file.
  /// Returns false if it could not start.
  bool startRecording(String path) {
    final start = _startRecording;
    if (!_loaded || start == null) return false;

    final ptr = path.toNativeUtf16();
    try {
      return start(ptr.cast<Uint16>());
    } finally {
      malloc.free(ptr);
    }
  }

  /// Stops recording and finishes the file. Returns false if the file may
  /// be incomplete.
  bool stopRecording() {
    final stop = _stopRecording;
    if (!_loaded || stop == null) return false;
    return stop();
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
