#!/usr/bin/env python3
"""Original five-character nonlinear-feedback studies; never replace frozen revisions."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("rage_study_base", HERE.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "diode-current", "name": "Diode Current", "category": "Basses", "host_category": "Bass", "mono": True,
         "character": 0, "anchor": "review:crooked-velvet:r2", "axis": "asymmetric feedback acid bass",
         "intent": "A dry, resonant acid-like bass. Harder notes open a voice-local filter; asymmetric diode-like feedback colors the resonant bite, rather than adding post-filter distortion.",
         "values": {"coupling": .08, "fm_depth": .65, "fm_ratio": 1.0, "fm_operator_mix": .10,
                    "amp_attack": .004, "amp_decay": .3, "amp_sustain": .75, "amp_release": .15,
                    "cutoff": 420.0, "resonance": .72, "filter_key_tracking": .75, "filter_velocity": .8,
                    "filter_attack": .002, "filter_decay": .14, "filter_sustain": .10, "filter_release": .08, "filter_amount": 3.2,
                    "rage_amount": .78, "output": -10.0},
         "notes": [(0, 4, 36, 88), (6, .35, 36, 64), (7, .35, 36, 112), (8, .35, 43, 88), (9, .35, 41, 96),
                   (12, 5, 36, 100), (20, 3, 41, 88), (26, 3, 48, 92), (32, 2, 31, 88)]},
        {"family": "pressure-brass", "name": "Pressure Brass", "category": "Leads", "host_category": "Leads & Brass", "mono": True,
         "character": 1, "anchor": "review:copper-lift:r1", "axis": "balanced feedback overdrive in a brass-like lead",
         "intent": "A forceful brass-like lead with a short opening vowel and balanced feedback overdrive. Detached phrases expose each filter articulation; not an acoustic brass or transistor circuit model.",
         "values": {"coupling": .20, "stress": .02, "fm_depth": 1.15, "fm_ratio": 1.0, "fm_shape": 1.0, "fm_shape_amount": .7,
                    "amp_attack": .025, "amp_decay": .4, "amp_sustain": .85, "amp_release": .35,
                    "cutoff": 1500.0, "resonance": .56, "filter_key_tracking": .6, "filter_velocity": .4,
                    "filter_attack": .09, "filter_decay": .3, "filter_sustain": .45, "filter_release": .3, "filter_amount": 1.4,
                    "rage_amount": .72, "output": -10.0},
         "notes": [(0, 4, 60, 88), (6, .5, 60, 72), (7, .5, 64, 96), (8, .5, 67, 88), (10, 4, 72, 96),
                   (18, 4, 65, 88), (26, 5, 55, 100), (34, 2, 60, 64)]},
        {"family": "soft-circuit", "name": "Soft Circuit", "category": "Leads", "host_category": "Keys & Organs", "mono": False,
         "character": 2, "anchor": "review:afterimage-keys:r1", "axis": "soft feedback electric keys with independent chord articulation",
         "intent": "Rounded electric keys with a brief independent FM strike and soft feedback saturation. Staggered chord entrances demonstrate that a new note does not retrigger the other voices' filter envelopes.",
         "values": {"coupling": .04, "fm_depth": .35, "fm_ratio": 2.0,
                    "fm_attack": .001, "fm_decay": .09, "fm_sustain": 0.0, "fm_release": .2, "fm_envelope_amount": .8,
                    "amp_attack": .003, "amp_decay": .9, "amp_sustain": .20, "amp_release": .4,
                    "cutoff": 2600.0, "resonance": .58, "filter_key_tracking": .7, "filter_velocity": .6,
                    "filter_attack": .001, "filter_decay": .3, "filter_sustain": .25, "filter_release": .3, "filter_amount": 1.5,
                    "rage_amount": .68, "space_mix": .12, "space_size": .3, "output": -10.0},
         "notes": [(0, 3, 60, 64), (5, 3, 60, 104), (11, 6, 48, 84), (11.5, 5.5, 55, 80), (12, 5, 59, 88), (12.5, 4.5, 64, 92),
                   (22, 4, 53, 84), (22, 4, 60, 80), (22, 4, 64, 88), (30, 3, 72, 88)]},
        {"family": "spool-horizon", "name": "Spool Horizon", "category": "Pads", "host_category": "Pads", "mono": False,
         "character": 3, "anchor": "review:budget-heaven:r1", "axis": "slow beating pad with memory-shaped feedback rounding",
         "intent": "A warm, slowly opening chord with near-unison beating and tape-inspired feedback compression/rounding. No tape recording, physical hysteresis or reversed room is claimed; ordinary Space remains downstream.",
         "values": {"coupling": .025, "fm_depth": .25, "fm_ratio": 1.006, "fm_shape": 1.0, "fm_shape_amount": .6, "fm_operator_mix": .5,
                    "amp_attack": .7, "amp_decay": 2.0, "amp_sustain": .8, "amp_release": 3.0,
                    "cutoff": 1400.0, "resonance": .70, "filter_key_tracking": .4,
                    "filter_attack": 1.2, "filter_decay": 1.7, "filter_sustain": .55, "filter_release": 2.6, "filter_amount": 1.5,
                    "rage_amount": .68, "space_mix": .2, "space_size": .6, "output": -10.0},
         "notes": [(0, 8, 48, 80), (0, 8, 55, 76), (0, 8, 59, 84), (0, 8, 64, 88),
                   (14, 8, 53, 84), (14, 8, 60, 80), (14, 8, 64, 88), (26, 5, 55, 84), (26, 5, 62, 80), (26, 5, 65, 88)]},
        {"family": "switch-grain", "name": "Switch Grain", "category": "Physical materials", "host_category": "Drums & Percussion", "mono": False,
         "character": 4, "anchor": "review:oxide-rim:r2", "axis": "short bright asymmetric feedback percussion",
         "intent": "A short papery metallic hit. Biased hard feedback clipping and voice-local audio-rate cutoff FM make the bright edge; no noise samples, random hit variation or separate post-distortion stage.",
         "values": {"coupling": .10, "stress": .04, "fm_depth": 1.1, "fm_ratio": 2.756,
                    "amp_attack": .001, "amp_decay": .18, "amp_sustain": 0.0, "amp_release": .12,
                    "cutoff": 2600.0, "resonance": .72, "filter_key_tracking": .45, "filter_fm_depth": 1.1,
                    "filter_attack": .001, "filter_decay": .07, "filter_sustain": 0.0, "filter_release": .08, "filter_amount": 1.8,
                    "rage_amount": .90, "space_mix": .04, "space_size": .15, "output": -10.0},
         "notes": [(i * .75, .12, 60, (64, 96, 80, 112)[i % 4]) for i in range(16)]
                   + [(15, 3, 48, 88), (21, .15, 72, 96), (24, .15, 60, 88), (27, .15, 67, 88), (30, 2, 60, 100)]}
    ]


def files():
    content, entries = {}, []
    for sketch in sketches():
        values = {**base.neutral(), "filter_family": 1.0, "rage_character": float(sketch["character"]), "rage_amount": 0.0}
        if set(sketch["values"]) - set(values): raise ValueError("Unknown filter-study parameter")
        values.update(sketch["values"]);values["lfo_tempo"] = 100.0
        stem = sketch["family"] + "-r1"
        definition = {"format": "nitride-preset", "version": 7,
                      "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:nonlinear-feedback-study:" + stem).hex,
                      "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                      "mono": sketch["mono"], "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(definition, indent=2) + "\n").encode()
        content[stem + ".mid"] = base.encoder.performance(sketch["name"] + " / original nonlinear-feedback study", 100, sketch["notes"])
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"], "seconds": 28,
                        "study_axis": sketch["axis"], "intent": sketch["intent"] + " Independent Rage-inspired implementation, not copied Arturia presets or a verified Pigments sonic match."})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": entries}, indent=2) + "\n").encode()
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument("--verify", action="store_true");args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE / name).exists() for name in content): raise SystemExit("Studies exist; use --verify, never replace frozen revisions")
    for name, data in content.items():
        if args.verify:
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen Rage study differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
