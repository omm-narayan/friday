#include <WiFiClientSecure.h>

#ifndef DEBUG
#define DEBUG true
#define DebugPrint(x); if (DEBUG) { Serial.print(x); }
#define DebugPrintln(x); if (DEBUG) { Serial.println(x); }
#endif

const char* deepgramApiKey = "YOUR_DEEPGRAM_API_KEY";

#define STT_LANGUAGE "en-IN"
#define TIMEOUT_DEEPGRAM 6
#define STT_KEYWORDS "&keywords=KALO&keywords=Janthip&keywords=Google"

WiFiClientSecure client;

String SpeechToText_Deepgram(String audio_filename) {
  uint32_t t_start = millis();

  if (!client.connected()) {
    client.setInsecure();
    if (!client.connect("api.deepgram.com", 443)) {
      client.stop();
      return "";
    }
  }

  File audioFile = SD.open(audio_filename);
  if (!audioFile) return "";
  size_t audio_size = audioFile.size();
  audioFile.close();

  while (client.available()) client.read();

  String optional_param = "?model=nova-2-general";
  optional_param += (String(STT_LANGUAGE) != "") ? "&language=" + String(STT_LANGUAGE) : "&detect_language=true";
  optional_param += "&smart_format=true";
  optional_param += "&numerals=true";
  optional_param += STT_KEYWORDS;

  client.println("POST /v1/listen" + optional_param + " HTTP/1.1");
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: audio/wav");
  client.println("Content-Length: " + String(audio_size));
  client.println();

  File file = SD.open(audio_filename, FILE_READ);
  uint8_t buffer[2048];
  size_t bytesRead;

  while (file.available()) {
    bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead > 0) client.write(buffer, bytesRead);
  }
  file.close();

  String response = "";
  uint32_t t_wait = millis();

  while (response == "" && millis() < (t_wait + TIMEOUT_DEEPGRAM * 1000)) {
    while (client.available()) {
      response += (char)client.read();
    }
    delay(100);
  }

  client.stop();

  String transcription = json_object(response, "\"transcript\":");
  return transcription;
}

int x = 0;

void Deepgram_KeepAlive() {
  uint32_t t_start = millis();

  if (!client.connected()) {
    client.setInsecure();
    if (!client.connect("api.deepgram.com", 443)) {
      x++;
      if (x > 2) esp_restart();
      return;
    }
    return;
  }

  uint8_t empty_wav[] = {
    0x52,0x49,0x46,0x46,0x40,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,
    0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x80,0x3E,0x00,0x00,0x80,0x3E,0x00,0x00,
    0x01,0x00,0x08,0x00,0x64,0x61,0x74,0x61,0x14,0x00,0x00,0x00,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80
  };

  client.println("POST /v1/listen HTTP/1.1");
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: audio/wav");
  client.println("Content-Length: " + String(sizeof(empty_wav)));
  client.println();
  client.write(empty_wav, sizeof(empty_wav));

  while (client.available()) client.read();
}

String json_object(String input, String element) {
  String content = "";
  int pos_start = input.indexOf(element);
  if (pos_start > 0) {
    pos_start += element.length();
    int pos_end = input.indexOf(",\"", pos_start);
    if (pos_end > pos_start) {
      content = input.substring(pos_start, pos_end);
    }
    content.trim();
    if (content.startsWith("\"")) {
      content = content.substring(1, content.length() - 1);
    }
  }
  return content;
}
