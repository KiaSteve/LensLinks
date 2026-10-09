import 'dart:io';

import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import 'ffi/virtual_camera_bridge.dart';
import 'screens/home_screen.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  if (Platform.isWindows) {
    await windowManager.ensureInitialized();

    // Load the native DLL only on Windows
    await VirtualCameraBridge.instance.load();
  }

  runApp(const LensLinksApp());
}

class LensLinksApp extends StatelessWidget {
  const LensLinksApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'LensLinks',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.dark,
        colorSchemeSeed: Colors.blue,
      ),
      home: const HomeScreen(),
    );
  }
}
