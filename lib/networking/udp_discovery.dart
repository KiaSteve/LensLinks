import 'dart:async';
import 'dart:convert';
import 'dart:io';

import '../models/device.dart';
import 'protocol.dart';

class UdpDiscovery {
  RawDatagramSocket? _socket;
  StreamSubscription<RawSocketEvent>? _subscription;

  Future<List<Device>> discover() async {
    // Clean up any previous scan
    await _subscription?.cancel();
    _socket?.close();

    final devices = <Device>[];

    _socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);

    _socket!.broadcastEnabled = true;

    final completer = Completer<List<Device>>();

    _subscription = _socket!.listen((event) {
      if (event != RawSocketEvent.read) return;

      final datagram = _socket?.receive();

      if (datagram == null) return;

      final message = utf8.decode(datagram.data);

      print(
        "Received '$message' from ${datagram.address.address}:${datagram.port}",
      );

      if (message == discoveryResponse) {
        if (!devices.any((d) => d.ip == datagram.address.address)) {
          devices.add(
            Device(
              id: datagram.address.address,
              name: "Android Phone",
              ip: datagram.address.address,
              port: discoveryPort,
            ),
          );

          print("Found ${devices.length} device(s)");
        }
      }
    });

    _socket!.send(
      utf8.encode(discoveryRequest),
      InternetAddress("192.168.0.255"),
      discoveryPort,
    );

    print("Discovery broadcast sent to 192.168.0.255");

    Future.delayed(const Duration(seconds: 5), () async {
      await _subscription?.cancel();
      _subscription = null;

      _socket?.close();
      _socket = null;

      if (!completer.isCompleted) {
        completer.complete(devices);
      }
    });

    return completer.future;
  }
}
