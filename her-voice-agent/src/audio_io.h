// ============================================================================
//  audio_io.h  -  microphone capture + speaker playback via the ES8311 codec.
//
//  >>> THIS IS A HARDWARE HOOK. The function bodies in audio_io.cpp are STUBS.
//  >>> Fill them using Waveshare's official demo for THIS board, which has the
//  >>> correct I2S/I2C pin numbers and ES8311 init sequence. Do NOT let the AI
//  >>> guess these pins -- copy them from the demo. See README "Step 3 & 4".
// ============================================================================
#pragma once
#include <Arduino.h>

namespace audioio {

// One-time init: I2C to ES8311, I2S bus, mic gain, speaker amp enable.
bool begin();

// Block-record from the mic until the user stops talking (or MAX_RECORD_SECS).
// Writes 16-bit mono PCM @ SAMPLE_RATE into a PSRAM buffer you must free().
// Returns bytes captured, 0 on failure. Sets *out to the buffer.
size_t recordUtterance(uint8_t **out);

// Play 24kHz 16-bit mono PCM through the speaker (blocks until done).
void play(const uint8_t *pcm, size_t bytes);

// True while a touch/press indicates "I want to talk" (push-to-talk).
// Simplest reliable trigger; swap for wake-word later.
bool talkPressed();

} // namespace audioio
