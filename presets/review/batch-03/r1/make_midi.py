#!/usr/bin/env python3
"""Fixed material/experimental SMF demos; refuses overwrite, --verify checks bytes."""
import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parent
# Reuse the frozen batch-02 standard-library SMF encoder; no DSP or host shortcut.
ENCODER = ROOT.parents[1] / "batch-02/r1/make_midi.py"
spec = importlib.util.spec_from_file_location("nitride_review_midi", ENCODER)
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)


def sketches():
    # 80 BPM: three velocities on one key, lower/upper strikes, held decay and dyads.
    material = [(0, .6, 57, 48), (3, .6, 57, 82), (6, .6, 57, 114),
                (9, .6, 45, 94), (12, .6, 69, 90),
                (15, 3, 60, 106), (19, .5, 57, 94), (19, .5, 64, 78),
                (22, .6, 52, 82), (22, .6, 59, 68)]
    # 90 BPM: held blooms, deliberately overlapping intervals, separated restart.
    experimental = [(0, 4, 48, 88), (6, 4, 55, 104), (9, 4, 61, 94),
                    (12, 5, 48, 100), (20, 4, 60, 70), (23, 4, 67, 98),
                    (26, 4, 58, 110), (32, 3, 48, 86)]
    return {"hollow-bronze-r1.mid": encoder.performance("Hollow Bronze r1 / fixed review", 80, material),
            "fault-bloom-r1.mid": encoder.performance("Fault Bloom r1 / fixed review", 90, experimental)}


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
