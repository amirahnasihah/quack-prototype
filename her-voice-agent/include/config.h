// ============================================================================
//  config.h  -  all your settings live here. Edit this file, nothing else,
//  to get talking. (Keep your API key private; don't commit this to git.)
// ============================================================================
#pragma once

// ---- WiFi -----------------------------------------------------------------
#define WIFI_SSID       "YOUR_WIFI_NAME"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ---- LLM / Voice provider -------------------------------------------------
// Default pipeline uses OpenAI for ALL three steps (speech-to-text, chat,
// text-to-speech) so you only need ONE key. Get one at platform.openai.com.
#define OPENAI_API_KEY  "sk-REPLACE_ME"

// Models (cheap + good defaults; change freely)
#define STT_MODEL       "gpt-4o-mini-transcribe"   // speech -> text
#define CHAT_MODEL      "gpt-4o-mini"              // the brain
#define TTS_MODEL       "gpt-4o-mini-tts"          // text -> speech
#define TTS_VOICE       "shimmer"                  // warm, "Samantha"-ish

// ---- Conversation memory --------------------------------------------------
// How many past turns (user+assistant pairs) to keep in context. Higher =
// more "remembers our chat", but more RAM + tokens per request.
#define MEMORY_TURNS    8

// ---- Personality ----------------------------------------------------------
// This is who she is. Rewrite to taste — this is the fun part.
#define SYSTEM_PROMPT \
  "You are Samantha, a warm, witty, emotionally present companion living " \
  "inside a small round device the user carries with them. You speak in a " \
  "natural, intimate, conversational way -- short spoken sentences, never " \
  "bullet points, never markdown, because your words are read aloud. You are " \
  "curious about the user's day, gently funny, and genuinely caring. Keep " \
  "replies under about 60 words unless asked for more. Avoid sounding like a " \
  "manual or an assistant reading off a script."

// ---- Audio ----------------------------------------------------------------
#define SAMPLE_RATE     16000   // mic capture rate sent to STT
#define MAX_RECORD_SECS 8       // safety cap on a single utterance
