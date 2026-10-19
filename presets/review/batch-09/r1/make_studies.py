#!/usr/bin/env python3
"""Original frontier recipes/MIDI; --verify checks bytes and creation never overwrites."""
import argparse
import importlib.util
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_review_midi", ROOT.parents[1] / "batch-02/r1/make_midi.py")
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)


def neutral():
    values = {
        "coupling": 0.0, "stress": 0.0, "response": .025, "fm_depth": 0.0, "pitch": 0.0,
        "amp_attack": .008, "amp_decay": .5, "amp_sustain": .7, "amp_release": .15,
        "cutoff": 4000.0, "resonance": .1, "space_mix": 0.0, "space_size": .2, "output": -6.0,
        "glide_enabled": 0.0, "glide_time": .025, "glide_target": 2.0, "glide_legato": 1.0, "glide_curve": 0.0,
        "autobend_enabled": 0.0, "autobend_time": .025, "autobend_depth": 0.0, "autobend_target": 2.0, "autobend_phrase": 0.0,
        "legato": 0.0, "filter_attack": .01, "filter_decay": .3, "filter_sustain": .5, "filter_release": .2, "filter_amount": 0.0,
        "lfo_tempo": 120.0, "filter_key_tracking": .5, "filter_velocity": 0.0, "filter_fm_depth": 0.0,
        "fm_ratio": 2.0, "fm_shape": 0.0, "fm_shape_amount": 0.0, "fm_operator_mix": 0.0,
        "fm_attack": .01, "fm_decay": .3, "fm_sustain": .5, "fm_release": .2, "fm_envelope_amount": 0.0
    }
    for lane in ("coupling", "pitch", "fm", "cutoff", "amplitude", "stress"):
        for field, value in (("enabled", 0.0), ("wave", 0.0), ("sync", 0.0), ("division", 9.0), ("rate", .25), ("depth", 0.0)):
            values[f"lfo_{lane}_{field}"] = value
    for n in (1, 2, 3):
        for field in ("source", "target", "amount"):
            values[f"fm_route_{n}_{field}"] = 0.0
    return values


def studies():
    return [
        {"family": "sub-canvas", "name": "Sub Canvas", "category": "Basses", "mono": True,
         "axis": "clean foundation", "seconds": 18, "bpm": 100,
         "intent": "Bare sustained foundation: zero coupling/stress/oscillator FM, no routes/motion/Space. Near-sine impression, not perfect sine after common soft ceiling/Tone. Contrast Felt Weight's damped pluck.",
         "values": {"amp_attack": .012, "amp_decay": .3, "amp_sustain": .85, "amp_release": .11,
                    "cutoff": 1800.0, "resonance": .05, "filter_key_tracking": 1.0, "fm_ratio": 1.0, "output": -6.85},
         "notes": [(0, 2, 36, 72), (3, 2, 36, 104), (6, .5, 43, 86), (7, .5, 41, 94),
                   (9, 4, 36, 96), (15, 2, 48, 84), (19, 2, 31, 100)]},
        {"family": "afterimage-keys", "name": "Afterimage Keys", "category": "Leads", "mono": False,
         "axis": "two-envelope harmonic key", "seconds": 22, "bpm": 100,
         "intent": "Harmonic key with short independent FM strike and delayed filter-envelope-to-depth bloom, neutral Tone-envelope amount. Hold through second stage; not a sampled piano or layer engine. Contrast Reed Snap articulation.",
         "values": {"coupling": .12, "response": .08, "fm_depth": .18, "amp_attack": .005,
                    "amp_decay": .9, "amp_sustain": .35, "amp_release": .25, "cutoff": 4200.0,
                    "filter_key_tracking": .6, "filter_velocity": .4, "space_mix": .1, "space_size": .28,
                    "filter_attack": .45, "filter_decay": .65, "filter_sustain": .45, "filter_release": .22,
                    "fm_operator_mix": .02, "fm_attack": .002, "fm_decay": .09, "fm_sustain": .02,
                    "fm_release": .12, "fm_envelope_amount": 1.1,
                    "fm_route_1_source": 3.0, "fm_route_1_target": 0.0, "fm_route_1_amount": .75, "output": -6.1},
         "notes": [(0, .5, 60, 76), (1, .5, 64, 94), (3, 3, 67, 88), (8, 3, 60, 94),
                   (8, 3, 67, 76), (13, 1.5, 65, 82), (17, 3, 62, 92), (23, 2, 72, 88)]},
        {"family": "overtone-lantern", "name": "Overtone Lantern", "category": "Experimental", "mono": False,
         "axis": "operator-dominant partial voice", "seconds": 24, "bpm": 100,
         "intent": "Exposed fifth-ratio triangle operator dominates carrier, giving a higher implied pitch/partial identity. Sparse notes/dyads, no pitch-step LFO. Not a new oscillator or proof of novel territory; contrast Orbit Shards.",
         "values": {"coupling": .13, "stress": .015, "response": .12, "fm_depth": .22,
                    "amp_attack": .018, "amp_decay": 1.2, "amp_sustain": .42, "amp_release": .55,
                    "cutoff": 12000.0, "resonance": .08, "filter_key_tracking": .4, "filter_velocity": .15,
                    "fm_ratio": 5.0, "fm_shape": 1.0, "fm_shape_amount": .7, "fm_operator_mix": .86,
                    "fm_attack": .02, "fm_decay": .18, "fm_sustain": .1, "fm_release": .4, "fm_envelope_amount": .3,
                    "space_mix": .26, "space_size": .58, "output": -1.75},
         "notes": [(0, 3, 48, 86), (5, 3, 55, 94), (10, 4, 60, 84), (10, 4, 67, 70),
                   (17, .5, 63, 78), (18, .5, 65, 96), (22, 4, 51, 88)]},
        {"family": "buzz-loom", "name": "Buzz Loom", "category": "Sequence elements", "mono": True,
         "axis": "fast AM and ramp-reset timbre", "seconds": 22, "bpm": 120,
         "intent": "Fast unipolar AM at 1/64 triplet (48 cycles/s at 120 BPM) against independent .25 Hz saw Tone motion. Hold low/mid notes with transport; sidebands are clock-relative, not key-tracked ring modulation or MIDI sequencing. Contrast Socket Dance chops.",
         "values": {"coupling": .18, "stress": .02, "response": .06, "fm_depth": .4,
                    "amp_attack": .035, "amp_decay": .5, "amp_sustain": .75, "amp_release": .16,
                    "cutoff": 2200.0, "resonance": .2, "filter_velocity": .3, "filter_amount": .35,
                    "fm_attack": .008, "fm_decay": .3, "fm_sustain": .5, "fm_release": .25, "fm_envelope_amount": .15,
                    "lfo_amplitude_enabled": 1.0, "lfo_amplitude_sync": 1.0,
                    "lfo_amplitude_division": 23.0, "lfo_amplitude_depth": .95,
                    "lfo_cutoff_enabled": 1.0, "lfo_cutoff_wave": 2.0, "lfo_cutoff_sync": 0.0,
                    "lfo_cutoff_division": 1.0, "lfo_cutoff_depth": .45,
                    "space_mix": .04, "output": -3.0},
         "notes": [(0, 6, 48, 96), (8, 6, 55, 88), (16, 6, 60, 102),
                   (24, .5, 48, 86), (25, .5, 55, 100), (28, 4, 51, 94)]},
        {"family": "cutoff-alloy", "name": "Cutoff Alloy", "category": "Physical materials", "mono": False,
         "axis": "filter-side FM texture", "seconds": 20, "bpm": 100,
         "intent": "Tuned struck texture centered on audio-rate cutoff FM depth 2.5 with small oscillator FM, rather than high network stress. Velocity/dyads affect shared filter drive. Not a noise source or new filter type; contrast ringing Hollow Bronze.",
         "values": {"coupling": .17, "stress": .01, "response": .035, "fm_depth": .08,
                    "amp_attack": .0016, "amp_decay": .7, "amp_sustain": 0.0, "amp_release": .3,
                    "cutoff": 1800.0, "resonance": .5, "filter_attack": .001, "filter_decay": .065,
                    "filter_sustain": .1, "filter_amount": 1.4, "filter_velocity": .45, "filter_fm_depth": 2.5,
                    "fm_operator_mix": .03, "space_mix": .08, "space_size": .3, "output": -3.9},
         "notes": [(0, .6, 48, 64), (2, .6, 48, 96), (4, .6, 48, 112), (7, .5, 60, 84),
                   (9, .5, 67, 102), (12, 2, 55, 96), (12, 2, 62, 78), (18, 2, 43, 106)]},
        {"family": "brake-bloom", "name": "Brake Bloom", "category": "Experimental", "mono": True,
         "axis": "self-braking activity route", "seconds": 26, "bpm": 100,
         "intent": "Negative network-activity-to-FM-depth restrains index as measured activity grows; modulator-only legato glide smears overtones while carrier takes the played note. Not a loudness limiter, new feedback topology or guarantee of autonomous pulsing. Contrast Tendon Drift.",
         "values": {"coupling": .62, "stress": .22, "response": .42, "fm_depth": 1.35,
                    "amp_attack": .055, "amp_decay": .8, "amp_sustain": .8, "amp_release": .65,
                    "cutoff": 3400.0, "resonance": .15, "filter_amount": .5, "filter_key_tracking": .6,
                    "filter_velocity": .3, "fm_ratio": 1.0, "fm_shape": 1.0, "fm_shape_amount": .3,
                    "fm_operator_mix": .02, "fm_route_1_source": 4.0, "fm_route_1_target": 0.0, "fm_route_1_amount": -2.4,
                    "glide_enabled": 1.0, "glide_time": .16, "glide_target": 1.0, "glide_curve": 2.0,
                    "legato": 1.0, "space_mix": .14, "space_size": .6, "output": -4.4},
         "notes": [(0, 4, 48, 60), (6, 4, 48, 110), (12, 5, 55, 88), (16, 4, 60, 96),
                   (19, 4, 51, 82), (27, 5, 48, 100)]}
    ]


def files():
    result, sketches = {}, []
    expected = set(json.loads((ROOT.parents[1] / "batch-08/r1/replica-voice-r1.nitridepreset").read_text())["values"])
    for study in studies():
        stem = study["family"] + "-r1"
        values = neutral()
        assert not set(study["values"]) - expected
        values.update(study["values"])
        values["lfo_tempo"] = float(study["bpm"])
        assert set(values) == expected
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:frontier:" + stem).hex,
                  "name": "Review / " + study["category"] + " / " + study["name"] + " r1",
                  "mono": study["mono"], "chord": False, "values": values}
        result[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        result[stem + ".mid"] = encoder.performance(study["name"] + " r1 / original frontier study", study["bpm"], study["notes"])
        sketches.append({"file": stem, "category": study["category"], "study_axis": study["axis"],
                         "intent": study["intent"], "seconds": study["seconds"]})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": sketches}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Study files already exist; use --verify, never replace frozen presets/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen study differs from generator: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
