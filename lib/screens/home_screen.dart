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
  bool fullscreen = false;
  bool reconnecting = false;

  Uint8List? currentFrame;

  String? lastIp;

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

  Future<void> scanNetwork() async {
    setState(() {
      scanning = true;
      devices.clear();
    });

    try {
      final results = await discovery.scanNetwork();

      if (results.isNotEmpty) {
        final phone = results.first;

        await tcpClient.connect(phone.ip);

        lastIp = phone.ip;

        results[0] = Device(
          id: phone.id,
          name: phone.name,
          ip: phone.ip,
          port: phone.port,
          connected: true,
        );
      }

      if (mounted) {
        setState(() {
          devices = results;
        });
      }
    } catch (e) {
      debugPrint("Scan error: $e");
    } finally {
      if (mounted) {
        setState(() {
          scanning = false;
        });
      }
    }
  }

  Future<void> reconnect() async {
    if (lastIp == null || reconnecting) return;

    if (!mounted) return;

    setState(() {
      reconnecting = true;
    });

    try {
      await tcpClient.disconnect();

      await Future.delayed(const Duration(seconds: 1));

      await tcpClient.connect(lastIp!);

      if (!mounted) return;

      setState(() {
        devices = devices
            .map(
              (d) => Device(
                id: d.id,
                name: d.name,
                ip: d.ip,
                port: d.port,
                connected: d.ip == lastIp,
              ),
            )
            .toList();
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

  @override
  Widget build(BuildContext context) {
    final connected = lastIp != null && !reconnecting;

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
                  Icon(
                    Icons.circle,
                    size: 14,
                    color: connected ? Colors.green : Colors.red,
                  ),
                  const SizedBox(width: 8),
                  Text(
                    reconnecting
                        ? "Reconnecting..."
                        : connected
                        ? "Android Connected"
                        : "Not Connected",
                    style: const TextStyle(color: Colors.white, fontSize: 18),
                  ),
                  const Spacer(),
                  if (!connected)
                    FilledButton.icon(
                      onPressed: scanning ? null : scanNetwork,
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
                child: ListView.builder(
                  itemCount: devices.length,
                  itemBuilder: (context, index) {
                    final device = devices[index];

                    return DeviceCard(
                      name: device.name,
                      address: "${device.ip}:${device.port}",
                      connected: device.connected,
                    );
                  },
                ),
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
