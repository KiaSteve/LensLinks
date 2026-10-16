import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

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

  Uint8List? currentFrame;

  // The phone we are connected to. null means "not connected, and do not
  // reconnect automatically".
  String? lastIp;

  bool get isConnected =>
      lastIp != null && !reconnecting && !connecting && tcpClient.connected;

  @override
  void initState() {
    super.initState();

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
                    minScale: 1,
                    maxScale: 4,
                    child: SizedBox.expand(
                      child: Image.memory(
                        currentFrame!,
                        fit: BoxFit.cover,
                        gaplessPlayback: true,
                        filterQuality: FilterQuality.high,
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
                            onPressed: () {},
                          ),

                          IconButton(
                            icon: const Icon(Icons.screen_rotation),
                            color: Colors.white,
                            tooltip: "Rotate",
                            onPressed: () {},
                          ),

                          IconButton(
                            icon: const Icon(Icons.zoom_in),
                            color: Colors.white,
                            tooltip: "Zoom",
                            onPressed: () {},
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
                            onPressed: () {},
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
