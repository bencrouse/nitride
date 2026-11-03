#!/usr/bin/env python3
"""Playable reverse-envelope instruments, not audio reversal; never overwrite frozen revisions."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("reverse_study_base", HERE.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "backspun-tines", "name": "Backspun Tines", "category": "Physical materials", "host_category": "Keys & Organs", "mono": False,
         "anchor": "review:classic-tines:r1", "axis": "electric-key body swelling into its hammer edge",
         "attack": .90, "brief": "A rounded electric-key ring gathers its hard tine edge late, then closes instead of ringing out. Sparse minor voicings and isolated keys; not a piano sample or a generic slow pad.",
         "values": {"coupling": .025, "fm_depth": .08, "fm_ratio": 1.0, "fm_operator_mix": .08,
                    "cutoff": 1800.0, "filter_amount": 1.4, "filter_velocity": .45, "filter_key_tracking": .7,
                    "fm_envelope_amount": 2.2, "fm_attack": .78, "fm_shape": 2.0, "fm_shape_amount": .10,
                    "output": -3.31}, "note": 60, "chord": [53, 60, 64, 69]},
        {"family": "unstruck-bell", "name": "Unstruck Bell", "category": "Physical materials", "host_category": "Plucks & Mallets", "mono": False,
         "anchor": "review:hollow-bronze:r2", "axis": "inharmonic ring pulled into a late metallic strike",
         "attack": 1.60, "brief": "An exposed bell ring sucks inward toward an increasingly metallic endpoint. The inharmonic primary distinguishes it from the harmonic tines; hold long enough to hear the terminal strike.",
         "values": {"coupling": .04, "fm_depth": .30, "fm_ratio": 2.756, "fm_operator_mix": .12,
                    "cutoff": 2400.0, "filter_amount": 1.6, "filter_key_tracking": .65,
                    "fm_envelope_amount": 2.8, "fm_attack": 1.52, "space_mix": .025, "space_size": .15,
                    "output": -2.73}, "note": 60, "chord": [55, 62]},
        {"family": "spool-guitar", "name": "Spool Guitar", "category": "Physical materials", "host_category": "Plucks & Mallets", "mono": False,
         "anchor": "review:rosewood-tap:r1", "axis": "woody string body arriving before its pick",
         "attack": .48, "brief": "A dry, woody plucked-string surrogate with the order inverted: rounded body first, wiry pick edge last. Detached dyads and small figures, not a guitar recording or reversed delay.",
         "values": {"coupling": .055, "fm_depth": .15, "fm_ratio": 3.0, "fm_operator_mix": .05,
                    "cutoff": 1200.0, "resonance": .20, "filter_amount": 1.8, "filter_velocity": .45,
                    "fm_envelope_amount": 1.5, "fm_attack": .43, "fm_shape": 1.0, "fm_shape_amount": .18,
                    "output": -3.76}, "note": 55, "chord": [48, 55, 62]},
        {"family": "inward-tom", "name": "Inward Tom", "category": "Physical materials", "host_category": "Drums & Percussion", "mono": False,
         "anchor": "review:bowl-toms:r2", "axis": "low drum resonance swelling into a skin-like edge",
         "attack": .36, "brief": "A low pitched drum body rises into a compact skin-like FM edge and drops away. Hold the key beyond the swell: the short decay ends the hit without a sequencer. No noise source, drum sample or pitch-LFO riser.",
         "values": {"coupling": .06, "stress": .015, "fm_depth": .06, "fm_ratio": 1.041,
                    "fm_operator_mix": .18, "cutoff": 500.0, "resonance": .35, "filter_amount": 2.1,
                    "filter_key_tracking": .65, "fm_envelope_amount": 1.7, "fm_attack": .32,
                    "output": -.36}, "note": 43, "chord": None},
        {"family": "suction-bass", "name": "Suction Bass", "category": "Basses", "host_category": "Bass", "mono": True,
         "anchor": "review:classic-bass:r1", "axis": "reverse finger-bass gulp with a grounded root",
         "attack": .42, "brief": "A rounded low root gathers a resonant finger-like edge, then cuts short. A compact backwards bass gesture rather than a long sweep, wobble or gliding pitch effect. Play detached roots.",
         "values": {"coupling": .08, "fm_depth": .12, "fm_ratio": 1.0, "cutoff": 180.0,
                    "resonance": .40, "filter_amount": 2.5, "filter_key_tracking": .8,
                    "fm_envelope_amount": .95, "fm_attack": .37, "fm_shape": 1.0, "fm_shape_amount": .3,
                    "output": -1.46}, "note": 36, "chord": None},
        {"family": "rewind-reed", "name": "Rewind Reed", "category": "Leads", "host_category": "Leads & Brass", "mono": True,
         "anchor": "review:reed-snap:r2", "axis": "hollow wind tone opening into a late reed bite",
         "attack": .68, "brief": "A hollow sustained-wind impression turns inside out: the reed bite appears at the end of the swell, with no sustained plateau. Detached mid-register phrases; not a breath/noise model or ordinary brass attack.",
         "values": {"coupling": .12, "response": .05, "fm_depth": .28, "fm_ratio": 1.0,
                    "cutoff": 900.0, "resonance": .28, "filter_amount": 2.0, "filter_key_tracking": .6,
                    "fm_envelope_amount": 1.9, "fm_attack": .61, "fm_shape": 1.0, "fm_shape_amount": .65,
                    "output": -.30}, "note": 60, "chord": None},
        {"family": "folded-strings", "name": "Folded Strings", "category": "Pads", "host_category": "Pads", "mono": False,
         "anchor": "review:melted-ensemble:r2", "axis": "ensemble chord swelling toward a dry bow-like endpoint",
         "attack": 1.85, "brief": "A string-ensemble surrogate rises as a chord, acquires bow-like upper partials and ends decisively instead of sustaining as a pad. Longer inverse gesture, no chorus, independent string layers or reverse reverb.",
         "values": {"coupling": .045, "fm_depth": .20, "fm_ratio": 2.006, "fm_operator_mix": .25,
                    "fm_shape": 1.0, "fm_shape_amount": .65, "cutoff": 1500.0, "filter_amount": 1.8,
                    "filter_key_tracking": .35, "fm_envelope_amount": 1.7, "fm_attack": 1.70,
                    "output": -3.78}, "note": 60, "chord": [48, 55, 59, 64]},
        {"family": "vanishing-clav", "name": "Vanishing Clav", "category": "Physical materials", "host_category": "Keys & Organs", "mono": False,
         "anchor": "review:reel-clav:r2", "axis": "short backwards electric-key flick",
         "attack": .20, "brief": "A very short dry electric-key flick: woody body into a late, bright clav-like edge, then disappearance. The quickest backwards gesture in the set, suitable for repeated rhythmic notes without a free-running gate.",
         "values": {"coupling": .035, "fm_depth": .12, "fm_ratio": 2.0, "fm_operator_mix": .10,
                    "cutoff": 1100.0, "resonance": .22, "filter_amount": 2.0, "filter_velocity": .65,
                    "fm_envelope_amount": 1.65, "fm_attack": .18,
                    "output": -4.20}, "note": 60, "chord": [60, 67, 70]}
    ]


def performance(sketch):
    # Times are in seconds here so each note can reach its own envelope endpoint.
    # The second note deliberately releases early; the third holds past the swell.
    a, note = sketch["attack"], sketch["note"]
    notes = [(0, a + .12, note, 88), (3, a * .30, note, 88), (5, a + .8, note, 104)]
    chord = sketch["chord"] or [note - 5]
    notes += [(9, a + .15, n, 80 + i * 3) for i, n in enumerate(chord)]
    notes += [(13, a + .15, n + 2, 88) for n in chord]
    notes += [(18 + i * (a + .35), a + .08, note + step, 80 + i * 6) for i, step in enumerate((0, 7, 3))]
    return [(start / .6, duration / .6, n, velocity) for start, duration, n, velocity in notes]


def files():
    content, entries = {}, []
    for sketch in sketches():
        a = sketch["attack"]
        values = base.neutral()
        values.update({"amp_attack": a, "amp_decay": .025, "amp_sustain": 0.0, "amp_release": .025,
                       "filter_attack": a * .95, "filter_decay": .04, "filter_sustain": 0.0, "filter_release": .025,
                       "fm_decay": .035, "fm_sustain": 0.0, "fm_release": .025, "lfo_tempo": 100.0})
        if set(sketch["values"]) - set(values): raise ValueError("Unknown reverse-study parameter")
        values.update(sketch["values"])
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:reverse-instrument-study:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                  "mono": sketch["mono"], "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        content[stem + ".mid"] = base.encoder.performance(sketch["name"] + " / original backwards-instrument gesture", 100, performance(sketch))
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"],
                        "seconds": 27, "study_axis": sketch["axis"],
                        "intent": sketch["brief"] + " Synthesized envelope illusion, not literal audio reversal. "
                                  "Hold for at least " + str(a) + " seconds; early release aborts the swell. "
                                  "Amplitude decay is exponential, not a hard gate. Shared Tone envelope; no note-reset LFO or automatic future note prediction."})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": entries}, indent=2) + "\n").encode()
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
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen reverse study differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
