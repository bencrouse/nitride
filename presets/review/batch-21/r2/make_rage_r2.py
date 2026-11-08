#!/usr/bin/env python3
"""Stronger Rage r2 auditions. Preserve r1 definitions, MIDI and evaluation audio."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("rage_r1", HERE.parent / "r1/make_rage_studies.py")
r1 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r1)


def files():
    result, entries = {}, []
    for sketch, cutoff, resonance in zip(r1.sketches(), (560, 950, 950, 650, 1400), (.82, .82, .78, .83, .83)):
        family = sketch["family"]; old = HERE.parent / "r1" / (family + "-r1.nitridepreset")
        definition = json.loads(old.read_text()); stem = family + "-r2"
        definition["id"] = uuid.uuid5(uuid.NAMESPACE_URL, "nitride:nonlinear-feedback-study:" + stem).hex
        definition["name"] = definition["name"][:-3] + " r2"
        definition["values"].update({"rage_amount": 1.0, "cutoff": float(cutoff), "resonance": resonance, "output": -4.0})
        result[stem + ".nitridepreset"] = (json.dumps(definition, indent=2) + "\n").encode()
        result[stem + ".mid"] = (HERE.parent / "r1" / (family + "-r1.mid")).read_bytes()
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"], "seconds": 28,
                        "study_axis": sketch["axis"] + "; full-range residual feedback",
                        "intent": sketch["intent"] + " Full Rage with a more exposed filter range. Original r1 MIDI/source retained; r1 on a new AU also uses the new DSP. Use archived eval-039 audio for an exact old-engine comparison, not the r1 label alone."})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r2", "sketches": entries}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--verify", action="store_true"); args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE/name).exists() for name in content): raise SystemExit("Studies exist; use --verify, never replace frozen revisions")
    for name, data in content.items():
        if args.verify:
            if (HERE/name).read_bytes() != data: raise SystemExit("Frozen Rage r2 differs: " + name)
        else:
            with (HERE/name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
