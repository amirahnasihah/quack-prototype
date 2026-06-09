// ============================================================================
//  voice_agent.cpp  -  OpenAI implementation of the STT -> chat -> TTS loop.
//  All three steps go through api.openai.com over HTTPS using one API key.
//
//  Swap providers by rewriting only the three functions below:
//    transcribe()  -> any speech-to-text endpoint
//    chat()        -> any chat/LLM endpoint
//    synthesize()  -> any text-to-speech endpoint (e.g. ElevenLabs for a
//                     richer voice -- see README "Swapping the voice").
// ============================================================================
#include "voice_agent.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace agent {

// ---- rolling conversation memory ------------------------------------------
// Stored as alternating user/assistant strings. Capped at MEMORY_TURNS*2.
static String history[MEMORY_TURNS * 2];
static int    historyCount = 0;

static void pushHistory(const char *role, const String &content) {
  // role is implied by position (even=user, odd=assistant) so we just store
  // text and prefix with role marker for clarity when rebuilding the request.
  String tagged = String(role) + "\x1f" + content;   // 0x1f = unit separator
  if (historyCount < MEMORY_TURNS * 2) {
    history[historyCount++] = tagged;
  } else {
    for (int i = 1; i < MEMORY_TURNS * 2; i++) history[i - 1] = history[i];
    history[MEMORY_TURNS * 2 - 1] = tagged;
  }
}

void resetMemory() { historyCount = 0; }

// ---------------------------------------------------------------------------
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[wifi] connecting to %s", WIFI_SSID);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] connected, IP %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  Serial.println("[wifi] FAILED");
  return false;
}

// Build a 44-byte WAV header for 16-bit mono PCM -- OpenAI's STT wants a real
// audio container, not raw samples.
static void writeWavHeader(uint8_t *h, uint32_t dataLen, uint32_t sampleRate) {
  uint32_t byteRate = sampleRate * 2;     // mono, 16-bit
  uint32_t chunkSize = 36 + dataLen;
  memcpy(h, "RIFF", 4);
  h[4]=chunkSize&0xff; h[5]=(chunkSize>>8)&0xff; h[6]=(chunkSize>>16)&0xff; h[7]=(chunkSize>>24)&0xff;
  memcpy(h + 8, "WAVEfmt ", 8);
  h[16]=16; h[17]=0; h[18]=0; h[19]=0;    // fmt chunk size
  h[20]=1;  h[21]=0;                      // PCM
  h[22]=1;  h[23]=0;                      // mono
  h[24]=sampleRate&0xff; h[25]=(sampleRate>>8)&0xff; h[26]=(sampleRate>>16)&0xff; h[27]=(sampleRate>>24)&0xff;
  h[28]=byteRate&0xff; h[29]=(byteRate>>8)&0xff; h[30]=(byteRate>>16)&0xff; h[31]=(byteRate>>24)&0xff;
  h[32]=2; h[33]=0;                       // block align
  h[34]=16; h[35]=0;                      // bits per sample
  memcpy(h + 36, "data", 4);
  h[40]=dataLen&0xff; h[41]=(dataLen>>8)&0xff; h[42]=(dataLen>>16)&0xff; h[43]=(dataLen>>24)&0xff;
}

// ---------------------------------------------------------------------------
String transcribe(const uint8_t *pcm, size_t pcmBytes) {
  WiFiClientSecure client;
  client.setInsecure();   // TODO(prod): pin OpenAI's root CA instead.
  HTTPClient https;
  if (!https.begin(client, "https://api.openai.com/v1/audio/transcriptions")) return "";

  const char *boundary = "----esp32herboundary";
  // multipart parts: model field + file field (wav header + pcm) + closing
  String head =
    String("--") + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"model\"\r\n\r\n" + STT_MODEL + "\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"file\"; filename=\"a.wav\"\r\n"
    "Content-Type: audio/wav\r\n\r\n";
  String tail = String("\r\n--") + boundary + "--\r\n";

  uint8_t wav[44];
  writeWavHeader(wav, pcmBytes, SAMPLE_RATE);

  size_t total = head.length() + sizeof(wav) + pcmBytes + tail.length();
  uint8_t *body = (uint8_t *)ps_malloc(total);   // PSRAM
  if (!body) { https.end(); return ""; }
  size_t o = 0;
  memcpy(body + o, head.c_str(), head.length()); o += head.length();
  memcpy(body + o, wav, sizeof(wav));            o += sizeof(wav);
  memcpy(body + o, pcm, pcmBytes);               o += pcmBytes;
  memcpy(body + o, tail.c_str(), tail.length()); o += tail.length();

  https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
  https.addHeader("Content-Type", String("multipart/form-data; boundary=") + boundary);
  int code = https.POST(body, total);
  String text;
  if (code == 200) {
    JsonDocument doc;
    if (!deserializeJson(doc, https.getString())) text = doc["text"].as<String>();
  } else {
    Serial.printf("[stt] HTTP %d: %s\n", code, https.getString().c_str());
  }
  free(body);
  https.end();
  text.trim();
  return text;
}

// ---------------------------------------------------------------------------
String chat(const String &userText) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  if (!https.begin(client, "https://api.openai.com/v1/chat/completions")) return "";

  JsonDocument doc;
  doc["model"] = CHAT_MODEL;
  JsonArray msgs = doc["messages"].to<JsonArray>();
  // system personality
  { JsonObject m = msgs.add<JsonObject>(); m["role"] = "system"; m["content"] = SYSTEM_PROMPT; }
  // replay memory
  for (int i = 0; i < historyCount; i++) {
    int sep = history[i].indexOf('\x1f');
    JsonObject m = msgs.add<JsonObject>();
    m["role"]    = history[i].substring(0, sep);
    m["content"] = history[i].substring(sep + 1);
  }
  // new user turn
  { JsonObject m = msgs.add<JsonObject>(); m["role"] = "user"; m["content"] = userText; }

  String payload;
  serializeJson(doc, payload);

  https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
  https.addHeader("Content-Type", "application/json");
  int code = https.POST(payload);
  String reply;
  if (code == 200) {
    JsonDocument res;
    if (!deserializeJson(res, https.getString()))
      reply = res["choices"][0]["message"]["content"].as<String>();
  } else {
    Serial.printf("[chat] HTTP %d: %s\n", code, https.getString().c_str());
  }
  https.end();
  reply.trim();

  if (reply.length()) {
    pushHistory("user", userText);
    pushHistory("assistant", reply);
  }
  return reply;
}

// ---------------------------------------------------------------------------
// Returns raw 24kHz 16-bit mono PCM (response_format = "pcm"). Feed straight
// to the ES8311/I2S speaker. Caller frees the buffer.
uint8_t *synthesize(const String &text, size_t *outBytes) {
  *outBytes = 0;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  if (!https.begin(client, "https://api.openai.com/v1/audio/speech")) return nullptr;

  JsonDocument doc;
  doc["model"] = TTS_MODEL;
  doc["voice"] = TTS_VOICE;
  doc["input"] = text;
  doc["response_format"] = "pcm";   // 24kHz, 16-bit, mono, little-endian
  String payload;
  serializeJson(doc, payload);

  https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
  https.addHeader("Content-Type", "application/json");
  int code = https.POST(payload);
  uint8_t *audio = nullptr;
  if (code == 200) {
    int len = https.getSize();
    WiFiClient *stream = https.getStreamPtr();
    // Stream into a growing PSRAM buffer (len may be -1 if chunked).
    size_t cap = (len > 0) ? len : 64 * 1024;
    audio = (uint8_t *)ps_malloc(cap);
    size_t got = 0;
    while (https.connected() && (len < 0 || got < (size_t)len)) {
      size_t avail = stream->available();
      if (avail) {
        if (got + avail > cap) { cap *= 2; audio = (uint8_t *)ps_realloc(audio, cap); }
        got += stream->readBytes(audio + got, avail);
      } else delay(1);
      if (len < 0 && !https.connected()) break;
    }
    *outBytes = got;
  } else {
    Serial.printf("[tts] HTTP %d: %s\n", code, https.getString().c_str());
  }
  https.end();
  return audio;
}

} // namespace agent
