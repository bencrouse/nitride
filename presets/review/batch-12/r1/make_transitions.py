#!/usr/bin/env python3
"""Original playable FX/transition recipes and MIDI; --verify checks bytes, never overwrite."""
import argparse
import importlib.util
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_transition_base", ROOT.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "pressure-lift", "name": "Pressure Lift", "category": "Experimental", "mono": True,
         "axis": "rising pressure transition", "lens": "SOPHIE", "seconds": 28, "bpm": 120,
         "intent": "Original tactile pressure-riser brief, not an artist recipe: hold a single note about 4s as pitch rises from two octaves below and amplitude/index/Tone open. Release into the next section; small Space may linger. Envelopes/Autobend in seconds, not bar-synced; no noise/reverse-audio or automatic handoff.",
         "values": {"coupling": .42, "stress": .1, "response": .3, "fm_depth": .15,
                    "amp_attack": 2.6, "amp_decay": .3, "amp_sustain": .9, "amp_release": .12,
                    "cutoff": 750.0, "resonance": .22, "filter_key_tracking": .25, "filter_velocity": .15,
                    "filter_attack": 3.2, "filter_decay": .1, "filter_sustain": 1.0,
                    "filter_release": .14, "filter_amount": 3.2, "fm_ratio": 1.414,
                    "fm_shape": 2.0, "fm_shape_amount": .25, "fm_operator_mix": .02,
                    "fm_attack": 3.2, "fm_decay": .1, "fm_sustain": 1.0, "fm_release": .15,
                    "fm_envelope_amount": 2.0, "fm_route_1_source": 3.0,
                    "fm_route_1_target": 2.0, "fm_route_1_amount": .35,
                    "autobend_enabled": 1.0, "autobend_time": 4.0,
                    "autobend_depth": -24.0, "autobend_target": 2.0,
                    "space_mix": .12, "space_size": .4, "output": -4.7},
         "notes": [(0, 8, 48, 96), (12, 8, 55, 96), (24, 2, 48, 96), (32, 8, 43, 108)]},
        {"family": "sinkhole", "name": "Sinkhole", "category": "Experimental", "mono": True,
         "axis": "falling impact and spacious tail", "lens": "Arca", "seconds": 32, "bpm": 120,
         "intent": "Original downward implosion brief, not an artist recipe: quick metallic entrance falls into low weight and a spacious tail. Trigger around MIDI43 and allow the release/room to clear. Pitch/Tone/FM gesture in seconds, not tempo-synced; gated synth envelope, no sample reversal, noise layer or one-shot switch.",
         "values": {"coupling": .5, "stress": .1, "response": .05, "fm_depth": .15, "pitch": -7.0,
                    "amp_attack": .002, "amp_decay": .8, "amp_sustain": 0.0, "amp_release": .7,
                    "cutoff": 2800.0, "resonance": .3, "filter_key_tracking": .3, "filter_velocity": .45,
                    "filter_attack": .001, "filter_decay": .4, "filter_sustain": 0.0,
                    "filter_release": .6, "filter_amount": 2.1, "filter_fm_depth": .3,
                    "fm_ratio": 2.71, "fm_shape": 2.0, "fm_shape_amount": .45,
                    "fm_attack": .001, "fm_decay": .18, "fm_sustain": 0.0, "fm_release": .45,
                    "fm_envelope_amount": 2.2, "autobend_enabled": 1.0, "autobend_time": 1.2,
                    "autobend_depth": 24.0, "autobend_target": 2.0,
                    "space_mix": .3, "space_size": .8, "output": -1.1},
         "notes": [(0, 2, 43, 100), (12, 4, 43, 100), (26, 1, 48, 94), (38, .15, 43, 100)]},
        {"family": "shred-relay", "name": "Shred Relay", "category": "Sequence elements", "mono": True,
         "axis": "held-note rhythmic disintegration", "lens": "Oneohtrix Point Never", "seconds": 26, "bpm": 120,
         "intent": "Original signal-breakup brief, not an artist recipe: a held pitched tone becomes more inharmonic while its spectrum closes inside a steady 1/16 sine-AM clock. Hold 3-4s with playing transport, then release for a dry handoff. Fixed clock, not accelerating stutter/generated MIDI; envelopes in seconds. Contrast static Socket/Buzz cells.",
         "values": {"coupling": .18, "stress": .02, "response": .05, "fm_depth": .2,
                    "amp_attack": .012, "amp_decay": .25, "amp_sustain": .8, "amp_release": .075,
                    "cutoff": 11000.0, "resonance": .12, "filter_key_tracking": .25, "filter_velocity": .1,
                    "filter_attack": 2.8, "filter_decay": .1, "filter_sustain": 1.0,
                    "filter_release": .1, "filter_amount": -2.0,
                    "fm_ratio": 1.12, "fm_shape": 1.0, "fm_shape_amount": .4, "fm_operator_mix": .08,
                    "fm_attack": 2.4, "fm_decay": .1, "fm_sustain": 1.0, "fm_release": .1,
                    "fm_envelope_amount": 2.1, "fm_route_1_source": 3.0,
                    "fm_route_1_target": 1.0, "fm_route_1_amount": .55,
                    "lfo_amplitude_enabled": 1.0, "lfo_amplitude_sync": 1.0,
                    "lfo_amplitude_wave": 0.0, "lfo_amplitude_division": 15.0, "lfo_amplitude_depth": .99,
                    "output": -2.0},
         "notes": [(0, 8, 48, 96), (12, 8, 55, 96), (24, 2, 48, 96), (32, 8, 43, 108)]}
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
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:transition:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                  "mono": sketch["mono"], "chord": False, "values": values}
        result[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        result[stem + ".mid"] = base.encoder.performance(sketch["name"] + " r1 / original transition", sketch["bpm"], sketch["notes"])
        manifest.append({"file": stem, "category": sketch["category"], "study_axis": sketch["axis"],
                         "design_lens": sketch["lens"], "intent": sketch["intent"], "seconds": sketch["seconds"]})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": manifest}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Transition files already exist; use --verify, never replace frozen presets/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen transition differs from generator: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
