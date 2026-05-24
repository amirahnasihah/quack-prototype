# Quack Prototype

Pixel **duck agent** UI on **ESP32-2432S028R** (Cheap Yellow Display), with WiFi, Mac daemon polling, INMP441 mic, and touch swipe pages. Developed in **Cursor + Wokwi**; flashable to real CYD hardware.

**Author:** [amirahnasihah](https://github.com/amirahnasihah)

---

## What it does

| Feature | Description |
|---------|-------------|
| Pixel duck | 20×20 sprites — idle, listening, thinking, talking |
| Daemon poll | `GET /usage` every 3s → state + transcript |
| INMP441 mic | I2S 16 kHz; level shown in LISTENING mode |
| Touch UI | Swipe left/right — agent page ↔ model details |
| Wokwi sim | CYD board + INMP441 custom chip |

---

## Quick start (Wokwi in Cursor)

1. Install **PlatformIO** + **Wokwi Simulator** extensions.
2. `Cmd+Shift+P` → **Wokwi: Request a new License** (once).
3. Build:
   ```bash
   pio run
   ```
4. **Wokwi: Start Simulator** (restart after chip/firmware changes).
5. Open **Wokwi Terminal** (bottom panel) → type **`2`** → Enter.

### Serial keys

| Key | Action |
|-----|--------|
| `1` | Idle |
| `2` | **Listening** (mic test) |
| `3` | Thinking |
| `4` | Talking |
| `[` / `p` | Previous page |
| `]` / `n` | Next page |

Type in **Wokwi Terminal**, not in the board picture or Cursor chat.

---

## INMP441 wiring (real CYD)

**P3 has no 3.3V** — power the mic from **CN1 pin 1**.

### CN1 (4-pin JST 1.25mm)

| Pin | GPIO / power | → INMP441 |
|-----|--------------|-----------|
| 1 | **3.3V** | VDD (red) |
| 2 | **IO27** | WS (blue) |
| 3 | **IO22** | SCK (yellow) |
| 4 | **GND** | GND + L/R (black) |

### P3 (4-pin) — extra jumper

| Pin | GPIO | → INMP441 |
|-----|------|-----------|
| 3 | **IO35** | SD (green) |

```
INMP441          CN1                    P3
VDD  ──red──────► 1  3.3V
WS   ──blue─────► 2  IO27
SCK  ──yellow───► 3  IO22
GND  ──black────► 4  GND
L/R  ──black────► GND
SD   ──green──────────────────────────► 3  IO35
```

**Do not** use **IO21** (P3 pin 1) for mic WS — it is **TFT backlight**.

**Parts to buy:** JST 1.25mm 4-pin → dupont (for CN1) + one dupont jumper for SD.

---

## Mic test — what to expect

### Real hardware

1. Wire as above, flash firmware.
2. Serial **`2`**.
3. Speak near mic → `Mic level: …` should **rise** (no fallback message).

### Wokwi simulator

I2S between ESP32 sim and custom INMP441 chip is ** imperfect**. When `i2s_read` gets no audio:

- Serial (once): `Mic: i2s_read empty — sim fallback active`
- Levels **~600–1000** oscillating — **UI test only**, not real audio.

| Output | Meaning |
|--------|---------|
| `Mic level: 600+` (wave) | Sim OK — listening UI works |
| `Mic: sim fallback active` | Expected in Wokwi; I2S sim silent |
| Real CYD + mic | No fallback; levels from actual sound |

After firmware/chip changes: `pio run` → **Restart Simulation** → `2`.

---

## Configuration

Edit top of `sketch.ino`:

```cpp
const char* SSID     = "Wokwi-GUEST";   // or your WiFi
const char* PASSWORD = "";
const char* DAEMON_URL = "http://192.168.1.200:8787/usage";
```

---

## Architecture

```
┌──────────────────┐   WiFi GET /usage    ┌─────────────────┐
│  CYD + INMP441   │ ◄──────────────────► │  Mac daemon     │
│  pixel duck UI   │   every 3s           │  :8787          │
└──────────────────┘                      └─────────────────┘
        │
        ├── Page 0: duck + state + transcript
        └── Page 1: model info (by amirahnasihah)
```

---

## Project layout

```
quack-prototype/
├── README.md           ← you are here
├── CHANGELOG.md        ← history of changes
├── sketch.ino          ← firmware
├── pixel_creature.h    ← duck sprites
├── platformio.ini      ← CYD TFT + touch pins
├── wokwi.toml          ← sim firmware + INMP441 chip
├── diagram.json        ← Wokwi circuit
├── chips/
│   ├── inmp441.chip.c
│   ├── inmp441.chip.json
│   └── inmp441.chip.wasm
└── wokwi-project.txt
```

### Rebuild INMP441 Wokwi chip

```bash
./.tools/wokwi-cli chip compile chips/inmp441.chip.c -o chips/inmp441.chip.wasm
```

(`wokwi-cli` binary is gitignored under `.tools/` — download from [wokwi-cli releases](https://github.com/wokwi/wokwi-cli/releases).)

---

## Status

| Area | Status |
|------|--------|
| Pixel duck + states | Done |
| Daemon JSON | Done |
| Touch swipe pages | Done |
| INMP441 hardware | Wired per CN1/P3 guide |
| Wokwi mic I2S | Sim fallback; real I2S sim WIP |
| STT / daemon v2 | Planned |

See **[CHANGELOG.md](./CHANGELOG.md)** for dated details.

---

## License

No license file yet. Add one if you open-source the project.
