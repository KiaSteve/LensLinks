import 'package:flutter/material.dart';
import 'screens/home_screen.dart';

void main() {
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
