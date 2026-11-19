# Changelog

## 1.0.0 - Model One

First release.

- Phone camera streamed over Wi-Fi to Windows (TCP 5001) with automatic discovery (UDP 5000).
- Windows 11 virtual camera, "LensLinks Camera", working in browsers, WhatsApp and the Windows Camera app (NV12, 1280x720).
- Windows app: Scan, Disconnect, Reconnect, Rotate (remembered), Snapshot, Zoom, Settings, Fullscreen.
- Record button: H.264 MP4 with AAC audio from the phone microphone (TCP 5002), saved to `Videos\LensLinks`.
- Encrypted and authenticated streams: the phone shows a random 16-character pairing key (entered once on the PC). Both ports use a mutual HMAC-SHA256 challenge, per-connection session keys, ChaCha20 encryption with HMAC-SHA256 integrity checks, and a lockout after 5 wrong keys.
- Installer and uninstaller for the virtual camera (`installer\install.ps1`, `uninstall.ps1`).

Known limits: no forward secrecy, the cipher code has not been independently audited, phone microphone is recording-only, Windows "N" editions lack the H.264/AAC encoders, tested on one PC and one phone.
