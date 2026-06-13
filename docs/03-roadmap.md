# Roadmap

---

## Phase 0: Hello Duck (Blink the LED, make it quack)

Goal: get the board running and understand the hardware.

- [ ] Set up Arduino IDE or VS Code + ESP-IDF
- [ ] Flash Waveshare example code, verify screen works
- [ ] Verify audio output — play a WAV file through the speaker
- [ ] Verify mic input — record audio and play it back
- [ ] Blink GPIO5 LED
- [ ] Connect to WiFi, print IP address

Deliverable: board is alive, audio in/out confirmed, WiFi confirmed.

---

## Phase 1: Tap to Talk

Goal: voice interaction without wake word (simplest possible path).

- [ ] Tap touch screen to start recording
- [ ] Capture mic audio into PSRAM buffer (5-10 seconds max)
- [ ] Tap again to stop and send
- [ ] Send audio buffer to Groq Whisper STT
- [ ] Print transcription to serial monitor
- [ ] Send transcription to Groq LLaMA for a response
- [ ] Print LLM response to serial monitor
- [ ] Send LLM response to OpenAI TTS
- [ ] Stream TTS audio chunks to speaker
- [ ] Basic AMOLED states: idle / listening / thinking / speaking

Deliverable: a fully working voice conversation loop. Janky but functional.

---

## Phase 2: Wake Word

Goal: hands-free activation.

- [ ] Integrate ESP-SR wake word engine
- [ ] Register `"hey quack"` as wake word (or use built-in trigger word)
- [ ] Replace tap-to-talk with wake word trigger
- [ ] Auto stop listening after 3-5 seconds of silence (VAD)
- [ ] LED pulse during wake word listening mode

Deliverable: fully hands-free like *Her* — just talk to it.

---

## Phase 3: Personality and Face

Goal: it feels alive.

- [ ] Design duck face states in LVGL or custom drawing:
  - Idle: slow blinking eyes
  - Listening: wide eyes + audio waveform
  - Thinking: swirling / loading animation
  - Speaking: mouth moves with audio amplitude
  - Error/sad: droopy eyes
- [ ] Give it a system prompt with personality
  - Warm, curious, slightly quacky
  - Good at rubber duck debugging (listens to code problems)
  - Does not pretend to be human but does not break immersion
- [ ] Tune TTS voice to match personality (OpenAI `nova` or `alloy`)

Deliverable: a duck with a soul.

---

## Phase 4: Polish and Housing

Goal: it lives in a rubber duck.

- [ ] Design or find a rubber duck enclosure
- [ ] Mount the board + speaker + battery inside
- [ ] Route mic holes for dual array
- [ ] Cut hole or use transparent dome for AMOLED screen (duck's belly/face)
- [ ] Seal and make it desk-worthy
- [ ] Battery life testing — estimate hours per charge
- [ ] Power optimization (display brightness, WiFi sleep between conversations)

Deliverable: a physical artifact. A real rubber duck that talks back.

---

## Phase 5: Nice to Have (someday)

- [ ] Persistent memory: remember things you told it (vector DB via companion server)
- [ ] BLE provisioning for WiFi setup without reflashing
- [ ] OTA firmware updates
- [ ] Multiple personalities / modes (debug mode, chill mode, roast mode)
- [ ] Conversation history display on touch screen
- [ ] Whisper local via companion Pi on same network

---

## Current Status

Work is split across two boards (see [00-overview.md](./00-overview.md)). **Prototype A (CYD)** is the active breadboard build proving the software; **Prototype B (Waveshare)** is the target the duck moves into.

| Phase | Prototype A — CYD | Prototype B — Waveshare |
|---|---|---|
| Phase 0 (alive: screen/audio/WiFi) | In progress — display, WiFi, INMP441 mic capture, **MAX98357 speaker test (440 Hz beep)** all working | Not started |
| Phase 1 (tap to talk) | Partial — pixel-duck states + Mac daemon JSON poll done; STT/LLM/TTS loop is next | Not started |
| Phase 2 (wake word) | Not started | Not started |
| Phase 3 (personality + face) | Pixel duck face states (idle/listen/think/talk) done | Not started |
| Phase 4 (housing) | Not started | Not started |

CYD path notes: built in Cursor + Wokwi (PlatformIO), flashable to real hardware. Voice agent prototyped separately on Mac (`voice-agent/listen.py` — faster-whisper STT, wake word "hey boo"). Hardware ready for both boards; battery (MX1.25) owned for the Waveshare build.

---

## Stack Summary

| Layer | Choice |
|---|---|
| Firmware | ESP-IDF or Arduino (ESP32-S3) |
| Wake word | ESP-SR |
| STT | Groq Whisper |
| LLM | Groq Llama 3.1 8B |
| TTS | OpenAI TTS (streaming) |
| Display UI | LVGL or custom QSPI drawing |
| Audio | ES8311 (I2S) + ES7210 (AEC) |
