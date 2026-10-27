import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:image/image.dart' as img;
import 'package:window_manager/window_manager.dart';

import '../ffi/virtual_camera_bridge.dart';
import '../models/device.dart';
import '../networking/tcp_client.dart';
import '../services/discovery_service.dart';
import '../widgets/device_card.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final DiscoveryService discovery = DiscoveryService();
  final TcpClient tcpClient = TcpClient();

  List<Device> devices = [];

  bool scanning = false;
  bool scanned = false;
  bool connecting = false;
  bool fullscreen = false;
  bool reconnecting = false;

  // Rotation applied to the virtual camera and the preview, in degrees clockwise.
  int rotation = 0;

  // Zoom button step (1 to 4) and the controller behind the picture.
  final TransformationController zoomController = TransformationController();
  int zoomLevel = 1;

  // Preview only: false shows the whole frame, true fills the window.
  bool fillPreview = false;

  Uint8List? currentFrame;

  // The phone we are connected to. null means "not connected, and do not
  // reconnect automatically".
  String? lastIp;

  bool get isConnected =>
      lastIp != null && !reconnecting && !connecting && tcpClient.connected;

  @override
  void initState() {
    super.initState();

    rotation = _loadRotation();

    tcpClient.frames.listen((frame) {
      if (!mounted) return;

      setState(() {
        currentFrame = frame;
      });
    });

    // Called whenever the TCP connection closes.
    tcpClient.onDisconnected = () {
      reconnect();
    };
  }

  File? _settingsFile() {
    final appData = Platform.environment['APPDATA'];

    if (appData == null) return null;

    return File('$appData\\LensLinks\\settings.txt');
  }

  int _loadRotation() {
    try {
      final file = _settingsFile();

      if (file == null || !file.existsSync()) return 0;

      final value = int.tryParse(file.readAsStringSync().trim()) ?? 0;

      return (value == 90 || value == 180 || value == 270) ? value : 0;
    } catch (_) {
      return 0;
    }
  }

  void _saveRotation(int degrees) {
    try {
      final file = _settingsFile();

      if (file == null) return;

      file.parent.createSync(recursive: true);
      file.writeAsStringSync('$degrees');
    } catch (e) {
      debugPrint("Could not save rotation: $e");
    }
  }

  void _showMessage(String text) {
    if (!mounted) return;

    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(text)),
    );
  }

  Future<void> snapshot() async {
    final frame = currentFrame;

    if (frame == null) {
      _showMessage("No picture to save yet");
      return;
    }

    try {
      var bytes = frame;

      // Match what the preview shows: apply the current rotation.
      if (rotation != 0) {
        final decoded = img.decodeJpg(frame);

        if (decoded != null) {
          final turned = img.copyRotate(decoded, angle: rotation);
          bytes = Uint8List.fromList(img.encodeJpg(turned, quality: 95));
        }
      }

      final home = Platform.environment['USERPROFILE'];

      if (home == null) {
        _showMessage("Snapshot failed: cannot find your user folder");
        return;
      }

      final folder = Directory('$home\\Pictures\\LensLinks');
      await folder.create(recursive: true);

      final now = DateTime.now();
      String two(int v) => v.toString().padLeft(2, '0');

      final name =
          'LensLinks_${now.year}${two(now.month)}${two(now.day)}_'
          '${two(now.hour)}${two(now.minute)}${two(now.second)}.jpg';

      final file = File('${folder.path}\\$name');
      await file.writeAsBytes(bytes);

      _showMessage("Saved ${file.path}");
    } catch (e) {
      _showMessage("Snapshot failed: $e");
    }
  }

  void zoomStep() {
    final next = zoomLevel >= 4 ? 1 : zoomLevel + 1;
    final size = MediaQuery.of(context).size;

    // Scale around the centre of the window.
    final tx = -(next - 1) * size.width / 2;
    final ty = -(next - 1) * size.height / 2;

    zoomController.value =
        Matrix4.diagonal3Values(next.toDouble(), next.toDouble(), 1)
          ..setTranslationRaw(tx, ty, 0);

    setState(() {
      zoomLevel = next;
    });
  }

  Future<void> _openFolder(String path) async {
    try {
      await Directory(path).create(recursive: true);
      await Process.start('explorer.exe', [path]);
    } catch (e) {
      _showMessage("Could not open $path: $e");
    }
  }

  Future<void> openSettings() async {
    final home = Platform.environment['USERPROFILE'] ?? '';

    await showDialog<void>(
      context: context,
      builder: (dialogContext) {
        return StatefulBuilder(
          builder: (context, setDialogState) {
            return AlertDialog(
              title: const Text("Settings"),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  SwitchListTile(
                    contentPadding: EdgeInsets.zero,
                    title: const Text("Fill the window (crop edges)"),
                    subtitle: const Text(
                      "Preview only. The virtual camera is not affected.",
                    ),
                    value: fillPreview,
                    onChanged: (value) {
                      setDialogState(() {});
                      setState(() {
                        fillPreview = value;
                      });
                    },
                  ),
                  const SizedBox(height: 8),
                  TextButton.icon(
                    onPressed: () {
                      if (VirtualCameraBridge.instance.setRotation(0)) {
                        _saveRotation(0);
                        setState(() {
                          rotation = 0;
                        });
                      } else {
                        _showMessage("Rotation is not supported by this library");
                      }
                    },
                    icon: const Icon(Icons.screen_rotation),
                    label: const Text("Reset rotation"),
                  ),
                  TextButton.icon(
                    onPressed: () => _openFolder('$home\\Pictures\\LensLinks'),
                    icon: const Icon(Icons.photo_library),
                    label: const Text("Open snapshots folder"),
                  ),
                  TextButton.icon(
                    onPressed: () => _openFolder('C:\\ProgramData\\LensLinks'),
                    icon: const Icon(Icons.description),
                    label: const Text("Open log folder"),
                  ),
                ],
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.of(dialogContext).pop(),
                  child: const Text("Close"),
                ),
              ],
            );
          },
        );
      },
    );
  }

  void rotate() {
    final next = (rotation + 90) % 360;

    // Tell the virtual camera first. If the native library is too old to
    // support rotation, leave the preview alone so the two cannot disagree.
    if (VirtualCameraBridge.instance.setRotation(next)) {
      _saveRotation(next);
      setState(() {
        rotation = next;
      });
    } else {
      debugPrint("Rotation is not supported by the installed native library");
    }
  }

  Future<void> toggleFullscreen() async {
    fullscreen = !fullscreen;

    await windowManager.setFullScreen(fullscreen);

    if (mounted) {
      setState(() {});
    }
  }

  List<Device> _withConnected(String? ip) {
    return devices
        .map(
          (d) => Device(
            id: d.id,
            name: d.name,
            ip: d.ip,
            port: d.port,
            connected: ip != null && d.ip == ip,
          ),
        )
        .toList();
  }

  Future<void> scanNetwork() async {
    if (scanning || connecting) return;

    setState(() {
      scanning = true;
      scanned = false;
      devices = [];
    });

    List<Device> results = [];

    try {
      results = await discovery.scanNetwork();
    } catch (e) {
      debugPrint("Scan error: $e");
    }

    if (!mounted) return;

    setState(() {
      devices = results;
      scanning = false;
      scanned = true;
    });

    // Exactly one phone answered: connect to it straight away.
    if (results.length == 1) {
      await connectTo(results.first);
    }
  }

  Future<void> connectTo(Device device) async {
    if (connecting) return;

    // Forget any previous phone so a failed attempt cannot trigger the
    // automatic reconnect to an old address.
    lastIp = null;

    setState(() {
      connecting = true;
    });

    try {
      await tcpClient.connect(device.ip);

      if (tcpClient.connected) {
        lastIp = device.ip;
        VirtualCameraBridge.instance.setRotation(rotation);
      } else {
        debugPrint("Could not connect to ${device.ip}");
      }
    } catch (e) {
      debugPrint("Connect error: $e");
    } finally {
      if (mounted) {
        setState(() {
          connecting = false;
          devices = _withConnected(lastIp);
        });
      }
    }
  }

  Future<void> disconnect() async {
    // Forget the phone first, so the automatic reconnect does not kick in.
    lastIp = null;

    await tcpClient.disconnect();

    if (!mounted) return;

    setState(() {
      currentFrame = null;
      devices = _withConnected(null);
    });
  }

  Future<void> reconnect() async {
    final ip = lastIp;

    if (ip == null || reconnecting) return;

    if (!mounted) return;

    setState(() {
      reconnecting = true;
    });

    try {
      await tcpClient.disconnect();

      await Future.delayed(const Duration(seconds: 1));

      // The user may have disconnected or picked another phone meanwhile.
      if (lastIp != ip) return;

      await tcpClient.connect(ip);

      if (!mounted) return;

      setState(() {
        devices = _withConnected(tcpClient.connected ? ip : null);
      });
    } catch (_) {
      Future.delayed(const Duration(seconds: 2), reconnect);
    } finally {
      reconnecting = false;

      if (mounted) {
        setState(() {});
      }
    }
  }

  @override
  void dispose() {
    zoomController.dispose();
    tcpClient.dispose();
    super.dispose();
  }

  Widget _deviceList() {
    const hintStyle = TextStyle(color: Colors.white70, fontSize: 16);

    if (scanning) {
      return const Center(
        child: Text("Looking for your phone...", style: hintStyle),
      );
    }

    if (connecting) {
      return const Center(child: Text("Connecting...", style: hintStyle));
    }

    if (devices.isEmpty) {
      return Center(
        child: Text(
          scanned
              ? "No devices found. Open the phone app on the same Wi-Fi, then scan again."
              : "Press Scan to look for your phone.",
          textAlign: TextAlign.center,
          style: hintStyle,
        ),
      );
    }

    return ListView.builder(
      itemCount: devices.length,
      itemBuilder: (context, index) {
        final device = devices[index];

        return GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTap: () => connectTo(device),
          child: DeviceCard(
            name: device.name,
            address: "${device.ip}:${device.port}",
            connected: device.connected,
          ),
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    final connected = isConnected;
    final busy = connecting || reconnecting;

    final statusText = reconnecting
        ? "Reconnecting..."
        : connecting
        ? "Connecting..."
        : connected
        ? "Android Connected"
        : "Not Connected";

    final statusColor = connected
        ? Colors.green
        : busy
        ? Colors.orange
        : Colors.red;

    return Scaffold(
      backgroundColor: Colors.black,
      body: Stack(
        children: [
          Positioned.fill(
            child: currentFrame == null
                ? const Center(
                    child: Text(
                      "Waiting for camera stream...",
                      style: TextStyle(color: Colors.white, fontSize: 24),
                    ),
                  )
                : InteractiveViewer(
                    transformationController: zoomController,
                    minScale: 1,
                    maxScale: 4,
                    child: SizedBox.expand(
                      child: RotatedBox(
                        quarterTurns: rotation ~/ 90,
                        child: Image.memory(
                          currentFrame!,
                          fit: fillPreview ? BoxFit.cover : BoxFit.contain,
                          gaplessPlayback: true,
                          filterQuality: FilterQuality.high,
                        ),
                      ),
                    ),
                  ),
          ),

          SafeArea(
            child: Padding(
              padding: const EdgeInsets.all(20),
              child: Row(
                children: [
                  Icon(Icons.circle, size: 14, color: statusColor),
                  const SizedBox(width: 8),
                  Text(
                    statusText,
                    style: const TextStyle(color: Colors.white, fontSize: 18),
                  ),
                  const Spacer(),
                  if (connected)
                    OutlinedButton.icon(
                      onPressed: disconnect,
                      icon: const Icon(Icons.link_off),
                      label: const Text("Disconnect"),
                      style: OutlinedButton.styleFrom(
                        foregroundColor: Colors.white,
                        side: const BorderSide(color: Colors.white54),
                      ),
                    )
                  else
                    FilledButton.icon(
                      onPressed: (scanning || connecting) ? null : scanNetwork,
                      icon: scanning
                          ? const SizedBox(
                              width: 18,
                              height: 18,
                              child: CircularProgressIndicator(strokeWidth: 2),
                            )
                          : const Icon(Icons.search),
                      label: Text(scanning ? "Scanning..." : "Scan"),
                    ),
                ],
              ),
            ),
          ),

          if (!connected)
            Align(
              alignment: Alignment.bottomCenter,
              child: Container(
                height: 220,
                padding: const EdgeInsets.all(20),
                decoration: BoxDecoration(
                  color: Colors.grey.shade900,
                  borderRadius: const BorderRadius.vertical(
                    top: Radius.circular(24),
                  ),
                ),
                child: _deviceList(),
              ),
            ),

          if (connected)
            SafeArea(
              child: Align(
                alignment: Alignment.bottomCenter,
                child: Padding(
                  padding: const EdgeInsets.all(20),
                  child: ClipRRect(
                    borderRadius: BorderRadius.circular(18),
                    child: Container(
                      height: 70,
                      color: Colors.black.withValues(alpha: .55),
                      child: Row(
                        children: [
                          const SizedBox(width: 20),

                          IconButton(
                            icon: const Icon(Icons.camera_alt),
                            color: Colors.white,
                            tooltip: "Snapshot",
                            onPressed: snapshot,
                          ),

                          IconButton(
                            icon: const Icon(Icons.screen_rotation),
                            color: Colors.white,
                            tooltip: "Rotate",
                            onPressed: rotate,
                          ),

                          IconButton(
                            icon: const Icon(Icons.zoom_in),
                            color: Colors.white,
                            tooltip: "Zoom",
                            onPressed: zoomStep,
                          ),

                          const Spacer(),

                          IconButton(
                            icon: Icon(
                              fullscreen
                                  ? Icons.fullscreen_exit
                                  : Icons.fullscreen,
                            ),
                            color: Colors.white,
                            tooltip: "Fullscreen",
                            onPressed: toggleFullscreen,
                          ),

                          const SizedBox(width: 10),

                          FilledButton.icon(
                            onPressed: reconnecting ? null : reconnect,
                            icon: reconnecting
                                ? const SizedBox(
                                    width: 16,
                                    height: 16,
                                    child: CircularProgressIndicator(
                                      strokeWidth: 2,
                                    ),
                                  )
                                : const Icon(Icons.refresh),
                            label: Text(
                              reconnecting ? "Reconnecting..." : "Reconnect",
                            ),
                          ),

                          const SizedBox(width: 10),

                          IconButton(
                            icon: const Icon(Icons.settings),
                            color: Colors.white,
                            tooltip: "Settings",
                            onPressed: openSettings,
                          ),

                          const SizedBox(width: 10),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }
}
