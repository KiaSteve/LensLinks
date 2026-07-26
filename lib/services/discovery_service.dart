import '../models/device.dart';

class DiscoveryService {
  Future<List<Device>> scanNetwork() async {
    // For now this is fake data.
    // Later we'll replace it with real network discovery.

    await Future.delayed(const Duration(seconds: 2));

    return [
      const Device(
        id: "phone_01",
        name: "Galaxy S24",
        ip: "192.168.1.12",
        port: 5000,
      ),
    ];
  }
}
