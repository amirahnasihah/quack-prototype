// ============================================================================
//  main.cpp  -  the "Her" voice agent loop.
//
//  Flow:  press to talk  ->  record  ->  transcribe  ->  LLM  ->  speak
//
//  The cloud logic (voice_agent.*) is real and ready. The audio + display
//  modules are STUBS you fill from the Waveshare demo (see README). With the
//  stubs in place this still COMPILES and runs the full cloud round-trip if
//  you feed it text -- great for testing your API key before the hardware is
//  wired up (see runTextSmokeTest()).
// ============================================================================
#include <Arduino.h>
#include "config.h"
#include "voice_agent.h"
#include "audio_io.h"
#include "display_ui.h"

// Flip to true to test the cloud pipeline over Serial WITHOUT mic/speaker:
// type a line in the monitor, get a reply printed back. Proves keys + WiFi.
#define TEXT_SMOKE_TEST false

static void converseOnce() {
  ui::setState(ui::LISTENING);
  uint8_t *pcm = nullptr;
  size_t pcmBytes = audioio::recordUtterance(&pcm);
  if (!pcmBytes || !pcm) { ui::setState(ui::IDLE); return; }

  ui::setState(ui::THINKING);
  String heard = agent::transcribe(pcm, pcmBytes);
  free(pcm);
  if (heard.isEmpty()) { ui::setState(ui::IDLE); return; }
  Serial.printf("[you] %s\n", heard.c_str());
  ui::showText(heard);

  String reply = agent::chat(heard);
  if (reply.isEmpty()) { ui::setState(ui::ERROR); delay(800); ui::setState(ui::IDLE); return; }
  Serial.printf("[her] %s\n", reply.c_str());
  ui::showText(reply);

  ui::setState(ui::SPEAKING);
  size_t ttsBytes = 0;
  uint8_t *tts = agent::synthesize(reply, &ttsBytes);
  if (tts && ttsBytes) { audioio::play(tts, ttsBytes); free(tts); }

  ui::setState(ui::IDLE);
}

static void runTextSmokeTest() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.isEmpty()) return;
  Serial.printf("[you] %s\n", line.c_str());
  String reply = agent::chat(line);
  Serial.printf("[her] %s\n", reply.c_str());
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Her voice agent booting ===");

  ui::begin();
  audioio::begin();

  if (!agent::connectWiFi()) {
    ui::setState(ui::ERROR);
    return;
  }
  ui::setState(ui::IDLE);
  Serial.println(TEXT_SMOKE_TEST
    ? "Type a message in the monitor and press enter."
    : "Ready. Press the screen to talk.");
}

void loop() {
  ui::tick();

#if TEXT_SMOKE_TEST
  runTextSmokeTest();
#else
  if (audioio::talkPressed()) converseOnce();
#endif

  delay(10);
}
