#!/usr/bin/env python3
"""Fixed contrasting lead/sequence performances; no overwrite, --verify checks bytes."""
import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_review_midi", ROOT.parents[1] / "batch-02/r1/make_midi.py")
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)


def sketches():
    # 112 BPM: detached runs, repeated accents, longer notes and actual dyads.
    lead = [(0, .3, 60, 80), (.5, .3, 63, 94), (1, .3, 67, 106), (1.5, .3, 70, 88),
            (2, .6, 72, 112), (3, .3, 67, 64), (3.5, .3, 67, 102),
            (5, 1.5, 65, 96), (8, .55, 60, 108), (8, .55, 67, 84),
            (10, .55, 63, 94), (10, .55, 70, 78),
            (12, .3, 72, 80), (12.5, .3, 70, 92), (13, .3, 67, 100), (13.5, .3, 63, 86),
            (15, 2, 60, 98), (20, .5, 67, 104), (20, .5, 72, 88),
            (23, 2, 63, 90)]
    # 116 BPM: held root changes and overlaps; note event rate cannot create 16ths.
    sequence = [(0, 7.5, 48, 94), (8, 8.3, 55, 86), (16, 8.3, 51, 104),
                (24, 7.5, 58, 88), (34, 4, 48, 96)]
    return {"reed-snap-r1.mid": encoder.performance("Reed Snap r1 / fixed contrast", 112, lead),
            "undertow-grid-r1.mid": encoder.performance("Undertow Grid r1 / fixed contrast", 116, sequence)}


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
