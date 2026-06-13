# Hardware

quack quack runs on **two boards** (see [00-overview.md](./00-overview.md)):

- **Prototype A — CYD** (ESP32-2432S028R): the working breadboard build with discrete I2S parts. Specs at the [bottom of this doc](#prototype-a--cyd-esp32-2432s028r).
- **Prototype B — Waveshare ESP32-S3-Touch-AMOLED-1.43C**: the newer round-AMOLED target board, documented immediately below.

---

# Prototype B: Waveshare ESP32-S3-Touch-AMOLED-1.43C

Source: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43C
Product: https://www.waveshare.com/esp32-s3-touch-amoled-1.43c.htm

---

## MCU

| Spec | Value |
|---|---|
| Chip | ESP32-S3-PICO-1-N8R8 |
| Architecture | Xtensa 32-bit LX7 dual-core |
| Clock | up to 240 MHz |
| SRAM | 512 KB (built-in) + 8 MB PSRAM (stacked) |
| ROM | 384 KB |
| Flash | 8 MB (stacked) |

---

## Display

| Spec | Value |
|---|---|
| Panel type | AMOLED |
| Size | 1.43 inch |
| Resolution | 466 x 466 px |
| Colors | 16.7M (24-bit) |
| Brightness | 600 cd/m² |
| Contrast | 10000:1 |
| Interface | QSPI |
| Driver IC | CO5300 |
| Touch controller | CST820 (I2C) |

Notes:
- Touch works, could be used for tap-to-talk or tap-to-dismiss
- High contrast AMOLED is great for animated face expressions with black background

---

## Audio

| Component | Role |
|---|---|
| ES8311 | Low-power audio codec (I2S) |
| ES7210 | Echo cancellation chip for dual-mic input |
| Speaker amp chip | Drives external speaker via MX1.25 |
| Dual mic array | Onboard, supports noise reduction + echo cancel |

The echo cancellation is hardware-assisted — ES7210 handles AEC so the MCU does not need to run software echo cancellation. This is a big deal for voice agent quality.

Speaker is external via MX1.25 2-pin connector. Battery already purchased (also MX1.25).

---

## Connectivity

| Feature | Detail |
|---|---|
| WiFi | 2.4 GHz 802.11 b/g/n |
| Bluetooth | BT 5 (LE) |
| Antenna | Onboard chip antenna |

WiFi is how we reach the LLM API. BT5 LE could be used later for pairing or config.

---

## Power

| Feature | Detail |
|---|---|
| Battery connector | MX1.25 2-pin, 3.7V LiPo |
| Charge/discharge | Onboard management (PMIC) |
| USB | Type-C (also used for flashing + serial) |

Battery is owned. Ready to connect.

---

## I/O and Buttons

| Item | GPIO / Note |
|---|---|
| BOOT button | Hold + PWR to enter download mode |
| PWR button | Short press ON, long press OFF |
| Programmable LED | GPIO5 |
| I2C pad | 1-ch exposed |
| UART pad | 1-ch exposed |
| USB pad | 1-ch (Type-C) |

---

## Development Environment Options

| Method | Notes |
|---|---|
| Arduino IDE | Easier to start, good library ecosystem |
| ESP-IDF | More control, better for production, VS Code plugin |

Recommendation: start with Arduino for prototyping voice pipeline, migrate to ESP-IDF when optimizing memory and latency.

---

## Known Constraints

- 8 MB PSRAM is the ceiling for model buffers and audio buffers
- No offline LLM at this RAM size (Llama 3.2 1B needs ~1.5GB minimum)
- Audio streaming to cloud API is the practical path
- I2S audio via ES8311 needs correct clock config — check Waveshare example code first
- QSPI display driver (CO5300) is less common than SPI — use Waveshare's provided library

---

# Prototype A: CYD (ESP32-2432S028R)

The "Cheap Yellow Display" — the first, hands-on prototype. Plain ESP32 with a 2.8" resistive-touch TFT, plus **discrete I2S audio parts** wired to its expansion connectors. This is where the pixel duck, mic capture, daemon polling, and the MAX98357 speaker test currently live (`quack-prototype` repo, developed in Cursor + Wokwi).

## Board

| Spec | Value |
|---|---|
| Chip | ESP32-WROOM (dual-core LX6, 240 MHz) |
| Display | ILI9341 2.8" TFT, 320×240, SPI |
| Touch | XPT2046 resistive, on a **separate SPI bus (VSPI)** |
| Connectivity | WiFi 2.4 GHz + BT Classic/BLE |
| Expansion | CN1 (4-pin JST 1.25mm) + P3 header |

## Audio (discrete I2S parts)

Unlike the Waveshare board, the CYD has **no onboard codec/amp/mic** — they are added externally and share one I2S port (`I2S_NUM_0`) in full-duplex.

| Part | Role | Pins |
|---|---|---|
| INMP441 | I2S MEMS microphone (input) | SCK=IO22, WS=IO27, SD=IO35, L/R→GND |
| MAX98357 | I2S Class-D amplifier (output) | BCLK=IO22, LRC=IO27, **DIN=IO26** |

The mic and amp **share BCLK (IO22) and WS/LRC (IO27)**; only the data lines differ (mic SD=IO35, amp DIN=IO26). Sample rate 16 kHz, 16-bit.

### MAX98357 speaker test (recent work)

A 440 Hz test tone (~0.6 s) verifies the speaker path: in the serial monitor, send **`5`** → `Spk: playing 440 Hz test (~0.6s)...`. If silent, tie the module's **SD pin to 3.3V** (unmutes the left channel) and re-check DIN wiring.

## Wiring notes / constraints

- **P3 has no 3.3V pin** — power INMP441 (and MAX98357 VIN) from **CN1 pin 1**.
- **IO21 is the TFT backlight** — do **not** use it for I2S WS (breaks word-select; this bit the project in the Wokwi sim).
- Touch (XPT2046) is on **VSPI**, separate from the display's SPI bus — `TFT_eSPI.getTouch()` won't work; the firmware uses a custom XPT2046 driver (CLK=25, MOSI=32, MISO=39, CS=33).
- In the Wokwi simulator, I2S mic capture is imperfect → firmware falls back to a synthetic mic level (~600–1000) for UI testing; real levels only on hardware.

## Why two boards?

The CYD is cheap, easy to wire, and great for proving the software (duck states, daemon JSON, voice pipeline) without surface-mount audio. The Waveshare ESP32-S3 board folds the mic array, codec (ES8311), amp, and AEC (ES7210) **onto the board** with a round AMOLED — better final form for a desk duck, fewer jumper wires, hardware echo cancellation. Prototype A proves the logic; Prototype B is the home it moves into.
