#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Packs the Witchbox recordings (ref/samples/*.wav, CC0, see ref/samples/ATTRIBUTION.md) into a C
header for the sound engine: 8-bit samples with one shift per 256-sample block (about 13 bits of
range), so the flash holds them. The three long textures are kept at 11,025 Hz, the rest at 22,050.

    tools/gen_samples.py OUT.h
"""
import struct
import sys
import wave
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
# name, file, rate divider, seconds kept (0 = all); the order is the engine's S_* enum
SAMPLES = [
    ("CRACKLE", "crackle.wav", 2, 0), ("SKIP", "skip.wav", 1, 0), ("HUM", "hum.wav", 2, 0),
    ("WOOSH", "woosh.wav", 2, 0), ("IMPACT", "impact.wav", 1, 0.9), ("DOOR", "door.wav", 1, 1.0),
    ("VOX_HIT", "vox_hit.wav", 1, 0), ("COINS", "coins.wav", 1, 0.8), ("TING", "ting.wav", 1, 0),
    ("WOOD", "wood.wav", 1, 0), ("PAPER", "paper.wav", 1, 0), ("PING", "ping.wav", 1, 0),
    ("GLASS", "glass.wav", 1, 1.2), ("TICK", "tick.wav", 1, 0), ("POP", "pop.wav", 1, 0),
    ("SNAP", "snap.wav", 1, 0), ("CROW", "crow.wav", 1, 0),
]
BLOCK = 256


def load(path, div, secs):
    w = wave.open(str(path))
    assert w.getnchannels() == 1 and w.getsampwidth() == 2 and w.getframerate() == 22050, path
    n = w.getnframes()
    pcm = list(struct.unpack("<%dh" % n, w.readframes(n)))
    if secs:
        pcm = pcm[: int(secs * 22050)]
    if div == 2:        # a gentle average, then every other sample
        pcm = [(pcm[i] + pcm[i + 1]) // 2 for i in range(0, len(pcm) - 1, 2)]
    # a short fade at the end so a cut sample does not click
    fade = min(len(pcm), 441)
    for i in range(fade):
        pcm[len(pcm) - fade + i] = pcm[len(pcm) - fade + i] * (fade - i) // fade
    return pcm


def pack(pcm):
    data = bytearray()
    shifts = bytearray()
    for b in range(0, len(pcm), BLOCK):
        blk = pcm[b:b + BLOCK]
        peak = max(1, max(abs(v) for v in blk))
        sh = 0
        while (peak >> sh) > 127:
            sh += 1
        shifts.append(sh)
        for v in blk:
            q = v >> sh if v >= 0 else -((-v) >> sh)
            data.append(max(-128, min(127, q)) & 0xFF)
    return data, shifts


def main():
    out = [
        "/* Made by tools/gen_samples.py from the recordings in ref/samples (CC0 recordings; ATTRIBUTION.md). */",
        "#pragma once", "#include <stdint.h>",
        "typedef struct { const int8_t *data; const uint8_t *shift; uint32_t len; uint16_t rate; } wb_sample_t;",
    ]
    names, total = [], 0
    for name, fn, div, secs in SAMPLES:
        pcm = load(SRC / "ref" / "samples" / fn, div, secs)
        data, shifts = pack(pcm)
        total += len(data) + len(shifts)
        out.append("static const int8_t smp_%s[%d] = {%s};" % (name.lower(), len(data),
                   ",".join(str(b - 256 if b > 127 else b) for b in data)))
        out.append("static const uint8_t smp_%s_sh[%d] = {%s};" % (name.lower(), len(shifts),
                   ",".join(str(s) for s in shifts)))
        names.append((name, len(pcm), 22050 // div))
    out.append("enum { " + ", ".join("S_" + n for n, _, _ in names) + ", S_COUNT };")
    out.append("static const wb_sample_t wb_samples[S_COUNT] = {")
    for name, n, rate in names:
        out.append("    {smp_%s, smp_%s_sh, %d, %d}," % (name.lower(), name.lower(), n, rate))
    out.append("};")
    out.append("#define WB_SAMPLE_BYTES %d" % total)
    Path(sys.argv[1]).write_text("\n".join(out) + "\n")
    print("samples: %d bytes" % total)


if __name__ == "__main__":
    main()
