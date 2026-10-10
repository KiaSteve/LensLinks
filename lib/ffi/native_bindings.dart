import 'dart:ffi';

typedef LLInitializeNative = Bool Function();
typedef LLInitialize = bool Function();

typedef LLShutdownNative = Void Function();
typedef LLShutdown = void Function();

typedef LLWriteFrameNative = Bool Function(Pointer<Uint8>, Int32, Int32, Int32);

typedef LLWriteFrame = bool Function(Pointer<Uint8>, int, int, int);

typedef LLWriteJpegNative = Bool Function(Pointer<Uint8>, Int32);
typedef LLWriteJpeg = bool Function(Pointer<Uint8>, int);
