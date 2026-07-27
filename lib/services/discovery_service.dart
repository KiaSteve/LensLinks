import '../models/device.dart';
import '../networking/udp_discovery.dart';

class DiscoveryService {
  final UdpDiscovery _udpDiscovery = UdpDiscovery();

  Future<List<Device>> scanNetwork() async {
    return await _udpDiscovery.discover();
  }
}
