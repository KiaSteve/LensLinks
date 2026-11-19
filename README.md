# LensLinks

**Version 1.0.0 ("Model One")**

Use an Android phone as a webcam on Windows. The phone streams its camera and
microphone over Wi-Fi to a Windows app, and the Windows app feeds a virtual
camera called **LensLinks Camera**, which browsers and video-call apps can
select. The Windows app can also record the stream (video plus phone audio)
to an MP4 file.

> **Security:** video and audio are **encrypted** between the phone and the PC
> (ChaCha20 with HMAC-SHA256 integrity checks). The phone makes a random 16-character
> **pairing key** once and keeps it; you type it into the Windows app once and the
> PC remembers it. Without the key nothing can connect, and anything that is
> tampered with, repeated or reordered is detected and the connection is closed.
> Every connection uses fresh session keys, the key itself is never sent over the
> network, and a PC that tries a wrong key 5 times is blocked for a minute.
> Keep the key private: it is shown on the phone screen and stored on the PC in
> `%APPDATA%\LensLinks\pairing.txt`. Tap **Make a new key** on the phone if it
> leaks. Not covered: someone who steals the key *and* recorded your traffic can
> decrypt it (no forward secrecy), and message sizes and timing are visible.

## Requirements

- Windows 11 (the virtual camera API is not available on older versions). "N" editions need the Media Feature Pack for recording (H.264/AAC encoders).
- An Android phone and the PC on the **same Wi-Fi network**
- Flutter, and Visual Studio Build Tools with CMake (to build from source)

## Projects

| Folder | What it is |
| --- | --- |
| `lenslinks` | Windows app (Flutter) and the native camera DLL (`native/`) |
| `lenslinks_mobile` | Android app (Flutter) that captures and sends the camera |

## Install the virtual camera (once, as Administrator)

Build the DLL (see below), then from an **elevated** PowerShell in this folder:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\install.ps1
```

This copies the DLL to `C:\Program Files\LensLinks`, registers it, and creates
`C:\ProgramData\LensLinks` with restricted permissions. To remove it:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\uninstall.ps1
```

## Run

Start the phone app first, then the Windows app, each in its own window:

```powershell
# Phone (use the ID shown by "flutter devices")
cd lenslinks_mobile
flutter run -d <device-id> --release

# Windows
cd lenslinks
flutter run -d windows --release
```

In the Windows app press **Scan**. With one phone found it connects
automatically; with several, tap the one you want. The first time, the app asks
for the **pairing key** shown on the phone screen (for example
`ABCD-EFGH-JKMN-PQRS`); enter it and press Connect. The PC remembers it, and the
phone keeps its key between runs, so this is a one-time step per phone. Then pick
**LensLinks Camera** in your browser or video app.

## Windows app controls

- **Scan / Disconnect** - find the phone, or drop the connection
- **Reconnect** - reconnect to the last phone
- **Rotate** - turns both the preview and the virtual camera by 90 degrees
  (remembered between runs)
- **Snapshot** - saves the current frame to `Pictures\LensLinks`
- **Record** - starts/stops recording to `Videos\LensLinks` (H.264 video plus AAC audio from the phone microphone, with a timer while recording)
- **Fullscreen**; the picture also zooms with a pinch or the scroll wheel
- **Zoom** steps the picture 1x to 4x; **Settings** has a fill/fit switch for the preview, reset rotation, and shortcuts to the snapshot and log folders

## Building the native DLL

```powershell
cmake -S native -B native_build
cmake --build native_build --config Release
```

The Windows app loads `LensLinksNative.dll` from next to its `.exe`
(`build\windows\x64\runner\Release`), while Frame Server loads the copy in
`C:\Program Files\LensLinks`. After rebuilding, update **both** copies, with
Frame Server stopped (`net stop FrameServer`), and compare file hashes to make
sure each copy really changed.

`native_build\Release\activate_test.exe` is an integration test of the camera
source and should end with `30-FRAME MEDIA PIPELINE TEST PASSED`.

## Files and logs

| Path | Purpose |
| --- | --- |
| `C:\ProgramData\LensLinks\frame.bin` | Frames passed from the app to Frame Server |
| `C:\ProgramData\LensLinks\log.txt` | Log from the DLL (moved to `log.old.txt` above 2 MB) |
| `%APPDATA%\LensLinks\settings.txt` | Saved rotation |
| `%APPDATA%\LensLinks\pairing.txt` | The phone's pairing key (keep private) |
| `Pictures\LensLinks` | Snapshots |
| `Videos\LensLinks` | Recordings (`LensLinks_<date>_<time>.mp4`) |

## Troubleshooting

- **Scan finds nothing:** check both devices are on the same Wi-Fi, the phone
  app is open, and Windows Firewall allows the app. Discovery uses UDP port
  5000, video uses TCP port 5001 and audio uses TCP port 5002.
- **Recording has no sound:** a weak Wi-Fi connection drops audio. Check the
  network first; the log shows `audio track: yes` and an `audio chunks` count
  when recording finishes.
- **"Pairing key not accepted":** type the key currently shown on the phone
  (it changes only if you tap **Make a new key**). After 5 wrong tries the phone
  blocks this PC for one minute.
- **The camera shows a dark-gray frame:** no phone is streaming (frames older
  than 3 seconds are dropped).
- **Camera missing in apps:** the DLL is not registered; run `install.ps1`
  elevated.
- **Cannot delete `log.txt`:** a process still has the DLL loaded. Close the
  app and any browser tab or app using the camera.
- **Picture does not change after a rebuild:** an old DLL copy is still in use.
  Stop Frame Server, copy the new DLL to both locations, and compare hashes.

## Known limits

- Discovery (UDP 5000) is not encrypted or authenticated: anyone on the network can see that a phone is running LensLinks, but cannot connect or watch without the key.
- Encryption uses a pure-Dart cipher implementation (PointyCastle). It has not had an independent security audit.
- The phone's microphone is used for recordings only. It is not available as a
  microphone in other apps (browsers, WhatsApp), which keep using the PC's own
  microphone. A virtual audio device is a possible future addition.
- Recording audio and video are timestamped when they arrive, so sync is
  approximate.
- Only Android phones and Windows 11 have been tested (one PC, one phone, one
  home network).

- The camera is fixed at 1280x720; other frame shapes are letterboxed.
- The virtual camera advertises NV12 at 1280x720. An earlier RGB32 version showed red and blue swapped in some apps (browser, WhatsApp) but not others, because applications disagree on RGB32 byte order; NV12 is read the same way everywhere.
- Frame rate is set by the phone's camera and is about 14 fps in the tests so
  far.
- Discovery assumes a /24 network on each adapter and also sends to
  255.255.255.255. It has only been tested on one home Wi-Fi network.
