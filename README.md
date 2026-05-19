# Quack Prototype

A Wokwi-simulated **Duck Agent** display: an ESP32 with a TFT screen that shows an animated duck face and reflects voice-assistant states (idle, listening, thinking, talking). The firmware polls a companion daemon on your Mac for usage/state updates over WiFi.

## Overview

This repo is a hardware prototype for a physical “duck” UI that mirrors what a desktop AI agent is doing. The duck’s expression and on-screen label change with agent state, and a transcript line can show what it heard or is saying.

| State       | Display              | Duck expression                          |
|-------------|----------------------|------------------------------------------|
| `IDLE`      | Idle...              | Normal eyes                              |
| `LISTENING` | Listening...         | Wide eyes, raised brows                  |
| `THINKING`  | Thinking...          | Squint + sweat drop                      |
| `TALKING`   | Talking!             | Open bill (mouth)                        |

The project was exported from [Wokwi](https://wokwi.com/projects/464420696806840321) and is meant to run in the simulator or on real ESP32 + ILI9341 hardware (e.g. Cheap Yellow Display–style boards).

## Hardware

From `diagram.json`:

- **MCU:** ESP32 DevKit V1
- **Display:** ILI9341 TFT (SPI), landscape (`rotation` 1 in firmware)
- **Touch (planned):** XPT2046 — listed in `libraries.txt`; touch handling is still a placeholder in code

### Pin wiring (simulator / diagram)

| ESP32 GPIO | ILI9341 |
|------------|---------|
| GND        | GND     |
| 3V3        | VCC     |
| 14         | SCK     |
| 13         | MOSI    |
| 12         | MISO    |
| 15         | CS      |
| 2          | DC      |
| 4          | RST     |

## Software stack

- **Platform:** Arduino on ESP32
- **Libraries:** `TFT_eSPI`, `WiFi`, `HTTPClient` (see `libraries.txt` for Wokwi deps: Adafruit ILI9341, TFT_eSPI, XPT2046_Touchscreen)

## Architecture

```
┌─────────────────┐     WiFi HTTP GET      ┌──────────────────────┐
│  ESP32 + TFT    │  ───────────────────►  │  Mac daemon          │
│  (this sketch)  │  /usage every 3s       │  :8787               │
└─────────────────┘                        └──────────────────────┘
        │
        └── Renders duck face + state label + transcript on ILI9341
```

- **`sketch.ino`** — main firmware: display drawing, WiFi, daemon polling, serial demo controls
- **`diagram.json`** — Wokwi circuit (ESP32 + ILI9341)
- **`libraries.txt`** — Wokwi library manifest
- **`wokwi-project.txt`** — link back to the original Wokwi project

## Getting started

### 1. Run in Cursor (local build — skips Wokwi cloud queue)

Use this when wokwi.com shows **“Build Servers Busy”**. You compile on your Mac; the extension only runs the simulator.

**One-time setup**

1. Install extensions in Cursor:
   - [Wokwi Simulator](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode) (`wokwi.wokwi-vscode`)
   - [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
2. `Cmd+Shift+P` → **Wokwi: Request a new License** (free trial; paid plan only needed after trial ends).
3. Open this folder in Cursor.

**Every run**

1. `Cmd+Shift+P` → **PlatformIO: Build** (or terminal: `pio run` in this folder).
2. Wait until `.pio/build/esp32dev/firmware.bin` exists.
3. `Cmd+Shift+P` → **Wokwi: Start Simulator**.
4. Serial monitor in the sim panel, **115200** baud — send `1`–`4` to change duck states.

`wokwi.toml` points at the PlatformIO build output. `platformio.ini` configures TFT pins to match `diagram.json`.

### 2. Simulate in Wokwi (browser)

1. Open [Wokwi](https://wokwi.com) and import this folder (or use the project linked in `wokwi-project.txt`).
2. Ensure `libraries.txt` dependencies install (TFT_eSPI, etc.).
3. Build and run the simulation.

If the cloud build queue is busy, use **§1** instead.

Default WiFi in the sketch is `Wokwi-GUEST` with an empty password, which matches the Wokwi guest network.

### 3. Configure for your environment

Edit the top of `sketch.ino`:

```cpp
const char* SSID     = "your-wifi-ssid";
const char* PASSWORD = "your-wifi-password";
const char* DAEMON_URL = "http://<your-mac-ip>:8787/usage";
```

- **`DAEMON_URL`** — point at your Mac (or host) running the usage daemon on port `8787`.
- **`POLL_INTERVAL`** — defaults to `3000` ms between HTTP polls.

### 4. Flash to hardware (optional)

Use the Arduino IDE or PlatformIO with an ESP32 board support package. Match `TFT_eSPI` pin defines to your physical wiring (the Wokwi diagram pins may differ on a CYD board — check your board’s pinout).

## Serial demo controls

While connected over serial (`115200` baud), send a single character to cycle states without the daemon:

| Key | State       |
|-----|-------------|
| `1` | IDLE        |
| `2` | LISTENING   |
| `3` | THINKING    |
| `4` | TALKING     |

Useful for testing the display logic before the Mac daemon is wired up.

## Daemon integration

`pollDaemon()` performs `GET` on `DAEMON_URL` when WiFi is connected. On HTTP 200, the response body is logged to serial. **JSON parsing and state updates from the daemon are not implemented yet** (`TODO` in `sketch.ino`).

Expected direction: parse usage/state JSON from the daemon and map fields to `currentState`, `transcript`, and `drawDuckFace()`.

## Project status

| Area              | Status                                      |
|-------------------|---------------------------------------------|
| Duck face UI      | Implemented (4 states)                      |
| WiFi connect      | Implemented                                 |
| Daemon HTTP poll  | Partial (GET only; no JSON → state yet)     |
| Touch input       | Placeholder (`isTouched()` returns false)   |
| Real CYD touch    | Planned via XPT2046 library                 |

## File layout

```
quack-prototype/
├── README.md          # This file
├── sketch.ino         # ESP32 firmware
├── platformio.ini     # Local build (PlatformIO)
├── wokwi.toml         # Wokwi extension → firmware paths
├── diagram.json       # Wokwi hardware diagram
├── libraries.txt      # Wokwi library dependencies
└── wokwi-project.txt  # Original Wokwi project reference
```

## License

No license file is included yet. Add one if you plan to share or open-source the project.
