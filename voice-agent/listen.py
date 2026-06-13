#!/usr/bin/env python3
"""
listen.py — Phase 1: mic → Whisper → print transcript
Wake word: "hey boo" (same breath or follow-up utterance)
"""

import os
import queue
import re
import sys
import warnings
from typing import Optional

warnings.filterwarnings("ignore", category=RuntimeWarning)

import numpy as np
import sounddevice as sd
from faster_whisper import WhisperModel

# ── config ────────────────────────────────────────────────────────────────────
SAMPLE_RATE         = 16000
BLOCK_SIZE          = 512
SILENCE_THRESHOLD   = 0.03    # lower = more sensitive; tune to your room noise
SILENCE_DURATION      = 2.0     # seconds of silence → end of utterance
MIN_SPEECH_FRAMES   = int(0.3 * SAMPLE_RATE / BLOCK_SIZE)  # ignore <0.3s clips
MIN_UTTERANCE_RMS   = 0.018   # skip quiet clips before Whisper (reduces hallucinations)
WAKE_WORD           = "hey boo"
WHISPER_MODEL       = "small"  # tiny / base / small — small is better for rojak
DEBUG               = os.environ.get("SIHAH_DEBUG", "").lower() in ("1", "true", "yes")

# Common Whisper noise outputs on silence / fan / keyboard
JUNK_PHRASES = (
    "thanks for watching",
    "thank you for watching",
    "subscribe",
    "i'm a cabal",
    "im a cabal",
    "you you you",
    "bye bye",
)
# ─────────────────────────────────────────────────────────────────────────────

q: queue.Queue = queue.Queue()
_wake_re = re.compile(r"hey\s*(?:boo|bu|bo|blue)", re.IGNORECASE)


def _audio_callback(indata, frames, time_info, status):
    if status:
        print(status, file=sys.stderr)
    q.put(indata.copy().flatten())


def rms(data: np.ndarray) -> float:
    return float(np.sqrt(np.mean(data ** 2)))


def capture_utterance() -> Optional[np.ndarray]:
    """Block until one full utterance (speech + trailing silence). Returns float32 PCM."""
    silence_limit = int(SILENCE_DURATION * SAMPLE_RATE / BLOCK_SIZE)
    buf: list[float] = []
    silent = 0
    speech_frames = 0

    while True:
        chunk = q.get()
        vol = rms(chunk)

        if vol > SILENCE_THRESHOLD:
            buf.extend(chunk)
            speech_frames += 1
            silent = 0
        elif buf:
            buf.extend(chunk)
            silent += 1
            if silent >= silence_limit:
                if speech_frames >= MIN_SPEECH_FRAMES:
                    audio = np.array(buf, dtype=np.float32)
                    if rms(audio) >= MIN_UTTERANCE_RMS:
                        return audio
                buf.clear()
                speech_frames = 0
                silent = 0


def normalize_transcript(text: str) -> str:
    lowered = text.lower().strip()
    collapsed = " ".join(lowered.split())
    return _wake_re.sub("hey boo", collapsed)


def is_junk_transcript(text: str) -> bool:
    clean = text.strip(".,!? \t")
    if not clean or len(clean) < 3:
        return True

    letters = sum(1 for ch in clean if ch.isalpha())
    if letters < 2:
        return True

    words = clean.split()
    if len(words) == 1 and len(words[0]) <= 2:
        return True

    if len(words) >= 3 and len(set(words)) == 1:
        return True

    if clean.count("hey") >= 3:
        return True

    return any(phrase in clean for phrase in JUNK_PHRASES)


def wake_word_hit(text: str) -> bool:
    return "hey boo" in normalize_transcript(text)


def extract_command(text: str) -> str:
    normalized = normalize_transcript(text)
    parts = normalized.split("hey boo", 1)
    if len(parts) < 2:
        return ""
    return parts[-1].strip().strip(".,!? ")


def transcribe(model: WhisperModel, audio: np.ndarray) -> str:
    segments, _ = model.transcribe(
        audio,
        language=None,
        vad_filter=True,
        no_speech_threshold=0.6,
        log_prob_threshold=-0.8,
        compression_ratio_threshold=2.2,
        condition_on_previous_text=False,
    )
    return " ".join(s.text for s in segments).strip()


def run(model: WhisperModel):
    print(f"[sihah] ready — say '{WAKE_WORD} <command>' (or wake word, then command)\n")

    awaiting_command = False

    with sd.InputStream(
        samplerate=SAMPLE_RATE,
        channels=1,
        blocksize=BLOCK_SIZE,
        dtype=np.float32,
        callback=_audio_callback,
    ):
        while True:
            audio = capture_utterance()
            if audio is None:
                continue

            text = transcribe(model, audio).lower()
            if is_junk_transcript(text):
                if DEBUG:
                    print(f"[debug] skip junk: {text!r}", file=sys.stderr)
                continue

            if awaiting_command:
                print(f"[YOU] {text}")
                awaiting_command = False
                continue

            if not wake_word_hit(text):
                if DEBUG:
                    print(f"[debug] no wake word: {text!r}", file=sys.stderr)
                continue

            command = extract_command(text)
            if command and len(command) > 2:
                print(f"[YOU] {command}")
            else:
                print("[sihah] yes?")
                awaiting_command = True


def main():
    print(f"[sihah] loading whisper ({WHISPER_MODEL})...")
    model = WhisperModel(WHISPER_MODEL, device="cpu", compute_type="int8")
    print("[sihah] model ready\n")

    try:
        run(model)
    except KeyboardInterrupt:
        print("\n[sihah] bye")


if __name__ == "__main__":
    main()
