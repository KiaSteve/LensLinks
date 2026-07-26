import 'package:flutter/material.dart';

class StatusCard extends StatelessWidget {
  final bool connected;

  const StatusCard({super.key, required this.connected});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(
          Icons.circle,
          color: connected ? Colors.green : Colors.red,
        ),
        title: const Text("Connection Status"),
        subtitle: Text(connected ? "Connected" : "Not Connected"),
      ),
    );
  }
}
