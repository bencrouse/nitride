#!/usr/bin/env python3
"""Original Nitride FM-archetype recipes/MIDI; --verify checks bytes, never overwrite."""
import argparse
import importlib.util
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_frontier_base", ROOT.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "rosewood-tap", "name": "Rosewood Tap", "category": "Physical materials", "mono": False,
         "axis": "wooden mallet", "seconds": 20, "bpm": 104,
         "intent": "Nitride interpretation of a warm pitched wooden mallet: stable pitch, brief harmonic index strike and quick damping, not a sampled marimba or physical model. Contrast Hollow Bronze ringing and Skin's pitch drop.",
         "values": {"coupling": .14, "stress": .018, "fm_depth": .1, "amp_attack": .0015,
                    "amp_decay": .38, "amp_sustain": 0.0, "amp_release": .12, "cutoff": 3500.0,
                    "resonance": .12, "filter_key_tracking": .65, "filter_velocity": 1.0,
                    "filter_attack": .001, "filter_decay": .04, "filter_sustain": .08,
                    "filter_release": .1, "filter_amount": .65, "fm_ratio": 3.0,
                    "fm_attack": .001, "fm_decay": .032, "fm_sustain": 0.0, "fm_release": .05,
                    "fm_envelope_amount": 2.1, "space_mix": .025, "space_size": .22, "output": -2.8},
         "notes": [(0, .6, 60, 62), (1, .6, 60, 90), (2, .6, 60, 112), (4, .4, 64, 82),
                   (4.75, .4, 67, 98), (5.5, .4, 72, 88), (7, 1.5, 60, 96), (7, 1.5, 67, 76),
                   (11, .2, 72, 74), (11.5, .2, 74, 88), (12, .2, 76, 104),
                   (15, 2.5, 55, 100), (21, .5, 79, 86)]},
        {"family": "clear-stops", "name": "Clear Stops", "category": "Leads", "mono": False,
         "axis": "simple sustained organ", "seconds": 24, "bpm": 100,
         "intent": "Nitride interpretation of compact dry sustained organ keys: even amplitude, stable harmonic carrier/operator blend, no cyclic motion or long ambience. Not a drawbar/tonewheel emulation. Contrast articulated Reed Snap.",
         "values": {"coupling": .08, "response": .08, "fm_depth": .045, "amp_attack": .004,
                    "amp_decay": .02, "amp_sustain": 1.0, "amp_release": .065,
                    "cutoff": 10000.0, "resonance": .05, "filter_key_tracking": .6,
                    "fm_ratio": 2.0, "fm_shape": 1.0, "fm_shape_amount": .18, "fm_operator_mix": .32,
                    "output": -8.9},
         "notes": [(0, 3, 48, 84), (0, 3, 55, 76), (0, 3, 60, 72),
                   (4, .5, 60, 90), (5, .5, 64, 90), (6, .5, 67, 90),
                   (8, 5, 53, 82), (8, 5, 60, 76), (8, 5, 65, 70),
                   (16, 3, 55, 88), (16, 3, 62, 80), (16, 3, 67, 74), (24, 3, 60, 86)]},
        {"family": "copper-lift", "name": "Copper Lift", "category": "Leads", "mono": False,
         "axis": "harmonic brass and stabs", "seconds": 24, "bpm": 112,
         "intent": "Nitride interpretation of harmonic FM brass: poly stabs and held swells, rising index/Tone entrance settling to solid body, small network color. Not an acoustic brass model or copied DX algorithm. Contrast mono Glass Current.",
         "values": {"coupling": .22, "stress": .035, "response": .065, "fm_depth": .35,
                    "amp_attack": .028, "amp_decay": .36, "amp_sustain": .72, "amp_release": .18,
                    "cutoff": 2200.0, "resonance": .16, "filter_key_tracking": .65, "filter_velocity": .9,
                    "filter_attack": .045, "filter_decay": .42, "filter_sustain": .4,
                    "filter_release": .16, "filter_amount": 1.8, "filter_fm_depth": .015,
                    "fm_ratio": 1.0, "fm_shape": 1.0, "fm_shape_amount": .4, "fm_operator_mix": .035,
                    "fm_attack": .035, "fm_decay": .32, "fm_sustain": .45, "fm_release": .18,
                    "fm_envelope_amount": 1.45, "fm_route_1_source": 4.0,
                    "fm_route_1_target": 0.0, "fm_route_1_amount": .055,
                    "space_mix": .06, "space_size": .25, "output": -7.25},
         "notes": [(0, .55, 53, 88), (0, .55, 60, 80), (0, .55, 65, 74),
                   (2, .55, 55, 104), (2, .55, 62, 94), (2, .55, 67, 88),
                   (5, 4, 53, 90), (5, 4, 60, 82), (5, 4, 65, 76),
                   (12, .6, 60, 78), (13, .6, 62, 94), (14, 2, 65, 100),
                   (20, 4, 55, 96), (20, 4, 62, 86), (20, 4, 67, 80)]},
        {"family": "pocket-slap", "name": "Pocket Slap", "category": "Basses", "mono": True,
         "axis": "concise FM slap bass", "seconds": 20, "bpm": 108,
         "intent": "Nitride interpretation of dry classic FM slap/rubber bass: short bright index pop, clear settled fundamental and concise stops; no glide, Autobend, cyclic wobble or Space. Must earn distinct utility against Crooked Velvet, not an exact classic preset clone.",
         "values": {"coupling": .18, "stress": .025, "response": .035, "fm_depth": .16,
                    "amp_attack": .002, "amp_decay": .22, "amp_sustain": .38, "amp_release": .075,
                    "cutoff": 1100.0, "resonance": .2, "filter_key_tracking": .55, "filter_velocity": 1.2,
                    "filter_attack": .001, "filter_decay": .075, "filter_sustain": .12,
                    "filter_release": .08, "filter_amount": 1.1, "filter_fm_depth": .01,
                    "fm_ratio": 1.0, "fm_attack": .001, "fm_decay": .035,
                    "fm_sustain": .03, "fm_release": .07, "fm_envelope_amount": 2.0, "output": -2.95},
         "notes": [(0, .6, 36, 102), (.75, .2, 36, 54), (1.5, .3, 43, 88),
                   (2.25, .3, 39, 76), (3, .65, 36, 108), (4.5, .35, 48, 94),
                   (6, 2, 36, 96), (10, .5, 41, 100), (10.75, .2, 41, 60),
                   (11.5, .4, 43, 92), (13, .8, 46, 86), (13.7, .8, 43, 104),
                   (17, 2.5, 36, 98), (23, 1.5, 41, 94)]}
    ]


def files():
    result, manifest = {}, []
    expected = set(json.loads((ROOT.parents[1] / "batch-09/r1/sub-canvas-r1.nitridepreset").read_text())["values"])
    for sketch in sketches():
        values = base.neutral()
        assert not set(sketch["values"]) - expected
        values.update(sketch["values"])
        values["lfo_tempo"] = float(sketch["bpm"])
        assert set(values) == expected
        stem = sketch["family"] + "-r1"
        data = {"format": "nitride-preset", "version": 6,
                "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:archetype:" + stem).hex,
                "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                "mono": sketch["mono"], "chord": False, "values": values}
        result[stem + ".nitridepreset"] = (json.dumps(data, indent=2) + "\n").encode()
        result[stem + ".mid"] = base.encoder.performance(sketch["name"] + " r1 / original archetype", sketch["bpm"], sketch["notes"])
        manifest.append({"file": stem, "category": sketch["category"], "study_axis": sketch["axis"],
                         "intent": sketch["intent"], "seconds": sketch["seconds"]})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": manifest}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Archetype files already exist; use --verify, never replace frozen presets/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen archetype differs from generator: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
