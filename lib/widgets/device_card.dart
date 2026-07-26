import 'package:flutter/material.dart';

class DeviceCard extends StatelessWidget {
  final String name;
  final String address;
  final bool connected;

  const DeviceCard({
    super.key,
    required this.name,
    required this.address,
    required this.connected,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: const Icon(Icons.phone_android),
        title: Text(name),
        subtitle: Text(address),
        trailing: Icon(
          Icons.circle,
          size: 14,
          color: connected ? Colors.green : Colors.grey,
        ),
      ),
    );
  }
}
