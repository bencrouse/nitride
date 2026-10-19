#!/usr/bin/env python3
"""Fixed contrasting pad/bass performances; no overwrite, --verify checks bytes."""
import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_review_midi", ROOT.parents[1] / "batch-02/r1/make_midi.py")
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)


def sketches():
    # 104 BPM: actual held triads; the synth supplies rhythmic timbre, not notes.
    pad = []
    for beat, pitches, velocities in (
        (0, (60, 67, 70), (88, 76, 82)),
        (8, (58, 65, 69), (78, 86, 74)),
        (16, (55, 62, 67), (96, 80, 90)),
        (24, (60, 64, 71), (82, 92, 76))
    ):
        for pitch, velocity in zip(pitches, velocities):
            pad.append((beat, 6, pitch, velocity))
    # 120 BPM: separated damped hits, velocity contrast, low octave and higher fill.
    pitches = [36, 36, 43, 39, 36, 46, 43, 34, 24, 24, 31, 27, 36, 43, 48, 36]
    velocities = [104, 68, 88, 78, 114, 72, 94, 82, 106, 70, 92, 76, 102, 84, 96, 88]
    bass = [(i, .55, pitch, velocity) for i, (pitch, velocity) in enumerate(zip(pitches, velocities))]
    bass += [(17, 2.5, 36, 98)]  # Holding exposes low sustain, not a legato slide.
    return {"prism-pulse-r1.mid": encoder.performance("Prism Pulse r1 / fixed contrast", 104, pad),
            "felt-weight-r1.mid": encoder.performance("Felt Weight r1 / fixed contrast", 120, bass)}


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
