"""Record the slop monster's line, "Certainly! Here is some slop.", with the robotic
macOS voice Zarvox, and write it as a C array (game/src/audio/voice_slop.c): mono,
signed 16 bit, at the mixer rate, trimmed of silence and normalised. Needs macOS
`say` and ffmpeg.
"""

import array
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "game" / "src" / "audio" / "voice_slop.c"
RATE = 22050
LINE = "Certainly! Here is some slop."


def record():
    with tempfile.TemporaryDirectory() as tmp:
        aiff = Path(tmp) / "slop.aiff"
        subprocess.run(["say", "-v", "Zarvox", "-r", "228", "-o", str(aiff), LINE], check=True)
        raw = subprocess.run(
            ["ffmpeg", "-v", "error", "-i", str(aiff), "-ar", str(RATE), "-ac", "1", "-f", "s16le", "-"],
            check=True, capture_output=True).stdout
    samples = array.array("h")
    samples.frombytes(raw)
    return list(samples)


def main():
    samples = record()
    loud = [i for i, v in enumerate(samples) if abs(v) > 600]
    samples = samples[loud[0]:loud[-1] + 1]
    peak = max(abs(v) for v in samples)
    samples = [int(v * 30000 / peak) for v in samples]
    fade = 300  # a short fade out, so the clip never ends on a click
    for i in range(fade):
        samples[-1 - i] = int(samples[-1 - i] * i / fade)
    lines = ["    " + ",".join(str(v) for v in samples[i:i + 16]) + "," for i in range(0, len(samples), 16)]
    OUT.write_text(
        '/**\n * @file voice_slop.c\n * @brief The slop monster saying "%s" in a robot voice, as a recorded clip\n'
        " * (mono, signed 16 bit at the mixer rate). Made by scripts/make_voice_slop.py.\n */\n"
        '#include "voice_slop.h"\n\nconst int kVoiceSlopLength = %d;\n\nconst int16_t kVoiceSlop[%d] = {\n%s\n};\n'
        % (LINE, len(samples), len(samples), "\n".join(lines)))
    (OUT.parent / "voice_slop.h").write_text(
        '/**\n * @file voice_slop.h\n * @brief The recorded line of the AI slop monster.\n */\n'
        "#ifndef GAME_AUDIO_VOICE_SLOP_H_\n#define GAME_AUDIO_VOICE_SLOP_H_\n\n#include <stdint.h>\n\n"
        "extern const int kVoiceSlopLength; /* in frames, at the mixer rate */\n"
        "extern const int16_t kVoiceSlop[];\n\n#endif  // GAME_AUDIO_VOICE_SLOP_H_\n")
    print("wrote %d samples (%.2f s)" % (len(samples), len(samples) / RATE))


main()
