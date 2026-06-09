// ============================================================================
//  voice_agent.h  -  the cloud pipeline: speech-to-text, chat, text-to-speech.
//  This is the part that is safe to "vibe code" on -- it's plain networking
//  and JSON, nothing board-specific. Provider calls are isolated here so you
//  can swap OpenAI for ElevenLabs/local later by editing only this file.
// ============================================================================
#pragma once
#include <Arduino.h>

namespace agent {

// Connect to WiFi (call once in setup). Returns true on success.
bool connectWiFi();

// Send recorded audio (raw 16-bit PCM mono @ SAMPLE_RATE) to speech-to-text.
// Returns the recognized text, or "" on failure.
String transcribe(const uint8_t *pcm, size_t pcmBytes);

// Send user text to the LLM (with personality + rolling memory) and return
// the assistant's spoken reply. Updates internal conversation history.
String chat(const String &userText);

// Convert reply text to speech. Returns a heap buffer of 24kHz 16-bit mono
// PCM that you feed to the speaker; caller must free() it. Sets outBytes.
// Returns nullptr on failure.
uint8_t *synthesize(const String &text, size_t *outBytes);

// Wipe conversation memory (e.g. "start a new conversation").
void resetMemory();

} // namespace agent
