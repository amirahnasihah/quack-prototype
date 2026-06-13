# Architecture: Voice Agent Pipeline

---

## High-Level Flow

```
User speaks
    ↓
Dual mic array + ES7210 (hardware echo cancel + noise reduce)
    ↓
ES8311 codec → I2S audio stream to ESP32-S3
    ↓
Wake word detection (on-device)
    ↓  [triggered]
Audio capture → buffer in PSRAM
    ↓
STT (Speech-to-Text) — cloud API via WiFi
    ↓
LLM — cloud API, streaming response
    ↓
TTS (Text-to-Speech) — cloud API, audio stream back
    ↓
ES8311 codec → speaker amp → MX1.25 speaker
    ↓
AMOLED face animation (reacts during listen / think / speak states)
```

---

## States

| State | Display | LED | Behavior |
|---|---|---|---|
| `idle` | Duck face, eyes blinking slowly | Off | Listening for wake word |
| `listening` | Eyes wide open, waveform | Dim | Capturing audio |
| `thinking` | Eyes swirling / loading | Pulse | Waiting for LLM response |
| `speaking` | Mouth animating | Off | Playing TTS audio |
| `error` | Sad duck face | Red blink | WiFi down, API error, etc. |

---

## Component Choices

### Wake Word
- **ESP-SR** (Espressif's own) — runs on ESP32-S3, supports custom wake words
- Built-in to IDF, no cloud needed
- Custom wake word: `"hey quack"` or just `"quack"`
- Fallback: tap-to-talk via touch screen (easier to implement first)

### STT (Speech-to-Text)
| Option | Notes |
|---|---|
| OpenAI Whisper API | Best quality, easy REST API, ~$0.006/min |
| Groq Whisper | Faster (real-time), free tier generous |
| Local Whisper (tiny) | Too slow on ESP32 alone, possible if offloaded to server |

Recommended start: **Groq Whisper** (fast + free tier)

### LLM
| Option | Notes |
|---|---|
| OpenAI GPT-4o mini | Good balance of quality vs cost |
| Groq Llama 3.1 8B | Very fast, free tier |
| Claude Haiku | Smart, affordable |

Recommended start: **Groq Llama 3.1 8B** (low latency matters for voice feel)

Prompt should establish duck personality: warm, curious, helpful, slightly quacky.

### TTS (Text-to-Speech)
| Option | Notes |
|---|---|
| OpenAI TTS | Very natural, MP3 streaming, `alloy` voice |
| ElevenLabs | Most expressive, higher cost |
| Piper TTS (local) | Offline but needs a companion server |
| ESP-TTS | On-device but robotic quality |

Recommended start: **OpenAI TTS** streaming — pipe audio chunks directly to speaker as they arrive (reduces perceived latency).

---

## Audio Streaming Strategy

For lowest latency, stream TTS audio in chunks:
1. LLM streams text tokens as they arrive
2. Buffer text into sentences (wait for `.`, `?`, `!` or pause)
3. Send sentence to TTS API
4. Stream audio response chunks to ESP32
5. Play chunk while next sentence is being processed

This means the duck starts speaking within ~1-2 seconds of the LLM starting to respond, instead of waiting for the full reply.

---

## WiFi Handling

- Connect on boot, store credentials in NVS (Non-Volatile Storage)
- Auto-reconnect on drop
- Show sad face if WiFi lost during conversation
- Consider: provisioning mode via BLE for first-time WiFi setup (Espressif has a library for this)

---

## Memory Budget (8MB PSRAM)

| Use | Estimated |
|---|---|
| Audio capture buffer (10s @ 16kHz 16-bit) | ~320 KB |
| TTS audio playback buffer | ~256 KB |
| Display frame buffer (466x466 RGB565) | ~434 KB |
| LLM response text buffer | ~32 KB |
| Wake word model (ESP-SR) | ~200 KB |
| OS + stack + misc | ~1 MB |
| **Total estimate** | **~2.3 MB** |

Headroom is comfortable. No compression tricks needed for v1.

---

## Companion Server (Optional but Useful)

A lightweight server (Pi Zero, VPS, or local machine) could:
- Handle WiFi provisioning UI
- Proxy API calls (hide keys from device firmware)
- Store conversation history
- Log quacks for debugging

Not required for v1. Ship the duck first.

---

## Security Notes

- API keys must NOT be hardcoded in firmware (if open-sourcing)
- Use NVS encrypted partition to store keys on device
- Or proxy all API calls through a small personal server
