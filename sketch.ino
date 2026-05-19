#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>

TFT_eSPI tft = TFT_eSPI();

// WiFi credentials — change these
const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

// MacBook daemon URL — change IP to your Mac's IP
const char* DAEMON_URL = "http://192.168.1.100:8787/usage";

// Duck states
enum DuckState {
  IDLE,
  LISTENING,
  THINKING,
  TALKING
};

DuckState currentState = IDLE;
String transcript = "";
unsigned long lastPoll = 0;
const int POLL_INTERVAL = 3000; // poll every 3s

// ── Colours ──────────────────────────────
#define BG_COLOR      TFT_BLACK
#define DUCK_YELLOW   0xFFE0
#define DUCK_ORANGE   0xFD20
#define TEXT_WHITE    TFT_WHITE
#define TEXT_GREEN    TFT_GREEN
#define TEXT_CYAN     TFT_CYAN

// ── Draw Duck Face ────────────────────────
void drawDuckFace(DuckState state) {
  tft.fillScreen(BG_COLOR);

  // Head (big yellow circle)
  tft.fillCircle(160, 100, 70, DUCK_YELLOW);

  // Bill (orange oval)
  tft.fillEllipse(160, 145, 25, 14, DUCK_ORANGE);

  // Eyes based on state
  if (state == IDLE) {
    // Normal eyes — open circles
    tft.fillCircle(135, 85, 12, TFT_WHITE);
    tft.fillCircle(185, 85, 12, TFT_WHITE);
    tft.fillCircle(137, 87, 6, TFT_BLACK);
    tft.fillCircle(187, 87, 6, TFT_BLACK);

  } else if (state == LISTENING) {
    // Wide eyes — bigger
    tft.fillCircle(135, 85, 14, TFT_WHITE);
    tft.fillCircle(185, 85, 14, TFT_WHITE);
    tft.fillCircle(135, 85, 7, TFT_BLACK);
    tft.fillCircle(185, 85, 7, TFT_BLACK);
    // Eyebrows raised
    tft.drawLine(125, 68, 148, 65, TFT_BLACK);
    tft.drawLine(175, 65, 198, 68, TFT_BLACK);

  } else if (state == THINKING) {
    // One eye squinting
    tft.fillCircle(135, 85, 12, TFT_WHITE);
    tft.fillCircle(185, 85, 12, TFT_WHITE);
    tft.fillCircle(137, 87, 6, TFT_BLACK);
    tft.fillCircle(187, 87, 6, TFT_BLACK);
    // Thinking brow
    tft.drawLine(125, 72, 148, 75, TFT_BLACK);
    tft.drawLine(175, 75, 198, 72, TFT_BLACK);
    // Sweat drop
    tft.fillCircle(210, 70, 5, TFT_CYAN);

  } else if (state == TALKING) {
    // Happy eyes — curved lines
    tft.fillCircle(135, 85, 12, TFT_WHITE);
    tft.fillCircle(185, 85, 12, TFT_WHITE);
    tft.fillCircle(137, 87, 6, TFT_BLACK);
    tft.fillCircle(187, 87, 6, TFT_BLACK);
    // Open bill (mouth open)
    tft.fillEllipse(160, 148, 25, 16, DUCK_ORANGE);
    tft.fillEllipse(160, 150, 18, 8, TFT_DARKGREY);
  }

  // State label
  drawStateLabel(state);

  // Transcript
  if (transcript.length() > 0) {
    drawTranscript(transcript);
  }
}

// ── Draw State Label ──────────────────────
void drawStateLabel(DuckState state) {
  tft.setTextSize(2);
  tft.setCursor(10, 185);

  switch(state) {
    case IDLE:
      tft.setTextColor(TEXT_WHITE);
      tft.print("  Idle...");
      break;
    case LISTENING:
      tft.setTextColor(TEXT_GREEN);
      tft.print("  Listening...");
      break;
    case THINKING:
      tft.setTextColor(TEXT_CYAN);
      tft.print("  Thinking...");
      break;
    case TALKING:
      tft.setTextColor(DUCK_YELLOW);
      tft.print("  Talking!");
      break;
  }
}

// ── Draw Transcript ───────────────────────
void drawTranscript(String text) {
  tft.setTextSize(1);
  tft.setTextColor(TEXT_WHITE);
  tft.setCursor(5, 215);
  // Truncate if too long
  if (text.length() > 50) {
    text = text.substring(0, 47) + "...";
  }
  tft.print(text);
}

// ── Touch Check ───────────────────────────
bool isTouched() {
  // Wokwi: simulate touch via serial
  // Real CYD: use XPT2046 library
  return false; // placeholder
}

// ── Poll Daemon ───────────────────────────
void pollDaemon() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(DAEMON_URL);
  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();
    Serial.println("Daemon: " + payload);
    // TODO: parse JSON and update state
  }
  http.end();
}

// ── Setup ─────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Init display
  tft.init();
  tft.setRotation(1); // landscape
  tft.fillScreen(BG_COLOR);

  // Boot screen
  tft.setTextColor(DUCK_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(60, 100);
  tft.print("Duck Agent");
  tft.setTextSize(1);
  tft.setTextColor(TEXT_WHITE);
  tft.setCursor(80, 140);
  tft.print("Connecting to WiFi...");

  // Connect WiFi
  WiFi.begin(SSID, PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    tft.setCursor(80, 160);
    tft.setTextColor(TEXT_GREEN);
    tft.print("WiFi OK!");
  } else {
    Serial.println("\nWiFi failed");
    tft.setCursor(80, 160);
    tft.setTextColor(TFT_RED);
    tft.print("WiFi failed - offline mode");
  }

  delay(1500);

  // Draw initial duck
  drawDuckFace(IDLE);
}

// ── Loop ──────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Poll daemon every 3s
  if (now - lastPoll > POLL_INTERVAL) {
    lastPoll = now;
    pollDaemon();
  }

  // Demo: cycle through states via Serial input
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '1') {
      currentState = IDLE;
      transcript = "";
      drawDuckFace(IDLE);
    } else if (c == '2') {
      currentState = LISTENING;
      transcript = "Listening for voice...";
      drawDuckFace(LISTENING);
    } else if (c == '3') {
      currentState = THINKING;
      transcript = "Processing your question...";
      drawDuckFace(THINKING);
    } else if (c == '4') {
      currentState = TALKING;
      transcript = "Here's what I think!";
      drawDuckFace(TALKING);
    }
  }

  delay(100);
}