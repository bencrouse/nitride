#!/usr/bin/env python3
"""Original Nitride studies of requested ROM1A30/25/23; --verify, never overwrite."""
import argparse
import importlib.util
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_requested_rom_base", ROOT.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "classic-voice", "name": "Classic Voice", "category": "Pads", "mono": False,
         "reference": "voice1", "factory_slot_one_based": 30, "reference_note_offset": 0, "anchor_note_offset": 0,
         "anchor": "review:stillwater:r1", "axis": "DX7 ROM1A30 VOICE 1 approximation", "bpm": 100, "seconds": 34,
         "intent": "Dry poly choir/vowel approximation: near-unison FM/tap, soft amplitude/index entrance, fixed shared Tone color, mild free triangle vibrato/index flutter. Source algorithm7 has two carrier branches/5.1stack/detuning/nonflatpitchEG and delayed shared LFO/operatorAMS2; native has no true formant bank/parallel operators/note-triggered LFOdelay-reset. Software A explicitly partial for operatorAMS, not altered source data.",
         "values": {"coupling": .08, "stress": 0.0, "response": .12, "fm_depth": .45,
                    "amp_attack": .35, "amp_decay": 1.0, "amp_sustain": .86, "amp_release": .8,
                    "cutoff": 2400.0, "resonance": .28, "filter_key_tracking": .1, "filter_velocity": .2,
                    "filter_attack": .4, "filter_decay": 1.1, "filter_sustain": .55, "filter_release": .75,
                    "filter_amount": -.7, "fm_ratio": 1.01, "fm_shape": 1.0, "fm_shape_amount": .3,
                    "fm_operator_mix": .12, "fm_attack": .45, "fm_decay": .9, "fm_sustain": .45,
                    "fm_release": .7, "fm_envelope_amount": .6,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_wave": 1.0, "lfo_pitch_rate": 5.8065, "lfo_pitch_depth": .28,
                    "lfo_fm_enabled": 1.0, "lfo_fm_wave": 1.0, "lfo_fm_rate": 5.8065, "lfo_fm_depth": .025,
                    "output": -7.15},
         "notes": [(0, 4, 60, 80), (8, 5, 60, 88), (8, 5, 63, 78), (8, 5, 67, 74),
                   (18, 4, 64, 58), (24, 4, 64, 108), (32, 6, 53, 90), (32, 6, 60, 80), (32, 6, 65, 76)]},
        {"family": "orch-chime", "name": "Orch Chime", "category": "Physical materials", "mono": False,
         "reference": "orch-chime", "factory_slot_one_based": 25, "reference_note_offset": 0, "anchor_note_offset": -12,
         "anchor": "review:hollow-bronze:r2", "axis": "DX7 ROM1A25 ORCH-CHIME approximation", "bpm": 100, "seconds": 32,
         "intent": "Dry single-voice chime-to-held-body interpretation, nominal6.28modulation relative to the.5carrier branch, brief index strike plus delayed secondary index contour. NativePitch-12 preserves source low-branch keyboard root; old Hollow reference MIDI-12 explicitly aligns sound. Source also has independent slow1:1/octave orchestral branches/detuning, not recreated by two envelopes on one voice. Mild immediate triangle vibrato approximates source LFO, no true layering/algorithm5 import.",
         "values": {"coupling": .035, "stress": 0.0, "response": .04, "fm_depth": .18, "pitch": -12.0,
                    "amp_attack": .003, "amp_decay": .8, "amp_sustain": .35, "amp_release": .8,
                    "cutoff": 11500.0, "resonance": .05, "filter_key_tracking": .4, "filter_velocity": .4,
                    "filter_attack": .55, "filter_decay": .5, "filter_sustain": .8, "filter_release": .65,
                    "filter_amount": 0.0, "fm_ratio": 6.28, "fm_shape": 0.0,
                    "fm_attack": .001, "fm_decay": .18, "fm_sustain": 0.0, "fm_release": .6,
                    "fm_envelope_amount": 1.8, "fm_route_1_source": 3.0, "fm_route_1_target": 0.0, "fm_route_1_amount": .4,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_wave": 1.0, "lfo_pitch_rate": 4.9678, "lfo_pitch_depth": .0725,
                    "output": -3.35},
         "notes": [(0, 1.5, 60, 64), (6, 1.5, 60, 104), (12, 6, 60, 88),
                   (24, 2, 55, 96), (30, 3, 67, 96), (38, 3, 60, 90), (38, 3, 67, 76)]},
        {"family": "classic-koto", "name": "Classic Koto", "category": "Physical materials", "mono": False,
         "reference": "koto", "factory_slot_one_based": 23, "reference_note_offset": 0, "anchor_note_offset": 0,
         "anchor": "review:rosewood-tap:r1", "axis": "DX7 ROM1A23 KOTO approximation", "bpm": 110, "seconds": 26,
         "intent": "Dry bright string/nail pluck from4:1 sine FM, brief shape/index entrance and longer damped body. Source algorithm2 adds a serial3/4/1stack alongside its4:1pair, a smallpitchEG and delayed synced-note sine LFO. Native preserves main4:1job but no exact cascade/operator scaling; reduced immediate sine vibrato instead of unavailable note-triggered delay/reset. AMD15 has zero operatorAMS recipients in original; do not add global tremolo.",
         "values": {"coupling": .045, "stress": 0.0, "response": .025, "fm_depth": .1,
                    "amp_attack": .001, "amp_decay": .5, "amp_sustain": 0.0, "amp_release": .35,
                    "cutoff": 8500.0, "resonance": .06, "filter_key_tracking": .55, "filter_velocity": 1.2,
                    "filter_attack": .001, "filter_decay": .025, "filter_sustain": 0.0, "filter_release": .08,
                    "filter_amount": 0.0, "fm_ratio": 4.0, "fm_shape": 2.0, "fm_shape_amount": 0.0,
                    "fm_attack": .001, "fm_decay": .045, "fm_sustain": 0.0, "fm_release": .15,
                    "fm_envelope_amount": 2.3, "fm_route_1_source": 3.0, "fm_route_1_target": 2.0, "fm_route_1_amount": .5,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_wave": 0.0, "lfo_pitch_rate": 4.9678, "lfo_pitch_depth": .05,
                    "output": -2.45},
         "notes": [(0, 1.5, 60, 48), (3, 1.5, 60, 80), (6, 1.5, 60, 112),
                   (10, .5, 60, 88), (10.75, .5, 62, 80), (11.5, .5, 67, 98), (12.25, 1, 69, 90),
                   (15, .5, 67, 96), (15.75, .5, 62, 78), (16.5, 1.5, 60, 100),
                   (22, 2, 60, 94), (22, 2, 67, 76), (28, 4, 57, 96)]}
    ]


def files():
    result, manifest = {}, []
    for sketch in sketches():
        values = base.neutral(); assert not set(sketch["values"]) - set(values)
        values.update(sketch["values"]); values["lfo_tempo"] = float(sketch["bpm"])
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:factory-inspired:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                  "mono": sketch["mono"], "chord": False, "values": values}
        result[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        result[stem + ".mid"] = base.encoder.performance(sketch["name"] + " r1 / requested ROM1A audition", sketch["bpm"], sketch["notes"])
        manifest.append({"file": stem, "category": sketch["category"], "study_axis": sketch["axis"], "intent": sketch["intent"], "seconds": sketch["seconds"]})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": manifest}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--verify", action="store_true"); args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Requested classic files exist; use --verify, never replace frozen presets/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen requested classic differs: " + name)
        else:
            with path.open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
