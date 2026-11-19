#!/usr/bin/env python3
"""Expose Ripple's useful phase range; retain frozen r1 sources/MIDI and DSP."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("ripple_r1", HERE.parent / "r1/make_ripple_studies.py")
r1 = importlib.util.module_from_spec(spec); spec.loader.exec_module(r1)

UPDATES = {
    "phase-silk": {"cutoff": 180., "resonance": 0., "ripple_dry": .45, "ripple_drive": .9, "lfo_cutoff_rate": .25, "lfo_cutoff_depth": 2.5, "output": -6.},
    "negative-reed": {"cutoff": 260., "resonance": .15, "ripple_dry": .05, "ripple_feedback": -.9, "filter_amount": -.8, "output": 0.},
    "ripple-glass": {"cutoff": 350., "resonance": 0., "ripple_dry": .3, "ripple_drive": 1., "filter_amount": -1., "output": -6.},
    "positive-loom": {"cutoff": 180., "resonance": .1, "ripple_dry": .1, "ripple_feedback": .9, "lfo_cutoff_rate": .5, "lfo_cutoff_depth": 1.5, "output": 0.},
    "folded-prism": {"cutoff": 600., "resonance": 0., "ripple_dry": .25, "ripple_drive": 1., "filter_amount": 1., "output": -6.},
    "phase-vault": {"cutoff": 260., "resonance": .18, "ripple_dry": .05, "ripple_feedback": -.9, "lfo_cutoff_depth": 2., "output": 0.}}


def files():
    content, entries = {}, []
    for sketch in r1.sketches():
        family = sketch["family"]; stem = family + "-r2"; original = HERE.parent / "r1" / (family + "-r1.nitridepreset")
        definition = json.loads(original.read_text())
        definition["id"] = uuid.uuid5(uuid.NAMESPACE_URL, "nitride:ripple-study:" + stem).hex
        definition["name"] = definition["name"][:-3] + " r2"
        definition["values"].update(UPDATES[family])
        content[stem + ".nitridepreset"] = (json.dumps(definition, indent=2) + "\n").encode()
        content[stem + ".mid"] = original.with_suffix(".mid").read_bytes()
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"], "seconds": 16,
            "study_axis": sketch["axis"] + "; exposed phase range",
            "intent": "r2 moves the phase transition into the source's harmonics, broadens it and balances Dry against the actual wet level. "
                      "Same r1 MIDI, oscillator/FM/amp settings, type/mode and unchanged Ripple DSP. Fixed output trims recover wet attenuation; matched comparisons remove gain. "
                      "Independent Ripple-inspired implementation, not Pigments equivalence or factory approval."})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r2", "sketches": entries}, indent=2) + "\n").encode()
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--verify", action="store_true"); args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE / name).exists() for name in content): raise SystemExit("Studies exist; use --verify, never replace frozen revisions")
    for name, data in content.items():
        if args.verify:
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen Ripple r2 differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
