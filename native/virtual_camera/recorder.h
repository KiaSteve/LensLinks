#pragma once

#include <cstdint>

// Records frames into an H.264 MP4 file on a worker thread.
// Recorder_Submit is cheap when not recording and never waits for the encoder.

bool Recorder_Start(const wchar_t* path, int width, int height);

// Finishes the file. Returns false if it may be incomplete.
bool Recorder_Stop();

bool Recorder_IsRecording();

// rgba: width * height * 4 bytes, R,G,B,A order. Frames whose size does not
// match the size given to Recorder_Start are ignored.
void Recorder_Submit(const uint8_t* rgba, int width, int height);
