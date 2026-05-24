# 1 "/var/folders/lc/hq4vdwxx7rl60tn07sgj8cyw0000gn/T/tmpcyrohzj5"
#include <Arduino.h>
# 1 "/Users/amirahnasihah/Developer/AMRHNSHH/quack-prototype/sketch.ino"
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <math.h>
#include <SPI.h>
#include "pixel_creature.h"


#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchSpi(VSPI);
const SPISettings touchSpiSettings(2000000, MSBFIRST, SPI_MODE0);
bool touchReady = false;
# 34 "/Users/amirahnasihah/Developer/AMRHNSHH/quack-prototype/sketch.ino"
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE 16000
#define I2S_MIC_SCK 22
#define I2S_MIC_WS 27
#define I2S_MIC_SD 35
#define I2S_BUFFER_SAMPLES 256

TFT_eSPI tft = TFT_eSPI();

const char* SSID = "Wokwi-GUEST";
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


uint16_t COLOR_YELLOW = 0;
uint16_t COLOR_BILL = 0;
const uint16_t COLOR_BG = 0x0841;
const uint16_t COLOR_PANEL = 0x1082;
const uint16_t COLOR_LINE = 0x3186;
const uint16_t COLOR_DIM = 0x8C71;
const uint16_t COLOR_GREEN = 0x4EC9;
const uint16_t COLOR_CYAN = 0x5D9F;

const int PIX_SCALE = 9;
const int PIX_COLS = 20;
const int PIX_ROWS = 20;
const int SPRITE_W = PIX_COLS * PIX_SCALE;
const int SPRITE_H = PIX_ROWS * PIX_SCALE;
const int PIX_X = (320 - SPRITE_W) / 2;
const int AGENT_TOP = 8;
const int STATUS_Y = 192;
const int STATUS_H = 22;
const int TRANSCRIPT_Y = 218;
const int DOTS_Y = 232;
const int PAGE_TAP_Y_MIN = DOTS_Y - 24;
const int PIX_Y = AGENT_TOP + ((STATUS_Y - AGENT_TOP - SPRITE_H) / 2);

struct AnimSet {
  const uint8_t* const* frames;
  uint8_t count;
};
DuckState duckStateFromText(const char* text);
void initMic();
int readMicSimFallback();
bool isWokwiSim();
int readMicLevel();
void pollMic();
AnimSet animFor(DuckState state);
const uint8_t* frameAt(const AnimSet& set, uint8_t index);
void drawPixelGrid(const uint8_t* frame);
void drawHeader();
void drawPageDots();
void drawDetailsPage();
void drawStateLabel(DuckState state);
void clearFooterArea();
void drawFooter();
void drawTranscript(const String& text);
void drawScene();
void resetAnimation();
void tickAnimation();
void initTouch();
int16_t xpt2046BestTwoAvg(int16_t a, int16_t b, int16_t c);
bool readTouchRaw(int16_t& xraw, int16_t& yraw, int16_t& zraw);
bool readTouchPoint(int16_t& outX, int16_t& outY);
void handleTapRelease(int16_t x, int16_t y, int dx, int dy);
void pollTouchSwipe();
void applyDaemonPayload(const String& payload);
void pollDaemon();
void setup();
void loop();
#line 120 "/Users/amirahnasihah/Developer/AMRHNSHH/quack-prototype/sketch.ino"
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
  const int cy = DOTS_Y;
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
  if (state == IDLE) return;

  tft.fillRect(8, STATUS_Y, 304, STATUS_H, COLOR_PANEL);
  tft.drawFastHLine(8, STATUS_Y, 304, COLOR_LINE);
  tft.drawFastHLine(8, STATUS_Y + STATUS_H - 1, 304, COLOR_LINE);
  tft.setTextSize(1);
  tft.setCursor(14, STATUS_Y + 6);

  if (state == LISTENING) {
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

void clearFooterArea() {
  tft.fillRect(8, STATUS_Y, 304, DOTS_Y - STATUS_Y + 8, COLOR_BG);
}

void drawFooter() {
  clearFooterArea();
  drawStateLabel(currentState);

  if (currentState != IDLE && transcript.length() > 0) {
    drawTranscript(transcript);
  }

  drawPageDots();
}

void drawTranscript(const String& text) {
  tft.fillRect(8, TRANSCRIPT_Y, 304, 18, COLOR_BG);
  tft.drawFastHLine(8, TRANSCRIPT_Y, 304, COLOR_LINE);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM);
  tft.setCursor(14, TRANSCRIPT_Y + 6);

  String line = text;
  if (line.length() > 44) {
    line = line.substring(0, 41) + "...";
  }
  tft.print(line);
}

void drawScene() {
  tft.fillScreen(COLOR_BG);

  const AnimSet set = animFor(currentState);
  drawPixelGrid(frameAt(set, animFrame));

  drawFooter();
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
  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  touchReady = true;
  Serial.println("Touch: XPT2046 ready — swipe or tap dots at bottom");
}

int16_t xpt2046BestTwoAvg(int16_t a, int16_t b, int16_t c) {
  const int16_t da = abs(a - b);
  const int16_t db = abs(a - c);
  const int16_t dc = abs(b - c);
  if (da <= db && da <= dc) return (a + b) / 2;
  if (db <= da && db <= dc) return (a + c) / 2;
  return (b + c) / 2;
}

bool readTouchRaw(int16_t& xraw, int16_t& yraw, int16_t& zraw) {
  int16_t data[6] = { 0, 0, 0, 0, 0, 0 };

  touchSpi.beginTransaction(touchSpiSettings);
  digitalWrite(XPT2046_CS, LOW);
  touchSpi.transfer(0xB1);
  const int16_t z1 = touchSpi.transfer16(0xC1) >> 3;
  int z = z1 + 4095;
  const int16_t z2 = touchSpi.transfer16(0x91) >> 3;
  z -= z2;

  if (z >= 400) {
    touchSpi.transfer16(0x91);
    data[0] = touchSpi.transfer16(0xD1) >> 3;
    data[1] = touchSpi.transfer16(0x91) >> 3;
    data[2] = touchSpi.transfer16(0xD1) >> 3;
    data[3] = touchSpi.transfer16(0x91) >> 3;
  }

  data[4] = touchSpi.transfer16(0xD0) >> 3;
  touchSpi.transfer16(0);
  digitalWrite(XPT2046_CS, HIGH);
  touchSpi.endTransaction();

  if (z < 0) z = 0;
  if (z < 400) {
    zraw = 0;
    return false;
  }

  zraw = z;
  const int16_t x = xpt2046BestTwoAvg(data[0], data[2], data[4]);
  const int16_t y = xpt2046BestTwoAvg(data[1], data[3], data[5]);
  xraw = 4095 - x;
  yraw = 4095 - y;
  return true;
}

bool readTouchPoint(int16_t& outX, int16_t& outY) {
  int16_t xraw = 0;
  int16_t yraw = 0;
  int16_t zraw = 0;
  if (!readTouchRaw(xraw, yraw, zraw)) {
    return false;
  }

  outX = map(xraw, 200, 3700, 0, 320);
  outY = map(yraw, 240, 3800, 0, 240);
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

  if (currentState != IDLE) {
    drawFooter();
  }
}

void applyDaemonPayload(const String& payload) {
  JsonDocument doc;
  if (deserializeJson(doc, payload)) {
    Serial.println("Daemon: bad JSON");
    return;
  }

  const char* duckState = doc["duckState"] | "";
  const char* duckLine = doc["transcript"] | "";


  if (duckState[0] == '\0') {
    return;
  }

  DuckState nextState = duckStateFromText(duckState);
  String nextTranscript = duckLine[0] != '\0' ? String(duckLine) : "";

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
  COLOR_BILL = tft.color565(255, 140, 0);

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