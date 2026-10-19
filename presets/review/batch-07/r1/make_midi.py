#!/usr/bin/env python3
"""Fixed original producer-lens studies; --verify reproduces bytes, never overwrite."""
import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_review_midi", ROOT.parents[1] / "batch-02/r1/make_midi.py")
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)


def sketches():
    elastic = [(0, .5, 48, 76), (1, .5, 48, 110), (2.5, .3, 55, 92),
               (4, .4, 60, 108), (5, .4, 63, 86), (6, .25, 60, 72),
               (6.5, .25, 60, 96), (6.75, .25, 67, 108),
               (9, 1.8, 55, 100), (12, .6, 48, 106), (12, .6, 60, 80),
               (16, .4, 63, 98), (18, 2, 48, 96), (24, 1, 60, 88)]
    # Open fifth/add-nine sets, gentle chord overlap and enough hold for slow fade-in.
    patient = [(0, 16, 48, 74), (0, 16, 55, 68), (0, 16, 62, 62),
               (14, 18, 53, 72), (14, 18, 60, 66), (14, 18, 67, 60)]
    # Unequal offbeat timing/ghost accents are deliberate MIDI choices, not preset swing.
    crooked = [(0, .7, 36, 104), (.68, .2, 36, 52), (1.1, .3, 43, 82),
               (1.74, .25, 39, 68), (2.12, .55, 36, 96), (3, .8, 46, 100),
               (3.7, .65, 43, 88), (4.45, .22, 36, 64), (5, .45, 36, 106),
               (5.7, .22, 39, 72), (6.14, .6, 41, 90), (7.02, .35, 43, 78),
               (8, 2, 36, 98), (12, .6, 39, 102), (12.67, .2, 39, 56),
               (13.12, .3, 46, 88), (14.08, .7, 43, 94), (14.7, .65, 36, 108),
               (18, 2, 36, 92), (24, 1.5, 41, 96)]
    return {"taffy-coil-r1.mid": encoder.performance("Taffy Coil r1 / original material study", 132, elastic),
            "stillwater-r1.mid": encoder.performance("Stillwater r1 / original patience study", 76, patient),
            "crooked-velvet-r1.mid": encoder.performance("Crooked Velvet r1 / original timing study", 92, crooked)}


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
