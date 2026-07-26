import 'package:flutter/material.dart';

import '../models/device.dart';
import '../services/discovery_service.dart';
import '../widgets/device_card.dart';
import '../widgets/status_card.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final DiscoveryService discovery = DiscoveryService();

  List<Device> devices = [];

  bool scanning = false;

  Future<void> scanNetwork() async {
    setState(() {
      scanning = true;
    });

    final results = await discovery.scanNetwork();

    setState(() {
      devices = results;
      scanning = false;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("LensLinks"), centerTitle: true),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            const SizedBox(height: 20),

            const Icon(Icons.link, size: 90, color: Colors.blue),

            const SizedBox(height: 20),

            const Text(
              "Connect your phone as a PC camera",
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
            ),

            const SizedBox(height: 30),

            StatusCard(connected: devices.any((d) => d.connected)),

            const SizedBox(height: 25),

            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () {
                      // TODO: Pair phone
                    },
                    icon: const Icon(Icons.qr_code_scanner),
                    label: const Text("Pair Phone"),
                  ),
                ),

                const SizedBox(width: 15),

                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: scanning ? null : scanNetwork,
                    icon: scanning
                        ? const SizedBox(
                            width: 18,
                            height: 18,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        : const Icon(Icons.search),
                    label: Text(scanning ? "Scanning..." : "Scan Network"),
                  ),
                ),
              ],
            ),

            const SizedBox(height: 35),

            const Text(
              "Available Devices",
              style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold),
            ),

            const SizedBox(height: 15),

            Expanded(
              child: devices.isEmpty
                  ? const Center(
                      child: Text(
                        "No devices found.\nClick Scan Network.",
                        textAlign: TextAlign.center,
                      ),
                    )
                  : ListView.builder(
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
          ],
        ),
      ),
    );
  }
}
