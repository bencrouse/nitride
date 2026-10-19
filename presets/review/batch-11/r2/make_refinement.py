#!/usr/bin/env python3
"""Attack-forward drum-machine tom refinement; preserve parent MIDI, never overwrite."""
import argparse
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parent / "r1/bowl-toms-r1.nitridepreset"
CHANGES = {
    "coupling": .07, "stress": 0.0, "response": .012, "fm_depth": .025,
    "amp_attack": .001, "amp_decay": .24, "amp_release": .09,
    "cutoff": 6500.0, "resonance": .06, "space_mix": 0.0,
    "autobend_time": .018, "autobend_depth": 12.0,
    "filter_decay": .012, "filter_release": .06, "filter_amount": 1.2,
    "filter_key_tracking": .55, "filter_velocity": 1.0,
    "fm_ratio": 2.0, "fm_shape": 0.0, "fm_shape_amount": 0.0,
    "fm_decay": .01, "fm_release": .025, "fm_envelope_amount": 2.8
}


def files():
    preset = json.loads(SOURCE.read_text())
    assert not set(CHANGES) - preset["values"].keys()
    preset["values"].update(CHANGES)
    preset["id"] = uuid.uuid5(uuid.NAMESPACE_URL, "nitride:percussion:bowl-toms-r2").hex
    preset["name"] = "Review / Physical materials / Bowl Toms r2"
    manifest = {"format": "nitride-review-batch", "version": 1, "revision": "r2", "sketches": [
        {"file": "bowl-toms-r2", "category": "Physical materials", "study_axis": "attack-forward drum-machine tom",
         "intent": "Respond to too much tone/not enough attack: short dry poly tom with almost bare harmonic body, brief 2:1 sine-FM strike, faster/deeper pitch entrance and open transient bandwidth. Same original MIDI/output as r1; not an exact drum-machine model, noise layer, phase reset or one-shot mode. Keep both praised kicks unchanged.",
         "seconds": 22}]}
    return {"bowl-toms-r2.nitridepreset": (json.dumps(preset, indent=2) + "\n").encode(),
            "bowl-toms-r2.mid": SOURCE.with_suffix(".mid").read_bytes(),
            "batch.json": (json.dumps(manifest, indent=2) + "\n").encode()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Refinement files already exist; use --verify, never replace frozen revisions/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen tom refinement differs: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
