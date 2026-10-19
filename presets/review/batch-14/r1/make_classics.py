#!/usr/bin/env python3
"""Original Nitride approximations of three DX7 factory voices; --verify, never overwrite."""
import argparse
import importlib.util
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_classic_base", ROOT.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "classic-tines", "name": "Classic Tines", "category": "Physical materials", "mono": False,
         "reference": "epiano1", "reference_note_offset": 0, "axis": "DX7 ROM1A E.PIANO 1 approximation", "bpm": 100, "seconds": 26,
         "intent": "Dry FM-centered EP1 interpretation: warm1:1 main body, independently short shape strike and longer index decay, no pitch tricks. Original algorithm5 has three parallel detuned pairs including14:1 attack beyond Nitride ratio8 limit; no faithful algorithm/operator-EG/velocity/detune conversion claimed. Compare unchanged Tine Keys, not a rename.",
         "values": {"coupling": .035, "stress": 0.0, "response": .025, "fm_depth": .18,
                    "amp_attack": .0015, "amp_decay": 1.45, "amp_sustain": 0.0, "amp_release": .3,
                    "cutoff": 10500.0, "resonance": .05, "filter_key_tracking": .45, "filter_velocity": 1.8,
                    "filter_attack": .001, "filter_decay": .025, "filter_sustain": 0.0, "filter_release": .05,
                    "filter_amount": 0.0, "fm_ratio": 1.0, "fm_shape": 2.0, "fm_shape_amount": 0.0,
                    "fm_attack": .0015, "fm_decay": .65, "fm_sustain": .02, "fm_release": .3,
                    "fm_envelope_amount": 1.15, "fm_route_1_source": 3.0,
                    "fm_route_1_target": 2.0, "fm_route_1_amount": .85, "output": -4.7},
         "notes": [(0, 1, 60, 48), (2, 1, 60, 80), (4, 1, 60, 112),
                   (7, 3, 60, 88), (7, 3, 63, 80), (7, 3, 67, 76), (7, 3, 70, 72),
                   (13, 3, 53, 86), (13, 3, 60, 78), (13, 3, 65, 74),
                   (19, .5, 72, 76), (20, .5, 74, 94), (21, 2, 75, 106), (26, 3, 48, 96)]},
        {"family": "classic-bass", "name": "Classic Bass", "category": "Basses", "mono": True,
         "reference": "bass1", "reference_note_offset": 24, "axis": "DX7 ROM1A BASS 1 approximation", "bpm": 108, "seconds": 24,
         "intent": "Dry restrained1:1 bass body with brief harder FM/shape front, no glide/Autobend. Original algorithm16 feeds one.5carrier through three branches including9x/5x operators and feedback7. ROMglobal-12 plus.5carrier sounds24stbelow input; this Nitride utility maps root to played note, reference MIDI+24 explicitly aligns sound, not keyboard map. Compare Pocket Slap; not exact topology/velocity scaling.",
         "values": {"coupling": .055, "stress": 0.0, "response": .015, "fm_depth": .12,
                    "amp_attack": .001, "amp_decay": .38, "amp_sustain": .08, "amp_release": .085,
                    "cutoff": 5500.0, "resonance": .06, "filter_key_tracking": .5, "filter_velocity": 1.6,
                    "filter_attack": .001, "filter_decay": .022, "filter_sustain": 0.0,
                    "filter_release": .05, "filter_amount": 0.0,
                    "fm_ratio": 1.0, "fm_shape": 2.0, "fm_shape_amount": .08,
                    "fm_attack": .001, "fm_decay": .045, "fm_sustain": .04, "fm_release": .085,
                    "fm_envelope_amount": 2.2, "fm_route_1_source": 3.0,
                    "fm_route_1_target": 2.0, "fm_route_1_amount": .6, "output": -1.7},
         "notes": [(0, 1, 36, 48), (2, 1, 36, 80), (4, 1, 36, 112),
                   (6, .5, 36, 100), (6.75, .2, 36, 58), (7.5, .5, 43, 88),
                   (8.5, .5, 39, 82), (9.5, .8, 36, 106), (12, 3, 36, 96),
                   (18, .5, 41, 98), (19, .5, 43, 84), (20, .5, 46, 94),
                   (21, .5, 43, 76), (23, 2, 36, 104), (29, 2, 48, 94)]},
        {"family": "classic-tubes", "name": "Classic Tubes", "category": "Physical materials", "mono": False,
         "reference": "tub-bells", "reference_note_offset": 0, "axis": "DX7 ROM1A TUB BELLS approximation", "bpm": 90, "seconds": 36,
         "intent": "Dry long FM bell using nominal3.5:1 ratio and slowly relaxing index. Original algorithm5 has two detuned3.5:1 pairs plus a fixed~323.6Hz carrier strike; Nitride has no independent fixed-frequency layer/detuned pairs. Preserve clang/body/decay role, not exact six-op spectrum. Compare Hollow Bronze on identical notes.",
         "values": {"coupling": .025, "stress": 0.0, "response": .025, "fm_depth": .65,
                    "amp_attack": .0015, "amp_decay": 1.8, "amp_sustain": 0.0, "amp_release": 1.6,
                    "cutoff": 14000.0, "resonance": .05, "filter_key_tracking": .4, "filter_velocity": .25,
                    "filter_amount": 0.0, "fm_ratio": 3.5, "fm_shape": 0.0,
                    "fm_attack": .001, "fm_decay": 1.5, "fm_sustain": .2, "fm_release": 1.6,
                    "fm_envelope_amount": 2.2, "output": -3.4},
         "notes": [(0, 1, 60, 64), (5, 1, 60, 96), (10, 1, 60, 112),
                   (16, 2, 48, 96), (22, 2, 67, 96), (29, 4, 60, 96), (29, 4, 67, 78)]}
    ]


def files():
    result, manifest = {}, []
    expected = set(base.neutral())
    for sketch in sketches():
        values = base.neutral()
        assert not set(sketch["values"]) - expected
        values.update(sketch["values"]); values["lfo_tempo"] = float(sketch["bpm"])
        assert set(values) == expected
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:factory-inspired:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                  "mono": sketch["mono"], "chord": False, "values": values}
        result[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        result[stem + ".mid"] = base.encoder.performance(sketch["name"] + " r1 / original ROM-inspired audition", sketch["bpm"], sketch["notes"])
        manifest.append({"file": stem, "category": sketch["category"], "study_axis": sketch["axis"], "intent": sketch["intent"], "seconds": sketch["seconds"]})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": manifest}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Classic files exist; use --verify, never replace frozen presets/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen classic differs: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
