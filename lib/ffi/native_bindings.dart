import 'dart:ffi';

typedef LLInitializeNative = Bool Function();
typedef LLInitialize = bool Function();

typedef LLShutdownNative = Void Function();
typedef LLShutdown = void Function();

typedef LLWriteFrameNative = Bool Function(Pointer<Uint8>, Int32, Int32, Int32);

typedef LLWriteFrame = bool Function(Pointer<Uint8>, int, int, int);

typedef LLWriteJpegNative = Bool Function(Pointer<Uint8>, Int32);
typedef LLWriteJpeg = bool Function(Pointer<Uint8>, int);

typedef LLSetRotationNative = Void Function(Int32);
typedef LLSetRotation = void Function(int);

typedef LLStartRecordingNative = Bool Function(Pointer<Uint16>);
typedef LLStartRecording = bool Function(Pointer<Uint16>);
typedef LLStopRecordingNative = Bool Function();
typedef LLStopRecording = bool Function();
typedef LLIsRecordingNative = Bool Function();
typedef LLIsRecording = bool Function();
