#!/usr/bin/env python3
"""Original fixed artist-lens performances; --verify checks bytes, never overwrite."""
import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_review_midi", ROOT.parents[1] / "batch-02/r1/make_midi.py")
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)


def sketches():
    voice = [(0, 1.6, 67, 76), (2, .6, 69, 90), (3, 1.8, 72, 82),
             (6, .5, 71, 64), (6.75, 1.5, 67, 102),
             (10, 4, 64, 82), (10, 4, 71, 68), (16, .7, 72, 76),
             (17, .7, 74, 92), (18, 2, 71, 84), (24, 3, 67, 78)]
    # Deliberate overlaps trigger legato pitch/Tone drag; rests retrigger soft onset.
    body = [(0, 5, 48, 92), (4, 4, 60, 86), (7, 4, 49, 98),
            (10, 5, 55, 82), (18, .5, 48, 104), (19, .5, 60, 78),
            (22, 5, 51, 90), (26, 4, 63, 84), (29, 3, 52, 96)]
    # Long holds expose real audio chops; sparse note changes cannot create every pulse.
    cell = [(0, .35, 48, 96), (.8, .35, 55, 74), (1.65, .4, 60, 106),
            (3, .4, 51, 84), (4.7, .55, 55, 92),
            (8, 5.5, 48, 98), (8, 5.5, 55, 78),
            (16, 5, 51, 90), (24, 5, 58, 100)]
    return {"replica-voice-r1.mid": encoder.performance("Replica Voice r1 / original phrase study", 88, voice),
            "tendon-drift-r1.mid": encoder.performance("Tendon Drift r1 / original body study", 96, body),
            "socket-dance-r1.mid": encoder.performance("Socket Dance r1 / original cell study", 118, cell)}


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
