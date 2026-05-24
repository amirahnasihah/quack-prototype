#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "pixel_creature.h"

TFT_eSPI tft = TFT_eSPI();

const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

const char* DAEMON_URL = "http://192.168.1.200:8787/usage";

enum DuckState {
  IDLE,
  LISTENING,
  THINKING,
  TALKING
};

DuckState currentState = IDLE;
String transcript = "";
unsigned long lastPoll = 0;
const int POLL_INTERVAL = 3000;

uint8_t animFrame = 0;
unsigned long lastAnimMs = 0;
const uint16_t ANIM_MS = 400;

// Duck palette
uint16_t COLOR_YELLOW = 0;
uint16_t COLOR_BILL   = 0;
const uint16_t COLOR_BG      = 0x0841;
const uint16_t COLOR_PANEL   = 0x1082;
const uint16_t COLOR_LINE    = 0x3186;
const uint16_t COLOR_DIM     = 0x8C71;
const uint16_t COLOR_GREEN   = 0x4EC9;
const uint16_t COLOR_CYAN    = 0x5D9F;

const int PIX_SCALE = 10;
const int PIX_COLS  = 20;
const int PIX_ROWS  = 20;
const int SPRITE_W  = PIX_COLS * PIX_SCALE;
const int SPRITE_H  = PIX_ROWS * PIX_SCALE;
const int PIX_X     = (320 - SPRITE_W) / 2;
const int PIX_Y     = 28;

struct AnimSet {
  const uint8_t* const* frames;
  uint8_t count;
};

AnimSet animFor(DuckState state) {
  AnimSet set = { idle_FRAMES, idle_FRAME_COUNT };
  if (state == LISTENING) {
    set.frames = listen_FRAMES;
    set.count = listen_FRAME_COUNT;
  } else if (state == THINKING) {
    set.frames = think_FRAMES;
    set.count = think_FRAME_COUNT;
  } else if (state == TALKING) {
    set.frames = talk_FRAMES;
    set.count = talk_FRAME_COUNT;
  }
  return set;
}

const uint8_t* frameAt(const AnimSet& set, uint8_t index) {
  return (const uint8_t*)pgm_read_ptr(&set.frames[index % set.count]);
}

void drawPixelGrid(const uint8_t* frame) {
  for (int row = 0; row < PIX_ROWS; row++) {
    for (int col = 0; col < PIX_COLS; col++) {
      const uint8_t cell = pgm_read_byte(&frame[row * PIX_COLS + col]);
      if (cell == PX_EMPTY) continue;

      const int x = PIX_X + col * PIX_SCALE;
      const int y = PIX_Y + row * PIX_SCALE;

      uint16_t color = COLOR_YELLOW;
      if (cell == PX_EYE) color = TFT_BLACK;
      if (cell == PX_BILL) color = COLOR_BILL;

      tft.fillRect(x, y, PIX_SCALE, PIX_SCALE, color);
      tft.drawRect(x, y, PIX_SCALE, PIX_SCALE, TFT_BLACK);
    }
  }
}

void drawHeader() {
  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(12, 8);
  tft.print("QUACK");
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(72, 8);
  tft.print("duck agent");
  tft.drawFastHLine(8, 22, 304, COLOR_LINE);
}

void drawStateLabel(DuckState state) {
  tft.fillRect(8, 178, 304, 22, COLOR_PANEL);
  tft.drawFastHLine(8, 178, 304, COLOR_LINE);
  tft.setTextSize(1);
  tft.setCursor(14, 186);

  if (state == IDLE) {
    tft.setTextColor(TFT_WHITE);
    tft.print("idle");
  } else if (state == LISTENING) {
    tft.setTextColor(COLOR_GREEN);
    tft.print("listening");
  } else if (state == THINKING) {
    tft.setTextColor(COLOR_CYAN);
    tft.print("thinking");
  } else if (state == TALKING) {
    tft.setTextColor(COLOR_BILL);
    tft.print("talking");
  }
}

void drawTranscript(const String& text) {
  tft.fillRect(8, 202, 304, 30, COLOR_BG);
  tft.drawFastHLine(8, 202, 304, COLOR_LINE);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(14, 214);

  String line = text;
  if (line.length() > 44) {
    line = line.substring(0, 41) + "...";
  }
  tft.print(line);
}

void drawScene() {
  tft.fillScreen(COLOR_BG);
  drawHeader();

  const AnimSet set = animFor(currentState);
  drawPixelGrid(frameAt(set, animFrame));

  drawStateLabel(currentState);
  if (transcript.length() > 0) {
    drawTranscript(transcript);
  }
}

void resetAnimation() {
  animFrame = 0;
  lastAnimMs = millis();
}

void tickAnimation() {
  const unsigned long now = millis();
  if (now - lastAnimMs < ANIM_MS) return;

  lastAnimMs = now;
  const AnimSet set = animFor(currentState);
  animFrame = (animFrame + 1) % set.count;

  tft.fillRect(PIX_X - 2, PIX_Y - 2, SPRITE_W + 4, SPRITE_H + 4, COLOR_BG);
  drawPixelGrid(frameAt(set, animFrame));
}

void setState(DuckState state, const String& line) {
  const bool stateChanged = state != currentState;
  currentState = state;
  transcript = line;

  if (stateChanged) {
    resetAnimation();
    drawScene();
    return;
  }

  if (line.length() > 0) {
    drawTranscript(line);
  }
}

void applyDaemonPayload(const String& payload) {
  JsonDocument doc;
  if (deserializeJson(doc, payload)) {
    Serial.println("Daemon: bad JSON");
    return;
  }

  const bool ok = doc["ok"] | false;
  const char* st = doc["st"] | "?";
  const int s = doc["s"] | 0;
  const int w = doc["w"] | 0;
  const int cs = doc["cs"] | 0;
  const int cw = doc["cw"] | 0;

  const String nextTranscript =
    String(st) + "  s:" + String(s) + " w:" + String(w);

  DuckState nextState = IDLE;
  if (!ok || cs > 0 || cw > 0) {
    nextState = THINKING;
  }

  const bool changed =
    nextState != currentState || nextTranscript != transcript;

  if (!changed) return;

  setState(nextState, nextTranscript);
}

void pollDaemon() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(DAEMON_URL);
  const int code = http.GET();

  if (code == 200) {
    const String payload = http.getString();
    Serial.println("Daemon: " + payload);
    applyDaemonPayload(payload);
  } else {
    Serial.println("Daemon HTTP " + String(code));
  }
  http.end();
}

void setup() {
  Serial.begin(115200);

  COLOR_YELLOW = tft.color565(255, 220, 0);
  COLOR_BILL   = tft.color565(255, 140, 0);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(COLOR_BG);

  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(72, 96);
  tft.print("QUACK");
  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(64, 124);
  tft.print("connecting wifi...");

  WiFi.begin(SSID, PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    tft.setCursor(88, 142);
    tft.setTextColor(COLOR_GREEN);
    tft.print("wifi ok");
  } else {
    Serial.println("\nWiFi failed");
    tft.setCursor(72, 142);
    tft.setTextColor(TFT_RED);
    tft.print("wifi offline");
  }

  delay(1200);
  resetAnimation();
  drawScene();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastPoll > POLL_INTERVAL) {
    lastPoll = now;
    pollDaemon();
  }

  tickAnimation();

  if (Serial.available()) {
    const char c = Serial.read();
    if (c == '1') {
      setState(IDLE, "");
    } else if (c == '2') {
      setState(LISTENING, "mic open...");
    } else if (c == '3') {
      setState(THINKING, "processing...");
    } else if (c == '4') {
      setState(TALKING, "speaking...");
    }
  }

  delay(20);
}
