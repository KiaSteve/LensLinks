# LensLinks

Use an Android phone as a webcam on Windows. The phone streams its camera
over Wi-Fi to a Windows app, and the Windows app feeds a virtual camera called
**LensLinks Camera**, which browsers and video-call apps can select.

## Requirements

- Windows 11 (the virtual camera API is not available on older versions)
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
automatically; with several, tap the one you want. Then pick **LensLinks
Camera** in your browser or video app.

## Windows app controls

- **Scan / Disconnect** - find the phone, or drop the connection
- **Reconnect** - reconnect to the last phone
- **Rotate** - turns both the preview and the virtual camera by 90 degrees
  (remembered between runs)
- **Snapshot** - saves the current frame to `Pictures\LensLinks`
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
| `Pictures\LensLinks` | Snapshots |

## Troubleshooting

- **Scan finds nothing:** check both devices are on the same Wi-Fi, the phone
  app is open, and Windows Firewall allows the app. Discovery uses UDP port
  5000 and the stream uses TCP port 5001.
- **The camera shows a dark-gray frame:** no phone is streaming (frames older
  than 3 seconds are dropped).
- **Camera missing in apps:** the DLL is not registered; run `install.ps1`
  elevated.
- **Cannot delete `log.txt`:** a process still has the DLL loaded. Close the
  app and any browser tab or app using the camera.
- **Picture does not change after a rebuild:** an old DLL copy is still in use.
  Stop Frame Server, copy the new DLL to both locations, and compare hashes.

## Known limits

- The camera is fixed at 1280x720; other frame shapes are letterboxed.
- Frame rate is set by the phone's camera and is about 14 fps in the tests so
  far.
- Discovery assumes a /24 network on each adapter and also sends to
  255.255.255.255. It has only been tested on one home Wi-Fi network.
