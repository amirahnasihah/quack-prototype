// ============================================================================
//  audio_io.cpp  -  STUB IMPLEMENTATION. Replace the marked sections with the
//  real ES8311 + I2S code from Waveshare's demo for ESP32-S3-Touch-AMOLED-1.43C.
//
//  HOW TO FILL THIS IN (the safe vibe-coding move):
//   1. Open Waveshare's demo audio example side-by-side in Cursor.
//   2. Copy its exact pin #defines (I2S BCLK/LRCK/DOUT/DIN, I2C SDA/SCL,
//      ES8311 address, PA/amp enable pin) into the section below.
//   3. Ask Cursor: "using these exact pins, implement recordUtterance() and
//      play() with the ESP-IDF/Arduino I2S driver, matching the demo's codec
//      init." Now the AI is editing WITH the real hardware facts, not guessing.
// ============================================================================
#include "audio_io.h"
#include "config.h"

// ---------------------------------------------------------------------------
//  >>> PASTE THE REAL PIN MAP FROM THE WAVESHARE DEMO HERE <<<
//  (these placeholder values are almost certainly WRONG for your board)
// #define I2S_BCLK   ??
// #define I2S_LRCK   ??
// #define I2S_DOUT   ??   // to speaker
// #define I2S_DIN    ??   // from mic
// #define I2C_SDA    ??
// #define I2C_SCL    ??
// #define PA_ENABLE  ??   // speaker amplifier enable
// ---------------------------------------------------------------------------

namespace audioio {

bool begin() {
  // TODO: init I2C, configure ES8311 (clocks, mic ADC, speaker DAC),
  //       init I2S in TX+RX, enable the PA pin.
  Serial.println("[audio] STUB begin() -- fill from Waveshare demo");
  return true;
}

size_t recordUtterance(uint8_t **out) {
  // TODO: read I2S mic frames into a PSRAM buffer until silence / time cap.
  //       Return bytes captured.
  Serial.println("[audio] STUB recordUtterance()");
  *out = nullptr;
  return 0;
}

void play(const uint8_t *pcm, size_t bytes) {
  // TODO: write PCM frames to I2S TX. Note: TTS PCM is 24kHz; either set the
  //       I2S sample rate to 24000 here or resample. Mind the speaker amp pin.
  (void)pcm; (void)bytes;
  Serial.println("[audio] STUB play()");
}

bool talkPressed() {
  // TODO: return true while the screen is touched (push-to-talk). Use the
  //       touch driver from the demo, or a GPIO button for a first test.
  return false;
}

} // namespace audioio
