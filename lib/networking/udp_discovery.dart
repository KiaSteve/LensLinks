import 'dart:async';
import 'dart:convert';
import 'dart:io';

import '../models/device.dart';
import 'protocol.dart';

class UdpDiscovery {
  RawDatagramSocket? _socket;
  StreamSubscription<RawSocketEvent>? _subscription;

  /// Addresses to send the discovery request to: for every IPv4 address of
  /// this PC, the ".255" address of its /24 network, plus the limited
  /// broadcast address. Dart does not expose subnet masks, so /24 is assumed,
  /// which fits nearly all home networks; 255.255.255.255 covers the rest.
  Future<List<InternetAddress>> _broadcastTargets() async {
    final targets = <String>{};

    try {
      final adapters = await NetworkInterface.list(
        type: InternetAddressType.IPv4,
        includeLoopback: false,
        includeLinkLocal: false,
      );

      for (final nic in adapters) {
        for (final address in nic.addresses) {
          final parts = address.address.split('.');

          if (parts.length == 4) {
            targets.add('${parts[0]}.${parts[1]}.${parts[2]}.255');
          }
        }
      }
    } catch (e) {
      print("Could not list network adapters: $e");
    }

    // Nothing found: fall back to the address this app always used.
    if (targets.isEmpty) {
      targets.add('192.168.0.255');
    }

    targets.add('255.255.255.255');

    return targets.map((t) => InternetAddress(t)).toList();
  }

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

    final targets = await _broadcastTargets();
    final sentTo = <String>[];

    for (final target in targets) {
      try {
        _socket!.send(
          utf8.encode(discoveryRequest),
          target,
          discoveryPort,
        );

        sentTo.add(target.address);
      } catch (e) {
        // A network that cannot be reached must not stop the others.
        print("Discovery send to ${target.address} failed: $e");
      }
    }

    print("Discovery broadcast sent to ${sentTo.join(', ')}");

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
