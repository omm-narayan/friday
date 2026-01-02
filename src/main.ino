#define VERSION "\n=== ESP32 AI Voice Assistant ==="

#include <WiFi.h>
#include <SD.h>
#include <Audio.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>

String text;
String filteredAnswer = "";
String repeat;
SimpleTimer Timer;
float batteryVoltage;

const char* ssid = "SSID";
const char* password = "PASS";
const char* OPENAI_KEY = "YOUR_OPENAI_KEY";
const char* gemini_KEY = "YOUR_GEMINI_API_KEY";

#define TTS_MODEL 0

String OpenAI_Max_Tokens = "1000";

#define AUDIO_FILE "/Audio.wav"
#define TTS_GOOGLE_LANGUAGE "en-IN"

#define pin_RECORD_BTN 36
#define pin_repeat 13

#define pin_LED_RED 15
#define pin_LED_GREEN 2
#define pin_LED_BLUE 4

#define pin_I2S_DOUT 25
#define pin_I2S_LRC 26
#define pin_I2S_BCLK 27

Audio audio_play;

bool I2S_Record_Init();
bool Record_Start(String filename);
bool Record_Available(String filename, float* audiolength_sec);
String SpeechToText_Deepgram(String filename);
void Deepgram_KeepAlive();

const int batteryPin = 34;
const float R1 = 100000.0;
const float R2 = 10000.0;
const float adcMax = 4095.0;
const float vRef = 3.4;
const int numSamples = 100;
const float calibrationFactor = 1.48;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(batteryPin, INPUT);
  analogReadResolution(12);

  pinMode(pin_LED_RED, OUTPUT);
  pinMode(pin_LED_GREEN, OUTPUT);
  pinMode(pin_LED_BLUE, OUTPUT);
  pinMode(pin_RECORD_BTN, INPUT);
  pinMode(pin_repeat, INPUT);

  Serial.println(VERSION);

  Timer.setInterval(10000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  if (!SD.begin()) return;

  I2S_Record_Init();

  audio_play.setPinout(pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT);
  audio_play.setVolume(21);
}

void loop() {

start:

  if (digitalRead(pin_RECORD_BTN) == LOW) {
    if (audio_play.isRunning()) audio_play.connecttohost("");
    Record_Start(AUDIO_FILE);
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH) {
    float recorded_seconds;
    if (Record_Available(AUDIO_FILE, &recorded_seconds)) {
      if (recorded_seconds > 0.4) {
        String transcription = SpeechToText_Deepgram(AUDIO_FILE);

        if (transcription == "") {
          speakTextInChunks("Please ask again", 93);
          goto start;
        }

        WiFiClientSecure client;
        client.setInsecure();

        if (client.connect("generativelanguage.googleapis.com", 443)) {
          String url = "/v1beta/models/gemini-2.5-flash:generateContent?key=" + String(gemini_KEY);
          String payload = "{\"contents\":[{\"parts\":[{\"text\":\"" + transcription + "\"}]}]}";

          client.println("POST " + url + " HTTP/1.1");
          client.println("Host: generativelanguage.googleapis.com");
          client.println("Content-Type: application/json");
          client.print("Content-Length: ");
          client.println(payload.length());
          client.println();
          client.println(payload);

          while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") break;
          }

          parseResponse(client.readString());
        }

        if (filteredAnswer != "") {
          speakTextInChunks(filteredAnswer, 93);
        }
      }
    }
  }

  if (digitalRead(pin_repeat) == LOW) {
    speakTextInChunks(repeat, 93);
  }

  audio_play.loop();

  if (Timer.isReady()) {
    battry_filtering();
    if (batteryVoltage < 3.4) speakTextInChunks("Battery low please charge", 93);
    Timer.reset();
  }
}

void speakTextInChunks(String text, int maxLength) {
  int start = 0;
  while (start < text.length()) {
    int end = min(start + maxLength, text.length());
    String chunk = text.substring(start, end);
    audio_play.connecttospeech(chunk.c_str(), TTS_GOOGLE_LANGUAGE);
    while (audio_play.isRunning()) audio_play.loop();
    start = end;
  }
}
