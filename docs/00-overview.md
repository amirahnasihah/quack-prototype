# quack quack

> A rubber duck voice agent. Sits on your desk. Listens. Talks back.
> Inspired by the AI from *Her* — always present, always warm.

---

## What is this?

A physical rubber duck that runs a local + cloud voice agent pipeline on an ESP32-S3.
You talk to it. It talks back. It has a tiny glowing round screen for a face.
No keyboard. No mouse. Just voice.

Think of it as a rubber duck debugging companion — but it actually responds.

---

## Why a rubber duck?

Rubber duck debugging is already a thing. Developers explain their problems out loud to a rubber duck to think through them. quack quack makes the duck talk back — giving you a real conversational thinking partner that sits on your desk.

It is also just fun and weird. A glowing AMOLED duck that has opinions. Perfect.

---

## Core Experience

- Say something, it listens (dual mic array, hardware echo cancellation)
- It thinks (LLM via WiFi)
- It speaks (speaker + audio codec)
- Its face reacts (1.43" AMOLED, animated expressions)
- It is battery-powered and portable

The personality target: warm, curious, slightly witty. Like Samantha from *Her* but less existential crisis, more rubber duck.

---

## Two Boards, Two Prototypes

quack quack has been prototyped on **two different ESP32 boards**. Same duck, two hardware paths:

| | Prototype A — CYD | Prototype B — Waveshare (target) |
|---|---|---|
| Board | ESP32-2432S028R "Cheap Yellow Display" | Waveshare ESP32-S3-Touch-AMOLED-1.43C |
| MCU | ESP32 (dual-core LX6) | ESP32-S3-PICO-1-N8R8 (LX7) |
| Display | ILI9341 320×240, SPI | 1.43" AMOLED 466×466, QSPI (round) |
| Mic | INMP441 (external, I2S) | Onboard dual mic array + ES7210 AEC |
| Speaker | MAX98357 I2S amp (external) | Onboard amp + ES8311 codec |
| Touch | XPT2046 (VSPI) | CST820 (I2C) |
| Status | **Working prototype** (pixel duck, mic, speaker, daemon) | Newer board, richer audio — main target |

- **Prototype A (CYD)** is the cheap, hands-on breadboard build — discrete I2S parts wired up. It is where the pixel-duck UI, mic capture, and the MAX98357 speaker test currently run. See the `quack-prototype` repo.
- **Prototype B (Waveshare)** is the newer round-AMOLED board with onboard mics, audio codec, and amp — what the rest of these docs describe as the eventual home for the duck.

The **Hardware at a Glance** table below is the **Waveshare (Prototype B)** target. Full specs for both boards are in [01-hardware.md](./01-hardware.md).

---

## Hardware at a Glance (Prototype B — Waveshare)

| Component | Detail |
|---|---|
| Board | Waveshare ESP32-S3-Touch-AMOLED-1.43C |
| MCU | ESP32-S3-PICO-1-N8R8, 240MHz dual-core LX7 |
| Display | 1.43" AMOLED, 466x466, touch |
| Microphone | Dual mic array + ES7210 echo cancellation |
| Speaker | MX1.25 connector + amplifier chip |
| Audio codec | ES8311 |
| Connectivity | WiFi 2.4GHz + Bluetooth 5 LE |
| Power | 3.7V LiPo via MX1.25 (battery owned, ready) |

---

## Non-Goals (for now)

- No offline LLM (not enough RAM for a proper model)
- No camera / vision
- No persistent long-term memory (maybe later)
- Not trying to be Alexa or Google Home

---

## Related Files

- [01-hardware.md](./01-hardware.md) — full board specs and pin notes
- [02-architecture.md](./02-architecture.md) — voice pipeline design
- [03-roadmap.md](./03-roadmap.md) — phases and milestones
