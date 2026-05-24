# Changelog

All notable changes to **quack-prototype** are documented here.

Format loosely follows [Keep a Changelog](https://keepachangelog.com/). This is a personal prototype — versions are dated milestones, not semver releases.

---

## [Unreleased]

### Planned

- Mac daemon v2 (`duckState`, `transcript`, `POST /audio`)
- Whisper STT + TTS pipeline
- Real mic levels on hardware (no sim fallback)

---

## 2026-05-24 — Mic sim fallback & CYD wiring

### Fixed

- **Wokwi mic level always `0`:** Early `return 0` when `i2s_read` failed or returned empty bytes prevented the sim fallback from running. Fallback now applies on all silent-I2S paths when connected to `Wokwi-GUEST`.
- **WS on IO21:** TFT backlight on GPIO 21 breaks I2S word-select in Wokwi CYD sim. Firmware and `diagram.json` now use **IO27** for WS.
- **Missing INMP441 in sim:** Added custom Wokwi chip (`chips/inmp441.chip.c` → `inmp441.chip.wasm`) and `[[chip]]` entry in `wokwi.toml`.

### Added

- **Sim fallback tone:** When I2S is silent in Wokwi, fake mic level ~600–1000 (sine wave) so listening UI and serial output remain testable. Serial logs `Mic: i2s_read empty — sim fallback active` (or similar) once.
- **CN1 + P3 wiring guide** in `sketch.ino` comments and README (INMP441 power from CN1 pin 1 — P3 has no 3.3V).

### Changed

- I2S mic config: **16-bit**, `I2S_COMM_FORMAT_STAND_I2S`, pins SCK=22, WS=27, SD=35.

---

## 2026-05-24 — Touch swipe & model page

### Added

- **Two UI pages:** Agent (pixel duck) and **Model details** (board, mic, `by amirahnasihah`).
- **Swipe left/right** on CYD touch (XPT2046, `TOUCH_CS=33`).
- Serial page nav: `[` / `p` prev, `]` / `n` next (for Wokwi when touch unavailable).

---

## 2026-05-24 — INMP441 I2S mic

### Added

- INMP441 I2S read (`initMic`, `readMicLevel`, `pollMic` in LISTENING state).
- Wokwi diagram: `chip-inmp441` + `wokwi-microphone`.
- Serial `2` → LISTENING + periodic `Mic level: …` on transcript line.

---

## 2026-05-24 — Pixel duck & daemon JSON

### Added

- Custom **20×20 pixel duck** sprites (`pixel_creature.h`) — idle, listen, think, talk.
- **Daemon JSON parsing:** `ok`, `st`, `duckState`, `transcript`; maps to duck state and transcript line.
- **PlatformIO** build (`platformio.ini`) for ESP32-2432S028R CYD pinout.

### Fixed

- Display flicker on daemon poll (full redraw only on state change).
- Wrong daemon URL; WiFi `Wokwi-GUEST` for sim.
- Display rotation (`setRotation(3)`).

---

## Earlier — Wokwi export

### Added

- Initial Wokwi project: ESP32 + ILI9341, duck state machine, HTTP poll stub.
