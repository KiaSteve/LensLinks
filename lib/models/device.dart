class Device {
  final String id;
  final String name;
  final String ip;
  final int port;
  final bool connected;

  const Device({
    required this.id,
    required this.name,
    required this.ip,
    required this.port,
    this.connected = false,
  });

  Device copyWith({bool? connected}) {
    return Device(
      id: id,
      name: name,
      ip: ip,
      port: port,
      connected: connected ?? this.connected,
    );
  }
}
