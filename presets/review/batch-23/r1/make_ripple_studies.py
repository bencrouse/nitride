#!/usr/bin/env python3
"""Original Ripple auditions: deterministic recipes/MIDI; never replace frozen files."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("ripple_base", HERE.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "phase-silk", "name": "Phase Silk", "category": "Pads", "host_category": "Pads", "type": 0, "mode": 0,
         "axis": "gentle moving phase cancellation", "intent": "A dry-added two-section phase pad; staggered chord entrances retain independent filter envelopes.",
         "values": {"amp_attack": .35, "amp_decay": 1.2, "amp_sustain": .7, "amp_release": 1.4,
                    "fm_depth": .35, "fm_ratio": 1.006, "fm_shape": 1., "fm_shape_amount": .7, "fm_operator_mix": .35,
                    "cutoff": 1300., "resonance": .35, "ripple_dry": 1., "ripple_drive": .12,
                    "lfo_cutoff_enabled": 1., "lfo_cutoff_rate": .18, "lfo_cutoff_depth": 1.1},
         "notes": [(0, 7, 48, 80), (.5, 6.5, 55, 76), (1, 6, 59, 84), (1.5, 5.5, 64, 88),
                   (10, 6, 53, 84), (10.5, 5.5, 60, 80), (11, 5, 64, 88)]},
        {"family": "negative-reed", "name": "Negative Reed", "category": "Leads", "host_category": "Leads & Brass", "type": 0, "mode": 1,
         "axis": "negative feedback hollow reed", "intent": "Negative phase feedback hollows a triangle-rich lead; note velocity moves the phase center, not a low-pass edge.",
         "values": {"fm_depth": .6, "fm_ratio": 1., "fm_shape": 1., "fm_shape_amount": 1., "fm_operator_mix": .5,
                    "cutoff": 850., "resonance": .52, "ripple_dry": .35, "ripple_feedback": -.72,
                    "filter_attack": .02, "filter_decay": .25, "filter_sustain": .3, "filter_amount": 1.2, "filter_velocity": .8},
         "notes": [(0, 2, 60, 72), (3, 1, 64, 96), (5, 3, 67, 84), (10, 2, 65, 100), (13, 3, 60, 88)]},
        {"family": "ripple-glass", "name": "Ripple Glass", "category": "Leads", "host_category": "Keys & Organs", "type": 1, "mode": 0,
         "axis": "four-section shaped phase keys", "intent": "Four all-pass sections and moderate post-phase shaping produce bright electric keys with moving dry cancellation.",
         "values": {"amp_attack": .003, "amp_decay": .65, "amp_sustain": .2, "amp_release": .35,
                    "fm_depth": .9, "fm_ratio": 2., "fm_operator_mix": .15,
                    "cutoff": 2400., "resonance": .45, "ripple_dry": .8, "ripple_drive": .65,
                    "filter_attack": .002, "filter_decay": .35, "filter_sustain": .1, "filter_amount": 1.4},
         "notes": [(0, 1, 60, 64), (2, 1, 60, 104), (5, 4, 48, 84), (5.5, 3.5, 55, 80), (6, 3, 64, 88),
                   (11, 3, 53, 84), (11, 3, 60, 80), (11, 3, 64, 88), (16, 1, 72, 92)]},
        {"family": "positive-loom", "name": "Positive Loom", "category": "Sequence elements", "host_category": "Sequences", "type": 1, "mode": 1,
         "axis": "positive feedback phase pulse", "intent": "Positive feedback through four same-frequency all-pass sections; restrained cutoff motion and short notes expose the comb-like structure.",
         "values": {"amp_attack": .004, "amp_decay": .22, "amp_sustain": .5, "amp_release": .12,
                    "fm_depth": .8, "fm_ratio": 1., "fm_shape": 1., "fm_shape_amount": .8, "fm_operator_mix": .4,
                    "cutoff": 1100., "resonance": .6, "ripple_dry": .6, "ripple_feedback": .78,
                    "lfo_cutoff_enabled": 1., "lfo_cutoff_rate": .3, "lfo_cutoff_depth": .75},
         "notes": [(i*.75, .4, (48, 55, 60, 58)[i%4], (80, 96, 72, 104)[i%4]) for i in range(24)]},
        {"family": "folded-prism", "name": "Folded Prism", "category": "Physical materials", "host_category": "Drums & Percussion", "type": 2, "mode": 0,
         "axis": "eight-section shaped phase strike", "intent": "Eight all-pass sections concentrate a brief metallic strike before waveshaping. No copied sample, circuit or Pigments preset.",
         "values": {"amp_attack": .001, "amp_decay": .3, "amp_sustain": 0., "amp_release": .15,
                    "fm_depth": 1.2, "fm_ratio": 2.756, "fm_operator_mix": .2,
                    "cutoff": 2800., "resonance": .65, "ripple_dry": .65, "ripple_drive": .85,
                    "filter_attack": .001, "filter_decay": .13, "filter_sustain": 0., "filter_amount": 1.8},
         "notes": [(i*1.5, .2, (48, 60, 67)[i%3], (64, 96, 112)[i%3]) for i in range(12)]},
        {"family": "phase-vault", "name": "Phase Vault", "category": "Experimental", "host_category": "Textures", "type": 2, "mode": 1,
         "axis": "deep negative feedback phase texture", "intent": "Eight-section negative feedback with a slowly moving phase center and wet-dominant sum. The explicit feedback input attenuation is not AGC.",
         "values": {"amp_attack": .15, "amp_decay": 1., "amp_sustain": .65, "amp_release": .8,
                    "fm_depth": 1.4, "fm_ratio": 1.414, "fm_operator_mix": .25,
                    "cutoff": 1800., "resonance": .72, "ripple_dry": .2, "ripple_feedback": -.88,
                    "lfo_cutoff_enabled": 1., "lfo_cutoff_rate": .22, "lfo_cutoff_depth": 1.5},
         "notes": [(0, 5, 48, 84), (2, 3, 55, 80), (7, 5, 51, 88), (9, 3, 58, 76), (14, 3, 60, 92)]}
    ]


def files():
    content, entries = {}, []
    for sketch in sketches():
        values = {**base.neutral(), "filter_family": 0., "rage_character": 0., "rage_amount": 0.,
                  "ripple_enabled": 1., "ripple_type": float(sketch["type"]), "ripple_mode": float(sketch["mode"]),
                  "ripple_dry": .5, "ripple_drive": 0., "ripple_feedback": 0.}
        if set(sketch["values"]) - set(values): raise ValueError("Unknown Ripple parameter")
        values.update(sketch["values"]); values["output"] = -12.; values["lfo_tempo"] = 100.
        stem = sketch["family"] + "-r1"
        definition = {"format": "nitride-preset", "version": 8,
                      "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:ripple-study:" + stem).hex,
                      "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                      "mono": False, "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(definition, indent=2) + "\n").encode()
        content[stem + ".mid"] = base.encoder.performance(sketch["name"] + " / original Ripple study", 100, sketch["notes"])
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"], "seconds": 16,
                        "study_axis": sketch["axis"], "intent": sketch["intent"] + " Independent Ripple-inspired DSP; not an Arturia-identical model or sonic equivalence claim."})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": entries}, indent=2) + "\n").encode()
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--verify", action="store_true"); args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE / name).exists() for name in content): raise SystemExit("Studies exist; use --verify, never overwrite frozen revisions")
    for name, data in content.items():
        if args.verify:
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen Ripple study differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
