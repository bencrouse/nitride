#!/usr/bin/env python3
"""Three original, explicitly partial FS1R studies. Never overwrite; use --verify."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("fs1r_nitride_neutral", HERE.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec); spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "whistle-stem", "name": "Whistle Stem", "category": "Leads", "source": "kuchibue", "bank": "Pre A", "number": 70,
         "anchor": "review:reed-snap:r2", "anchor_note_offset": 12, "reference_note_offset": 0,
         "axis": "FS1R Kuchibue dominant sine-branch interpretation", "bpm": 100, "seconds": 24,
         "intent": "Dry soft whistle: source2xcarrier register retained with Pitch+12, near-unison sine operator tap substitutes for independently detuned2:1FM branches, mild immediate free vibrato instead of source delayed per-op modulation. Source formant/noise removals barely affect this neutral-part probe, so main tonal role is plausibly transferable. No exact multi-operator envelopes/key scaling/pitchEG/phase/noise claim. Unchanged Reed anchor MIDI+12 aligns settled root.",
         "values": {"coupling": .015, "stress": 0.0, "response": .025, "fm_depth": .10, "pitch": 12.0,
                    "amp_attack": .11, "amp_decay": .8, "amp_sustain": .90, "amp_release": .15,
                    "cutoff": 4500.0, "resonance": .04, "filter_key_tracking": .85, "filter_velocity": .18,
                    "filter_amount": 0.0, "fm_ratio": 1.003, "fm_shape": 0.0, "fm_operator_mix": .45,
                    "fm_attack": .003, "fm_decay": .035, "fm_sustain": 0.0, "fm_release": .12, "fm_envelope_amount": .20,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_wave": 1.0, "lfo_pitch_rate": 5.8, "lfo_pitch_depth": .06,
                    "output": -4.31},
         "notes": [(0, 2, 60, 64), (5, 3, 60, 104), (12, .5, 67, 88), (13, .5, 69, 76), (14, 2, 72, 96),
                   (20, 4, 60, 88), (20, 4, 67, 76), (30, 3, 55, 92)]},
        {"family": "jaw-twang", "name": "Jaw Twang", "category": "Physical materials", "source": "mukkuri", "bank": "Pre B", "number": 5,
         "anchor": "review:taffy-coil:r1", "anchor_note_offset": -12, "reference_note_offset": 0,
         "axis": "FS1R Mukkuri single-resonance architectural-ceiling study", "bpm": 100, "seconds": 26,
         "intent": "Provisional dry low jaw-harp role, NOT faithful Mukkuri: original carrier formants are essential and disappear in sine-only reference. Native Pitch-12 retains source carrier transpose; rich1:1FM through one fixed-register moving resonant Tone peak stands in for six independently enveloped fixed-formant carriers/noise. No real formant bank/window/FMsum bus, no noise generator, no pitch bend. Judge whether this mouth-colored non-bending articulation earns a job beside Taffy; reject if it is merely another generic pluck. Old Taffy MIDI-12 aligns settled root but keeps its opposed bend/Space.",
         "values": {"coupling": .02, "stress": 0.0, "response": .025, "fm_depth": 1.10, "pitch": -12.0,
                    "amp_attack": .004, "amp_decay": .60, "amp_sustain": 0.0, "amp_release": .35,
                    "cutoff": 480.0, "resonance": .72, "filter_key_tracking": 0.0, "filter_velocity": .20,
                    "filter_attack": .035, "filter_decay": .24, "filter_sustain": .12, "filter_release": .18, "filter_amount": 1.40,
                    "fm_ratio": 1.0, "fm_shape": 2.0, "fm_shape_amount": .75, "fm_operator_mix": .06,
                    "fm_attack": .001, "fm_decay": .09, "fm_sustain": .16, "fm_release": .16, "fm_envelope_amount": 1.40,
                    "output": -1.13},
         "notes": [(0, .9, 60, 48), (4, .9, 60, 80), (8, .9, 60, 112), (14, .5, 55, 88),
                   (15.5, .5, 60, 96), (17, .5, 62, 80), (20, 3, 60, 94), (26, 1.5, 48, 96),
                   (33, 2, 60, 90), (33, 2, 67, 76)]},
        {"family": "vowel-pick", "name": "Vowel Pick", "category": "Physical materials", "source": "fmt-pluck", "bank": "Pre B", "number": 66,
         "anchor": "review:classic-koto:r1", "anchor_note_offset": 0, "reference_note_offset": 0,
         "axis": "FS1R Fmt-Pluck partial vowel-key interpretation", "bpm": 110, "seconds": 26,
         "intent": "Provisional dry vowel-colored picked key with normal root: triangle-shaped2:1FM strike, slower damped body and two contours moving one resonant Tone/index color. Source odd2 carrier plus fixed-formant pairs, feedback5, independent EGs and unvoiced layers cannot be imported. Native triangle is NOT FS1R odd2, and one LP peak is NOT the multi-formant bank. No new DSP/noise/Fseq/algorithm or pitch-bend imitation. Judge identity/overlap beside unchanged Classic Koto; reject generic mallet outcomes, don't infer closeness from the name.",
         "values": {"coupling": .025, "stress": 0.0, "response": .025, "fm_depth": .40,
                    "amp_attack": .002, "amp_decay": 1.25, "amp_sustain": .015, "amp_release": .30,
                    "cutoff": 1000.0, "resonance": .63, "filter_key_tracking": .05, "filter_velocity": .65,
                    "filter_attack": .07, "filter_decay": .25, "filter_sustain": .40, "filter_release": .25, "filter_amount": .85,
                    "fm_ratio": 2.0, "fm_shape": 1.0, "fm_shape_amount": .70, "fm_operator_mix": .08,
                    "fm_attack": .001, "fm_decay": .075, "fm_sustain": .05, "fm_release": .22, "fm_envelope_amount": 2.0,
                    "fm_route_1_source": 3.0, "fm_route_1_target": 0.0, "fm_route_1_amount": .40,
                    "output": -5.35},
         "notes": [(0, 1.5, 60, 48), (4, 1.5, 60, 80), (8, 1.5, 60, 112), (12, .5, 60, 88), (12.75, .5, 64, 80),
                   (13.5, .5, 67, 98), (14.25, 1, 69, 90), (20, 5, 60, 94), (29, 3, 55, 90), (29, 3, 62, 76), (34, 2, 72, 96)]}
    ]


def files():
    result, manifest = {}, []
    for sketch in sketches():
        values = base.neutral(); assert not set(sketch["values"]) - set(values)
        values.update(sketch["values"]); values["lfo_tempo"] = float(sketch["bpm"])
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6, "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:fs1r-study:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1", "mono": False, "chord": False, "values": values}
        result[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        result[stem + ".mid"] = base.encoder.performance(sketch["name"] + " r1 / FS1R partial study", sketch["bpm"], sketch["notes"])
        manifest.append({"file": stem, "category": sketch["category"], "seconds": sketch["seconds"], "study_axis": sketch["axis"], "intent": sketch["intent"]})
    result["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": manifest}, indent=2) + "\n").encode()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--verify", action="store_true"); args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE / name).exists() for name in content): raise SystemExit("Studies exist; use --verify, never replace frozen files")
    for name, data in content.items():
        if args.verify:
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen FS1R study differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
