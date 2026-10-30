#!/usr/bin/env python3
"""A little more static detune; frozen r1 MIDI and every other sound setting stay unchanged."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
PARENT = HERE.parent / "r1"
spec = importlib.util.spec_from_file_location("analog_r1", PARENT / "make_analog_studies.py")
parent = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parent)

RATIOS = {
    "low-battery-piano": 1.006,
    "slack-drum": 1.017,
    "brown-console": .994,
    "silt-bass": .996,
    "bent-aerial": 1.010,
    "melted-ensemble": 1.007,
    "oxide-rim": 2.022,
    "reel-clav": 2.006,
}


def files():
    content, entries = {}, []
    for sketch in parent.sketches():
        family = sketch["family"]
        original = json.loads((PARENT / (family + "-r1.nitridepreset")).read_text())
        before = original["values"]["fm_ratio"]
        original["values"]["fm_ratio"] = RATIOS[family]
        original["id"] = uuid.uuid5(uuid.NAMESPACE_URL, "nitride:analog-mood-study:" + family + "-r2").hex
        if not original["name"].endswith(" r1"):
            raise ValueError("Unexpected parent revision name")
        original["name"] = original["name"][:-3] + " r2"
        stem = family + "-r2"
        content[stem + ".nitridepreset"] = (json.dumps(original, indent=2) + "\n").encode()
        content[stem + ".mid"] = (PARENT / (family + "-r1.mid")).read_bytes()
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"],
                        "seconds": sketch["seconds"], "study_axis": sketch["axis"] + " / slightly stronger detune",
                        "design_lens": sketch["lens"] + " (imagined mood brief, not artist-authentic)",
                        "intent": f"r2 of {sketch['name']}: static FM ratio {before} to {RATIOS[family]}. "
                                  "Original MIDI, amplitude/FM/filter envelopes, tone, Output, tap mix and all LFO/drift settings unchanged; "
                                  "more relative separation rather than extra vibrato. " + sketch["exclude"]})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1,
                                        "revision": "r2", "sketches": entries}, indent=2) + "\n").encode()
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE / name).exists() for name in content):
        raise SystemExit("Studies exist; use --verify, never replace frozen revisions")
    for name, data in content.items():
        if args.verify:
            if (HERE / name).read_bytes() != data:
                raise SystemExit("Frozen analog r2 differs: " + name)
        else:
            with (HERE / name).open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
