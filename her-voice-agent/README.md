# Her — a voice companion on the ESP32-S3 AMOLED (1.43C)

A little round device you talk to and it talks back, with a personality and
memory of your conversation. Speech-to-text → LLM → text-to-speech, all over
WiFi. Built to be **vibe-coded in Cursor**.

## The mental model (read this first)

The ESP32 is **not** running the AI. It's a tiny relay: it records your voice,
ships it to the cloud, and plays back what comes home. Three cloud calls per
turn (transcribe → chat → speak). That means the AI/personality part is just
normal networking code — easy and safe to vibe on. The genuinely fiddly part
is the **board-specific hardware** (the AMOLED screen and the ES8311 audio
chip), whose exact pins live in Waveshare's demo. So:

> **Golden rule:** get Waveshare's demo running first, paste its driver code
> into the two stub files here, and only *then* let Cursor go wild on features.
> Don't let the AI invent pin numbers — that's the #1 way this fails.

## What's already done vs. what you fill in

| File | Status |
|------|--------|
| `voice_agent.cpp/.h` | ✅ Real, working OpenAI calls (STT, chat w/ memory, TTS) |
| `main.cpp` | ✅ Full conversation state machine |
| `config.h` | ✅ Your settings + Samantha's personality (edit this) |
| `platformio.ini` | ✅ Board build config |
| `audio_io.cpp` | ⛏️ **STUB** — paste ES8311/I2S code from Waveshare demo |
| `display_ui.cpp` | ⛏️ **STUB** — paste AMOLED/LVGL code from Waveshare demo |

## Setup

**Step 1 — Tools.** Install [Cursor](https://cursor.com), then inside it install
the **PlatformIO IDE** extension. Open this folder. A toolbar appears at the
bottom with ✓ (Build), → (Upload), and 🔌 (Serial Monitor).

**Step 2 — Keys.** Open `include/config.h`. Set your WiFi name/password and your
`OPENAI_API_KEY` (from platform.openai.com). Tweak `SYSTEM_PROMPT` — that's
literally who she is.

**Step 3 — Prove the cloud works (no hardware needed yet).** In `main.cpp` set
`#define TEXT_SMOKE_TEST true`, Upload, open the Serial Monitor, type a message,
press enter. If she replies, your key + WiFi + LLM are all good. Set it back to
`false` afterward.

**Step 4 — Get the hardware drivers.** Download Waveshare's demo for this exact
board from the wiki:
<https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.43> (and the 1.43C docs
at <https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43C>). Flash their demo
once to confirm your screen + mic + speaker work.

**Step 5 — Fill the stubs.** Open the demo's audio + display examples next to
`audio_io.cpp` and `display_ui.cpp`. Copy the **exact pin `#define`s** and codec
init. Then ask Cursor, with both files in context:
> "Using these exact pins from the Waveshare demo, implement `recordUtterance()`,
> `play()`, and `talkPressed()` in audio_io.cpp."

Add any driver libraries the demo uses to `lib_deps` in `platformio.ini`.

**Step 6 — Talk to her.** Upload. Press the screen, speak, let go.

## Vibe-coding ideas (once it works)

- A glowing orb on the AMOLED that pulses when listening, swirls when thinking.
- Wake word ("Hey Sam") instead of push-to-talk, using ESP-SR.
- Persist memory to the TF card so she remembers across reboots.
- Use the onboard IMU so she reacts when you pick her up.

## Swapping the voice (more "Samantha")

OpenAI's `shimmer` voice is the warm default. For a richer, more intimate
voice, swap `synthesize()` in `voice_agent.cpp` to call **ElevenLabs**
(`api.elevenlabs.io/v1/text-to-speech/{voice_id}`, `xi-api-key` header,
request `pcm_24000` output). Everything else stays the same.

## Reality check

- **Latency:** ~2–5s round trip per turn. Normal for cloud voice on a microcontroller.
- **Cost:** fractions of a cent per turn on the mini models. Set a spend limit on your API account.
- **RAM:** audio buffers live in PSRAM (`ps_malloc`). Keep utterances short (`MAX_RECORD_SECS`).
- **Security:** the code uses `setInsecure()` for TLS to keep things simple. Fine for tinkering; pin the root CA before anything serious.
