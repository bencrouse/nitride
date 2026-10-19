#!/usr/bin/env python3
"""Fixed contrasting material/experimental performances; never overwrite frozen MIDI."""
import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_review_midi", ROOT.parents[1] / "batch-02/r1/make_midi.py")
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)


def sketches():
    # 100 BPM: velocity ladder, detached hits, roll, dyad and longer-held strike.
    impacts = [(0, .4, 43, 56), (1, .4, 43, 80), (2, .4, 43, 108),
               (4, .3, 48, 96), (5, .3, 55, 82), (6, .3, 60, 104),
               (8, .2, 43, 72), (8.5, .2, 43, 90), (9, .2, 43, 110),
               (9.25, .15, 50, 76), (9.5, .15, 50, 94), (9.75, .15, 50, 108),
               (12, .75, 48, 100), (12, .75, 55, 78),
               (15, 2, 43, 102), (19, .5, 60, 88)]
    # 96 BPM: isolated glints, sustained pitch-step exposure and true overlapping dyads.
    shards = [(0, 2.5, 60, 84), (4, 3, 67, 92), (8, 8, 63, 88),
              (10, 4, 70, 72), (18, 5, 60, 94), (18, 5, 67, 78),
              (27, 5, 65, 86)]
    return {"skin-knock-r1.mid": encoder.performance("Skin Knock r1 / fixed contrast", 100, impacts),
            "orbit-shards-r1.mid": encoder.performance("Orbit Shards r1 / fixed contrast", 96, shards)}


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
