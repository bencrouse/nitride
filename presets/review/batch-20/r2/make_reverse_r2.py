#!/usr/bin/env python3
"""Distinct reverse-instrument revisions; preserve r1 and its exact comparison MIDI."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
PARENT = HERE.parent / "r1"
spec = importlib.util.spec_from_file_location("reverse_r1", PARENT / "make_reverse_studies.py")
parent = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parent)


def sketches():
    return [
        {"family": "backspun-tines", "axis": "clean harmonic key with a dry late tine edge, no filter sweep",
         "brief": "The clean reference: hollow, almost sine-like electric-key body and a restrained late fourth-harmonic tine edge. No resonant whoosh, grit, vibrato or room. Sparse chords expose its stillness against the other revisions.",
         "values": {"coupling": .015, "fm_depth": .10, "fm_ratio": 4.0, "fm_operator_mix": .04,
                    "amp_attack": .80, "amp_decay": .04, "amp_release": .06,
                    "cutoff": 5000.0, "resonance": .08, "filter_key_tracking": .6, "filter_velocity": .2,
                    "fm_attack": .80, "fm_decay": .045, "fm_release": .04, "fm_envelope_amount": .65,
                    "output": -4.67}},
        {"family": "unstruck-bell", "axis": "thin high inharmonic glass ring with a small spatial trail",
         "brief": "A much higher, thinner, glass-like bell: the exposed 7.13-ratio primary contributes an inharmonic upper ring, not another midrange rounded key. Its index thins toward the endpoint while amplitude swells; a small ordinary room trail contrasts the dry instruments.",
         "values": {"coupling": .025, "fm_depth": 1.7, "fm_ratio": 7.13, "fm_operator_mix": .55,
                    "amp_attack": 1.30, "amp_decay": .06, "amp_release": .10,
                    "cutoff": 12500.0, "resonance": .08, "filter_key_tracking": .15, "filter_fm_depth": .3,
                    "fm_attack": .90, "fm_decay": .10, "fm_release": .08, "fm_envelope_amount": -.7,
                    "space_mix": .24, "space_size": .45, "output": -.22}},
        {"family": "spool-guitar", "axis": "low odd-partial woody twang with a closing nasal throat",
         "brief": "A low, woody/nasal string surrogate rather than another glassy pluck. Half-ratio shaped FM, a resonant closing Tone contour and a longer endpoint decay make a choked twang. The filter moves opposite to the old shared brightening recipe; not an acoustic guitar model.",
         "values": {"coupling": .07, "fm_depth": 2.3, "fm_ratio": .5, "fm_shape": 2.0,
                    "fm_shape_amount": .60, "fm_operator_mix": .24,
                    "amp_attack": .42, "amp_decay": .07, "amp_release": .05,
                    "cutoff": 3600.0, "resonance": .60, "filter_key_tracking": .8, "filter_velocity": .2,
                    "filter_attack": .36, "filter_decay": .10, "filter_amount": -1.2,
                    "fm_attack": .40, "fm_decay": .07, "fm_release": .04, "fm_envelope_amount": .70,
                    "fm_route_1_source": 2.0, "fm_route_1_target": 2.0, "fm_route_1_amount": .30,
                    "output": -.94}},
        {"family": "inward-tom", "axis": "short rising-pitch suction drum with a rough skin endpoint",
         "brief": "A compact suction drum, not a miniature pad: pitch rises from below the played note while a low body gathers a rougher skin-like endpoint. The negative Autobend is a note-triggered upward entrance; no pitch-LFO sweep, sampled shell or noise layer.",
         "values": {"coupling": .25, "stress": .07, "response": .02, "fm_depth": .25,
                    "fm_ratio": 1.18, "fm_shape": 2.0, "fm_shape_amount": .25, "fm_operator_mix": .16,
                    "amp_attack": .16, "amp_decay": .04, "amp_release": .03,
                    "cutoff": 1300.0, "resonance": .25, "filter_key_tracking": .3,
                    "fm_attack": .15, "fm_decay": .035, "fm_release": .025, "fm_envelope_amount": 1.1,
                    "autobend_enabled": 1.0, "autobend_time": .34, "autobend_depth": -24.0, "autobend_target": 2.0,
                    "output": -.09}},
        {"family": "suction-bass", "axis": "smooth near-sine sub swell without FM bite or a bright endpoint",
         "brief": "The smooth low-frequency counterweight: no FM index, operator tap, network coupling, pitch motion or Space. A modest closing low-pass contour and short endpoint form a round sub gulp, deliberately unlike the brighter, biting r1 bass.",
         "values": {"coupling": 0.0, "fm_depth": 0.0, "fm_ratio": 1.0, "fm_operator_mix": 0.0,
                    "amp_attack": .32, "amp_decay": .045, "amp_release": .04,
                    "cutoff": 700.0, "resonance": .10, "filter_key_tracking": 1.0,
                    "filter_attack": .28, "filter_decay": .06, "filter_amount": -1.5,
                    "output": 0.0}},
        {"family": "rewind-reed", "axis": "frayed unstable reed with network rasp and a contracting vowel",
         "brief": "The deliberately rough voice: stronger network interaction, noninteger FM, a contracting resonant vowel and a little free vibrato. Index falls as the amplitude rises instead of copying the brightening key envelope. Not a breath model or per-note random source.",
         "values": {"coupling": .64, "stress": .27, "response": .19, "fm_depth": 2.8,
                    "fm_ratio": 1.414, "fm_shape": 1.0, "fm_shape_amount": .9, "fm_operator_mix": .04,
                    "amp_attack": .60, "amp_decay": .055, "amp_release": .05,
                    "cutoff": 6000.0, "resonance": .50, "filter_key_tracking": .5, "filter_velocity": .2,
                    "filter_attack": .24, "filter_decay": .12, "filter_amount": -.8,
                    "fm_attack": .57, "fm_decay": .07, "fm_release": .04, "fm_envelope_amount": -1.3,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_rate": 5.1, "lfo_pitch_depth": .10,
                    "output": -.78}},
        {"family": "folded-strings", "axis": "soft detuned ensemble pull with a broad ordinary Space tail",
         "brief": "The soft, beating chordal voice: near-unison triangle-rich primary tap, gentle common drift, small relative-ratio wandering and a broader ordinary Space tail. No rising FM-index strike. The amplitude still turns over rather than sustaining; the room itself is not reversed or chorused.",
         "values": {"coupling": .02, "response": .04, "fm_depth": .10, "fm_ratio": 1.016,
                    "fm_shape": 1.0, "fm_shape_amount": .9, "fm_operator_mix": .65,
                    "amp_attack": 1.75, "amp_decay": .10, "amp_release": .16,
                    "cutoff": 4200.0, "resonance": .10, "filter_key_tracking": .35,
                    "filter_attack": 1.50, "filter_decay": .16, "filter_amount": -.5,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_rate": .28, "lfo_pitch_depth": .045,
                    "lfo_fm_enabled": 1.0, "lfo_fm_rate": .073, "lfo_fm_depth": 0.0,
                    "fm_route_1_source": 1.0, "fm_route_1_target": 1.0, "fm_route_1_amount": .002,
                    "space_mix": .30, "space_size": .60, "output": -1.19}},
        {"family": "vanishing-clav", "axis": "very short papery clav-rim flick from audio-rate Tone FM",
         "brief": "The brittle rhythmic opposite to the soft strings: a 110 ms papery clav/rim flick, a high-ratio late edge and audio-rate Tone FM rather than network grit. No slow resonant filter sweep or room. Repeat the same short note to compare its dry texture with the pitched suction drum.",
         "values": {"coupling": .015, "response": .02, "fm_depth": .40, "fm_ratio": 5.0,
                    "fm_shape": 2.0, "fm_shape_amount": .85, "fm_operator_mix": .08,
                    "amp_attack": .11, "amp_decay": .012, "amp_release": .02,
                    "cutoff": 2600.0, "resonance": .62, "filter_fm_depth": 2.1, "filter_key_tracking": .45,
                    "fm_attack": .10, "fm_decay": .018, "fm_release": .02, "fm_envelope_amount": 2.1,
                    "output": -2.83}}
    ]


def files():
    content, entries = {}, []
    originals = {s["family"]: s for s in parent.sketches()}
    for sketch in sketches():
        family = sketch["family"]; original = originals[family]
        values = parent.base.neutral()
        values.update({"amp_sustain": 0.0, "filter_sustain": 0.0, "fm_sustain": 0.0, "lfo_tempo": 100.0})
        if set(sketch["values"]) - set(values): raise ValueError("Unknown reverse-r2 parameter")
        values.update(sketch["values"])
        stem = family + "-r2"
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:reverse-instrument-study:" + stem).hex,
                  "name": "Review / " + original["category"] + " / " + original["name"] + " r2",
                  "mono": original["mono"], "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        content[stem + ".mid"] = (PARENT / (family + "-r1.mid")).read_bytes()
        entries.append({"file": stem, "category": original["category"], "host_category": original["host_category"],
                        "seconds": 27, "study_axis": sketch["axis"], "intent": sketch["brief"] +
                        " Original r1 MIDI retained for direct comparison. Synthesized backwards gesture, not literal audio reversal; "
                        "shared Tone and global free LFOs, not independent per-voice filters or drift."})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r2", "sketches": entries}, indent=2) + "\n").encode()
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((HERE / name).exists() for name in content):
        raise SystemExit("Studies exist; use --verify, never replace frozen revisions")
    for name, data in content.items():
        if args.verify:
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen reverse r2 differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
