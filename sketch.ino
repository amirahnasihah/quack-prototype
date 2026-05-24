#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <math.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "pixel_creature.h"

// CYD XPT2046 — separate SPI bus from display (TFT_eSPI getTouch does not work here)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touchScreen(XPT2046_CS, XPT2046_IRQ);
bool touchReady = false;

// INMP441 on CYD — two JST/dupont headers
//
// CN1 (4-pin): 1=3.3V  2=IO27  3=IO22  4=GND  ← power + WS + SCK + GND
// P3  (4-pin): 1=IO21  2=IO22  3=IO35  4=GND  ← SD on pin 3 (IO35)
//
// INMP441 → CYD (wire colours):
//   VDD  red    → CN1 pin 1 (3.3V)
//   WS   blue   → CN1 pin 2 (IO27)
//   SCK  yellow → CN1 pin 3 (IO22)  [same as P3 pin 2]
//   GND  black  → CN1 pin 4 or P3 pin 4
//   L/R  black  → GND (mono left)
//   SD   green  → P3 pin 3 (IO35)   ← extra dupont jumper
//
// IO21 = TFT backlight — jangan guna untuk WS
#define I2S_PORT          I2S_NUM_0
#define I2S_SAMPLE_RATE   16000
#define I2S_MIC_SCK       22
#define I2S_MIC_WS        27
#define I2S_MIC_SD        35
#define I2S_BUFFER_SAMPLES 256

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

enum UiPage : uint8_t {
  PAGE_AGENT = 0,
  PAGE_DETAILS = 1,
  PAGE_COUNT = 2
};

struct TouchTrack {
  bool active;
  int16_t startX;
  int16_t startY;
  int16_t lastX;
  int16_t lastY;
};

DuckState currentState = IDLE;
UiPage currentPage = PAGE_AGENT;
TouchTrack touchTrack = { false, 0, 0, 0, 0 };

const int SWIPE_MIN_PX = 28;
const int PAGE_TAP_Y_MIN = 210;
String transcript = "";
unsigned long lastPoll = 0;
const int POLL_INTERVAL = 3000;
unsigned long lastMicMs = 0;
const int MIC_INTERVAL = 500;

bool micReady = false;

void setState(DuckState state, const String& line);
void setPage(UiPage page);
void nextPage();
void prevPage();

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

DuckState duckStateFromText(const char* text) {
  if (!text || text[0] == '\0') return currentState;
  if (strcmp(text, "idle") == 0) return IDLE;
  if (strcmp(text, "listening") == 0) return LISTENING;
  if (strcmp(text, "thinking") == 0) return THINKING;
  if (strcmp(text, "talking") == 0) return TALKING;
  return currentState;
}

void initMic() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_BUFFER_SAMPLES,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD
  };

  if (i2s_driver_install(I2S_PORT, &config, 0, NULL) != ESP_OK) {
    Serial.println("Mic: I2S install failed");
    return;
  }
  if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) {
    Serial.println("Mic: pin config failed");
    return;
  }
  i2s_set_clk(I2S_PORT, I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  i2s_zero_dma_buffer(I2S_PORT);
  micReady = true;
  Serial.println("Mic: INMP441 ready (SCK=22 WS=27 SD=35)");
}

int readMicSimFallback() {
  return 600 + (int)(fabs(sin(millis() / 180.0)) * 400.0);
}

bool isWokwiSim() {
  return WiFi.SSID() == "Wokwi-GUEST";
}

int readMicLevel() {
  int16_t samples[I2S_BUFFER_SAMPLES];
  size_t bytesRead = 0;

  const esp_err_t readOk =
    i2s_read(I2S_PORT, samples, sizeof(samples), &bytesRead, pdMS_TO_TICKS(50));

  if (readOk != ESP_OK) {
    static bool readErrLogged = false;
    if (!readErrLogged && isWokwiSim()) {
      readErrLogged = true;
      Serial.println("Mic: i2s_read failed — sim fallback active");
    }
    return isWokwiSim() ? readMicSimFallback() : 0;
  }

  const size_t count = bytesRead / sizeof(int16_t);
  if (count == 0) {
    static bool emptyLogged = false;
    if (!emptyLogged && isWokwiSim()) {
      emptyLogged = true;
      Serial.println("Mic: i2s_read empty — sim fallback active");
    }
    return isWokwiSim() ? readMicSimFallback() : 0;
  }

  int64_t sum = 0;
  int32_t peak = 0;
  for (size_t i = 0; i < count; i++) {
    const int32_t sample = samples[i];
    const int32_t absSample = sample < 0 ? -sample : sample;
    sum += absSample;
    if (absSample > peak) peak = absSample;
  }

  if (peak == 0 && isWokwiSim()) {
    static bool silentLogged = false;
    if (!silentLogged) {
      silentLogged = true;
      Serial.println("Mic: I2S silent — sim fallback active");
    }
    return readMicSimFallback();
  }

  return (int)(sum / count);
}

void pollMic() {
  if (!micReady || currentState != LISTENING) return;

  const unsigned long now = millis();
  if (now - lastMicMs < MIC_INTERVAL) return;
  lastMicMs = now;

  const int level = readMicLevel();
  Serial.println("Mic level: " + String(level));
  setState(LISTENING, "mic lvl " + String(level));
}

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
  tft.print(currentPage == PAGE_AGENT ? "duck agent" : "model info");
  tft.drawFastHLine(8, 22, 304, COLOR_LINE);
}

void drawPageDots() {
  const int cy = 232;
  const int cx = 160;
  const uint16_t active = COLOR_YELLOW;
  const uint16_t idle = COLOR_LINE;

  tft.fillCircle(cx - 8, cy, currentPage == PAGE_AGENT ? 3 : 2,
    currentPage == PAGE_AGENT ? active : idle);
  tft.fillCircle(cx + 8, cy, currentPage == PAGE_DETAILS ? 3 : 2,
    currentPage == PAGE_DETAILS ? active : idle);
}

void drawDetailsPage() {
  tft.fillScreen(COLOR_BG);
  drawHeader();

  tft.fillRect(8, 30, 304, 188, COLOR_PANEL);
  tft.drawRect(8, 30, 304, 188, COLOR_LINE);

  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(18, 44);
  tft.print("MODEL");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(18, 72);
  tft.print("board");
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(88, 72);
  tft.print("esp32-2432s028r");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(18, 88);
  tft.print("mic");
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(88, 88);
  tft.print("inmp441 @ 16kHz");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(18, 104);
  tft.print("display");
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(88, 104);
  tft.print("ili9341 320x240");

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(18, 120);
  tft.print("agent");
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(88, 120);
  tft.print("quack prototype v0");

  tft.drawFastHLine(18, 136, 284, COLOR_LINE);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(18, 148);
  tft.print("daemon poll + pixel duck ui");

  tft.setTextColor(COLOR_DIM);
  tft.setCursor(18, 168);
  tft.print("swipe left/right to change page");
  tft.setCursor(18, 182);
  tft.print("sim: serial [ prev  ] next");

  tft.setTextColor(COLOR_BILL);
  tft.setCursor(18, 204);
  tft.print("by amirahnasihah");

  drawPageDots();
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
  drawPageDots();
}

void resetAnimation() {
  animFrame = 0;
  lastAnimMs = millis();
}

void tickAnimation() {
  if (currentPage != PAGE_AGENT) return;

  const unsigned long now = millis();
  if (now - lastAnimMs < ANIM_MS) return;

  lastAnimMs = now;
  const AnimSet set = animFor(currentState);
  animFrame = (animFrame + 1) % set.count;

  tft.fillRect(PIX_X - 2, PIX_Y - 2, SPRITE_W + 4, SPRITE_H + 4, COLOR_BG);
  drawPixelGrid(frameAt(set, animFrame));
}

void setPage(UiPage page) {
  if (page == currentPage) return;
  currentPage = page;

  if (currentPage == PAGE_AGENT) {
    resetAnimation();
    drawScene();
    Serial.println("Page: agent");
    return;
  }

  drawDetailsPage();
  Serial.println("Page: details");
}

void nextPage() {
  const uint8_t next = (currentPage + 1) % PAGE_COUNT;
  setPage(static_cast<UiPage>(next));
}

void prevPage() {
  const uint8_t prev = (currentPage + PAGE_COUNT - 1) % PAGE_COUNT;
  setPage(static_cast<UiPage>(prev));
}

void initTouch() {
  touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchScreen.begin(touchSpi);
  touchScreen.setRotation(3);
  touchReady = true;
  Serial.println("Touch: XPT2046 ready — swipe or tap dots at bottom");
}

bool readTouchPoint(int16_t& outX, int16_t& outY) {
  if (!touchReady || !touchScreen.touched()) {
    return false;
  }

  const TS_Point p = touchScreen.getPoint();
  outX = map(p.x, 200, 3700, 0, 320);
  outY = map(p.y, 240, 3800, 0, 240);
  return true;
}

void handleTapRelease(int16_t x, int16_t y, int dx, int dy) {
  const bool isTap =
    abs(dx) < SWIPE_MIN_PX && abs(dy) < SWIPE_MIN_PX;

  if (isTap && y >= PAGE_TAP_Y_MIN) {
    if (x < 150) {
      prevPage();
      return;
    }
    if (x > 170) {
      nextPage();
      return;
    }
  }

  if (abs(dx) >= SWIPE_MIN_PX && abs(dx) > abs(dy)) {
    if (dx < 0) {
      nextPage();
    } else {
      prevPage();
    }
  }
}

void pollTouchSwipe() {
  if (!touchReady) return;

  int16_t x = 0;
  int16_t y = 0;
  const bool pressed = readTouchPoint(x, y);

  if (pressed && !touchTrack.active) {
    touchTrack.active = true;
    touchTrack.startX = x;
    touchTrack.startY = y;
    touchTrack.lastX = x;
    touchTrack.lastY = y;
    return;
  }

  if (pressed && touchTrack.active) {
    touchTrack.lastX = x;
    touchTrack.lastY = y;
    return;
  }

  if (!pressed && touchTrack.active) {
    touchTrack.active = false;
    const int dx = touchTrack.lastX - touchTrack.startX;
    const int dy = touchTrack.lastY - touchTrack.startY;

    static bool touchDebugOnce = false;
    if (!touchDebugOnce) {
      touchDebugOnce = true;
      Serial.println(
        "Touch at " + String(touchTrack.lastX) + "," + String(touchTrack.lastY) +
        " dx=" + String(dx)
      );
    }

    handleTapRelease(touchTrack.lastX, touchTrack.lastY, dx, dy);
  }
}

void setState(DuckState state, const String& line) {
  const bool stateChanged = state != currentState;
  currentState = state;
  transcript = line;

  if (currentPage != PAGE_AGENT) return;

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

  const char* duckState = doc["duckState"] | "";
  const char* duckLine = doc["transcript"] | "";

  DuckState nextState = duckState[0] != '\0'
    ? duckStateFromText(duckState)
    : IDLE;

  if (duckState[0] == '\0') {
    if (!ok || cs > 0 || cw > 0) {
      nextState = THINKING;
    }
  }

  String nextTranscript = duckLine[0] != '\0'
    ? String(duckLine)
    : String(st) + "  s:" + String(s) + " w:" + String(w);

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
  initMic();
  initTouch();
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
  pollMic();
  pollTouchSwipe();

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
    } else if (c == ']' || c == 'n') {
      nextPage();
    } else if (c == '[' || c == 'p') {
      prevPage();
    }
  }

  delay(20);
}
