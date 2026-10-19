#!/usr/bin/env python3
"""Deterministic SMF performances; refuses overwrites, --verify checks frozen bytes."""
import argparse
from pathlib import Path
import struct

PPQ = 960
ROOT = Path(__file__).resolve().parent


def vlq(value):
    assert 0 <= value <= 0x0fffffff
    result = [value & 0x7f]
    while value >> 7:
        value >>= 7
        result.insert(0, 0x80 | (value & 0x7f))
    return bytes(result)


def performance(name, bpm, notes, bends=()):
    events = [(0, bytes([0xff, 0x51, 3]) + round(60_000_000 / bpm).to_bytes(3, "big")),
              (0, bytes([0xff, 0x58, 4, 4, 2, 24, 8])),
              (0, b"\xff\x03" + vlq(len(name.encode())) + name.encode())]
    for beat, duration, pitch, velocity in notes:
        events.append((round(beat * PPQ), bytes([0x90, pitch, velocity])))
        events.append((round((beat + duration) * PPQ), bytes([0x80, pitch, 0])))
    for beat, value in bends:
        events.append((round(beat * PPQ), bytes([0xe0, value & 0x7f, value >> 7])))
    # At coincident boundaries release old notes before starting new notes.
    events.sort(key=lambda e: (e[0], 0 if e[1][0] == 0xff else 1 if e[1][0] == 0x80 else 2))
    track, previous = bytearray(), 0
    for tick, event in events:
        track += vlq(tick - previous) + event
        previous = tick
    track += b"\x00\xff\x2f\x00"
    return b"MThd" + struct.pack(">IHHH", 6, 0, 1, PPQ) + b"MTrk" + struct.pack(">I", len(track)) + track


def sketches():
    # 100 BPM: separated notes, overlapping phrases, velocity and held-note bend.
    lead = [(0, .7, 60, 74), (1, .7, 63, 86), (2, 1.25, 67, 102),
            (3, 1.25, 70, 92), (4, 2, 72, 108),
            (7, .75, 67, 70), (8, 1.3, 65, 83), (9, 1.3, 63, 99),
            (10, 2, 60, 89), (13, 3, 72, 105),
            (17, .4, 67, 64), (17.75, .4, 70, 82), (18.5, .4, 72, 101),
            (19.25, 1.5, 75, 114), (22, 1.4, 72, 95),
            (23, 1.4, 70, 88), (24, 1.4, 67, 100), (25, 2, 63, 80),
            (29, 3, 60, 92)]
    bends = [(13, 8192), (13.5, 9216), (14, 11264), (14.5, 12288),
             (15, 11264), (15.5, 9216), (16, 8192)]
    # 120 BPM: eight bars, intentional rests and occasional two-note accents.
    pitches = [48, 55, 60, 63, 55, 58, 67, 60]
    velocities = [108, 68, 86, 72, 98, 64, 90, 76]
    sequence = []
    for step in range(64):
        if step % 16 in (7, 15):
            continue
        note = pitches[step % 8] + (5 if step >= 32 else 0)
        sequence.append((step * .5, .18 if step % 4 else .3, note, velocities[step % 8]))
        if step % 16 == 0:
            sequence.append((step * .5, .3, note + 12, 76))
    return {"glass-current-r1.mid": performance("Glass Current r1 / fixed review", 100, lead, bends),
            "pulse-lattice-r1.mid": performance("Pulse Lattice r1 / fixed review", 120, sequence)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    data = sketches()
    if not args.verify and any((ROOT / name).exists() for name in data):
        raise SystemExit("MIDI already exists; use --verify, never replace frozen performances")
    for name, content in data.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != content:
                raise SystemExit("Frozen MIDI differs from its generator: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(content)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
