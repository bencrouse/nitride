#!/usr/bin/env python3
"""Original Nitride kick/tom recipes and MIDI; --verify checks bytes, never overwrite."""
import argparse
import importlib.util
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_drum_base", ROOT.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "floor-pulse", "name": "Floor Pulse", "mono": True,
         "axis": "rounded low kick", "seconds": 20, "bpm": 120,
         "intent": "Dry rounded low kick: fast two-octave downward entrance over an almost bare low body, no ring/Space or cyclic motion. MIDI 36 with Pitch -3 nominally settles at 55 Hz. Not a one-shot, phase-reset oscillator or drum-machine clone; note length/release still matter. Contrast Skin Knock and the shorter harmonic Rivet kick.",
         "values": {"coupling": .025, "response": .012, "fm_depth": .015, "pitch": -3.0,
                    "amp_attack": .001, "amp_decay": .36, "amp_sustain": 0.0, "amp_release": .16,
                    "cutoff": 1300.0, "resonance": .05, "filter_key_tracking": .2, "filter_velocity": .15,
                    "filter_attack": .001, "filter_decay": .018, "filter_sustain": 0.0,
                    "filter_release": .08, "filter_amount": .4, "fm_ratio": 1.0,
                    "fm_attack": .001, "fm_decay": .012, "fm_sustain": 0.0, "fm_release": .025,
                    "fm_envelope_amount": .2, "autobend_enabled": 1.0, "autobend_time": .026,
                    "autobend_depth": 24.0, "autobend_target": 2.0, "output": -3.55},
         "notes": [(0, 1, 36, 72), (2, 1, 36, 100), (4, 1, 36, 118),
                   (6, .8, 36, 104), (7, .8, 36, 92), (8, .8, 36, 108), (9, .3, 36, 58),
                   (9.5, .4, 36, 94), (10.5, .8, 36, 110), (12, 1, 33, 102),
                   (14, 1, 40, 102), (16, .4, 36, 104), (16.5, .4, 36, 80),
                   (17, .4, 36, 106), (17.5, .4, 36, 88), (20, 3, 36, 104),
                   (25, .05, 36, 104), (27, 1, 36, 104)]},
        {"family": "rivet-kick", "name": "Rivet Kick", "mono": True,
         "axis": "compact harmonic kick", "seconds": 20, "bpm": 128,
         "intent": "Hard compact synthetic kick: short harmonic FM/index bite and stronger network over a rapidly damped body, no ambience. A tactile synthetic front, not Floor Pulse with more gain or a noise-click layer. MIDI 36 with Pitch -1 nominally settles near 61.74 Hz. No one-shot/phase-reset added; compare both kicks on identical notes.",
         "values": {"coupling": .34, "stress": .065, "response": .012, "fm_depth": .1, "pitch": -1.0,
                    "amp_attack": .001, "amp_decay": .17, "amp_sustain": 0.0, "amp_release": .085,
                    "cutoff": 2400.0, "resonance": .11, "filter_key_tracking": .25, "filter_velocity": .7,
                    "filter_attack": .001, "filter_decay": .016, "filter_sustain": 0.0,
                    "filter_release": .05, "filter_amount": 1.5, "filter_fm_depth": .025,
                    "fm_ratio": 1.0, "fm_shape": 2.0, "fm_shape_amount": .35,
                    "fm_attack": .001, "fm_decay": .014, "fm_sustain": 0.0, "fm_release": .025,
                    "fm_envelope_amount": 2.6, "autobend_enabled": 1.0, "autobend_time": .012,
                    "autobend_depth": 30.0, "autobend_target": 0.0, "output": -1.45},
         "notes": [(0, 1, 36, 72), (2, 1, 36, 100), (4, 1, 36, 118),
                   (6, .55, 36, 106), (7, .55, 36, 104), (7.75, .2, 36, 58),
                   (8.5, .4, 36, 100), (9.5, .4, 36, 110), (10.25, .2, 36, 68),
                   (12, .5, 33, 100), (13, .5, 40, 100), (15, .16, 36, 90),
                   (15.25, .16, 36, 108), (15.5, .16, 36, 76), (15.75, .16, 36, 114),
                   (18, 3, 36, 104), (24, .05, 36, 104), (26, 1, 36, 104)]},
        {"family": "bowl-toms", "name": "Bowl Toms", "mono": False,
         "axis": "pitched floor and rack toms", "seconds": 22, "bpm": 108,
         "intent": "Poly tuned tom fills: gently relaxing pitch with a longer lightly inharmonic hollow body, room kept minimal. Intended MIDI 41-57; no GM drum map, sampled shell or membrane model. Lower FM/index and gentler/slower bend than Skin Knock, with overlapping pitches allowed to ring. Shared Tone still follows note ownership. Gate length matters; not a one-shot.",
         "values": {"coupling": .13, "stress": .012, "response": .028, "fm_depth": .22,
                    "amp_attack": .0015, "amp_decay": .62, "amp_sustain": 0.0, "amp_release": .28,
                    "cutoff": 2100.0, "resonance": .16, "filter_key_tracking": .7, "filter_velocity": .75,
                    "filter_attack": .001, "filter_decay": .055, "filter_sustain": 0.0,
                    "filter_release": .16, "filter_amount": .8, "fm_ratio": 1.48,
                    "fm_shape": 1.0, "fm_shape_amount": .12,
                    "fm_attack": .001, "fm_decay": .06, "fm_sustain": 0.0, "fm_release": .12,
                    "fm_envelope_amount": .65, "autobend_enabled": 1.0, "autobend_time": .065,
                    "autobend_depth": 4.5, "autobend_target": 2.0,
                    "space_mix": .035, "space_size": .18, "output": -4.03},
         "notes": [(0, 1.5, 45, 72), (2, 1.5, 45, 100), (4, 1.5, 45, 118),
                   (6, 1.3, 53, 94), (6.75, 1.3, 50, 86), (7.5, 1.3, 45, 106),
                   (8.25, 1.3, 41, 98), (10, .8, 53, 96), (10.5, .8, 53, 64),
                   (11, .8, 50, 104), (11.5, .8, 45, 88), (12, 1.5, 41, 110),
                   (15, 1.5, 57, 98), (18, 3, 45, 104), (24, .05, 45, 104),
                   (27, 1.5, 45, 104)]}
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
                "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:percussion:" + stem).hex,
                "name": "Review / Physical materials / " + sketch["name"] + " r1",
                "mono": sketch["mono"], "chord": False, "values": values}
        result[stem + ".nitridepreset"] = (json.dumps(data, indent=2) + "\n").encode()
        result[stem + ".mid"] = base.encoder.performance(sketch["name"] + " r1 / original kick-tom study", sketch["bpm"], sketch["notes"])
        manifest.append({"file": stem, "category": "Physical materials", "study_axis": sketch["axis"],
                         "intent": sketch["intent"], "seconds": sketch["seconds"]})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": manifest}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Percussion files already exist; use --verify, never replace frozen presets/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen percussion differs from generator: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
