import 'dart:convert';

class NetworkMessage {
  final String type;
  final Map<String, dynamic> data;

  const NetworkMessage({required this.type, required this.data});

  String encode() {
    return jsonEncode({"type": type, "data": data});
  }

  factory NetworkMessage.decode(String json) {
    final map = jsonDecode(json);

    return NetworkMessage(
      type: map["type"],
      data: Map<String, dynamic>.from(map["data"]),
    );
  }
}
