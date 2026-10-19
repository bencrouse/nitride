#!/usr/bin/env python3
"""Original imagined A.G. Cook taste briefs, not artist recipes. Never overwrite."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("pop_study_base", HERE.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec); spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "plastic-hero", "name": "Plastic Hero", "category": "Leads", "host_category": "Leads & Brass", "mono": True,
         "anchor": "review:copper-lift:r1", "axis": "foreground synthetic pop hook", "bpm": 144, "seconds": 22,
         "brief": "Shamelessly melodic glossy plastic lead: pointed index/Tone front settles into a simple strong held center. Mono legato all-pitch glide, small phrase-only common-direction scoop, tiny undelayed global vibrato. Be the hook, not a distortion demonstration.",
         "exclude": "No high network stress, opposed bend, long room, evolving ratio routes or rhythmic gating. Not another gritty/resonant experimental lead or an authentic artist recipe.",
         "values": {"coupling": .13, "stress": 0.0, "response": .035, "fm_depth": 1.0,
                    "amp_attack": .003, "amp_decay": .18, "amp_sustain": .85, "amp_release": .11,
                    "cutoff": 6200.0, "resonance": .12, "filter_key_tracking": .70, "filter_velocity": .35,
                    "filter_attack": .001, "filter_decay": .09, "filter_sustain": .15, "filter_release": .10, "filter_amount": 1.10,
                    "fm_ratio": 1.0, "fm_shape": 2.0, "fm_shape_amount": .42, "fm_operator_mix": .10,
                    "fm_attack": .001, "fm_decay": .055, "fm_sustain": .08, "fm_release": .10, "fm_envelope_amount": 1.1,
                    "glide_enabled": 1.0, "glide_time": .045, "glide_target": 2.0, "glide_legato": 1.0, "glide_curve": 2.0,
                    "legato": 1.0, "autobend_enabled": 1.0, "autobend_time": .035, "autobend_depth": -1.2,
                    "autobend_target": 2.0, "autobend_phrase": 1.0,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_wave": 1.0, "lfo_pitch_rate": 5.2, "lfo_pitch_depth": .035,
                    "space_mix": .10, "space_size": .22, "output": -4.52},
         "notes": [(0, 1, 60, 64), (4, 1.5, 60, 112), (8, .5, 72, 96), (8.5, .5, 76, 90),
                   (9, 1, 79, 108), (10.5, .7, 77, 88), (11.1, .8, 76, 100),
                   (16, .5, 72, 88), (16.5, .5, 76, 100), (17, 1, 79, 112), (18.5, 1.5, 81, 104),
                   (24, 6, 67, 96), (32, 2, 55, 96), (36, 2, 84, 88)]},
        {"family": "budget-heaven", "name": "Budget Heaven", "category": "Pads", "host_category": "Pads", "mono": False,
         "anchor": "review:soft-alloy:r2", "axis": "earnest bargain-keyboard strings", "bpm": 120, "seconds": 32,
         "brief": "Thin, exposed, buzzy electronic ensemble played with sincerity. Triangle-rich near-unison operator tap gives a cheap beating string edge, direct moderate entrance and sustained chords, shallow global vibrato. Keep artificiality audible instead of polishing it into an ambient wash.",
         "exclude": "No slow index/ratio/cutoff evolution, giant Space, chord helper, network distortion or huge attack/release. Near-unison tap is not a new chorus engine or independently detuned ensemble.",
         "values": {"coupling": .03, "stress": 0.0, "response": .025, "fm_depth": .50,
                    "amp_attack": .18, "amp_decay": 1.5, "amp_sustain": .95, "amp_release": .60,
                    "cutoff": 12500.0, "resonance": .04, "filter_key_tracking": .35, "filter_velocity": .15,
                    "fm_ratio": 1.008, "fm_shape": 1.0, "fm_shape_amount": .90, "fm_operator_mix": .36,
                    "fm_envelope_amount": 0.0,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_wave": 0.0, "lfo_pitch_rate": 5.6, "lfo_pitch_depth": .11,
                    "space_mix": .14, "space_size": .42, "output": -7.55},
         "notes": [(0, 2, 60, 64), (4, 2, 60, 104),
                   (8, 6, 48, 80), (8, 6, 55, 72), (8, 6, 59, 76), (8, 6, 64, 84),
                   (16, 6, 53, 80), (16, 6, 60, 72), (16, 6, 64, 76), (16, 6, 69, 84),
                   (24, 6, 55, 80), (24, 6, 62, 72), (24, 6, 65, 76), (24, 6, 71, 84),
                   (32, 4, 60, 88), (32, 4, 64, 80), (32, 4, 67, 84),
                   (40, 4, 72, 88), (40, 4, 76, 80), (40, 4, 79, 84)]},
        {"family": "sugar-teeth", "name": "Sugar Teeth", "category": "Physical materials", "host_category": "Keys & Organs", "mono": False,
         "anchor": "review:classic-tines:r1", "axis": "hard crystalline key with soft interior", "bpm": 144, "seconds": 22,
         "brief": "Tiny abrasive harmonic bite followed by a round restrained key body. Strong very brief shaped1:1FM index strike, short bright Tone contour and velocity opening; hard playing exposes the teeth, soft playing feels delicate. Shared Tone uses last-note velocity, not independent per-operator/per-voice velocity layers.",
         "exclude": "No inharmonic bell ratio, filter-FM tine body, slow pluck-vowel sweep, pitch entrance, stress distortion or long room. Not an acoustic piano, electric-piano model, sample layer or authentic artist recipe.",
         "values": {"coupling": .10, "stress": 0.0, "response": .025, "fm_depth": .08,
                    "amp_attack": .0015, "amp_decay": .55, "amp_sustain": .12, "amp_release": .16,
                    "cutoff": 3500.0, "resonance": .08, "filter_key_tracking": .5, "filter_velocity": 1.8,
                    "filter_attack": .001, "filter_decay": .070, "filter_sustain": 0.0, "filter_release": .12, "filter_amount": .50,
                    "fm_ratio": 1.0, "fm_shape": 2.0, "fm_shape_amount": .95, "fm_operator_mix": .04,
                    "fm_attack": .001, "fm_decay": .018, "fm_sustain": 0.0, "fm_release": .08, "fm_envelope_amount": 3.5,
                    "space_mix": .04, "space_size": .18, "output": -4.35},
         "notes": [(0, .6, 60, 40), (2, .6, 60, 80), (4, .6, 60, 120),
                   (8, .5, 60, 88), (8.75, .5, 64, 104), (9.5, .5, 67, 76), (10.25, 1, 72, 112),
                   (12, .5, 69, 84), (12.75, .5, 67, 100), (13.5, 1, 64, 92), (20, 5, 60, 96),
                   (28, 2, 48, 96), (32, 1, 84, 88),
                   (36, 3, 60, 88), (36, 3, 64, 84), (36, 3, 67, 80), (36, 3, 74, 92)]}
    ]


def files():
    content, entries = {}, []
    for sketch in sketches():
        values = base.neutral(); assert not set(sketch["values"]) - set(values)
        values.update(sketch["values"]); values["lfo_tempo"] = float(sketch["bpm"])
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6, "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:pop-study:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1", "mono": sketch["mono"], "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        content[stem + ".mid"] = base.encoder.performance(sketch["name"] + " / original synthetic pop brief", sketch["bpm"], sketch["notes"])
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"], "seconds": sketch["seconds"],
                        "study_axis": sketch["axis"], "design_lens": "A.G. Cook (imagined taste brief, not artist-authentic)",
                        "intent": sketch["brief"] + " " + sketch["exclude"]})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": entries}, indent=2) + "\n").encode()
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--verify", action="store_true"); args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE / name).exists() for name in content): raise SystemExit("Studies exist; use --verify, never replace frozen revisions")
    for name, data in content.items():
        if args.verify:
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen pop study differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
