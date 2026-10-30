#!/usr/bin/env python3
"""Original detuned-analog mood studies, not artist recipes; never overwrite frozen revisions."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("analog_study_base", HERE.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "low-battery-piano", "name": "Low Battery Piano", "category": "Physical materials", "host_category": "Keys & Organs", "mono": False,
         "anchor": "review:sugar-teeth:r1", "axis": "worn piano-like strike and near-unison ring", "lens": "Portishead",
         "bpm": 82, "seconds": 32, "tuned_ratio": 1.0,
         "brief": "Close, worn piano-like key: brief FM hammer edge, decaying body and audible near-unison primary tap at 1.003. The static relative offset supplies beating even with pitch wow disabled; shallow slow common pitch motion adds a tired transport impression. Sparse minor/ninth voicings, not a quoted song.",
         "exclude": "An FM piano interpretation, not acoustic piano samples, independently detuned strings, stretch tuning, a tape processor or an artist-authentic instrument. Velocity opens the shared Tone filter, not a separate filter per key.",
         "values": {"coupling": .025, "stress": 0.0, "response": .035, "fm_depth": .16,
                    "amp_attack": .002, "amp_decay": 1.25, "amp_sustain": .04, "amp_release": .28,
                    "cutoff": 4600.0, "resonance": .10, "filter_key_tracking": .70, "filter_velocity": 1.10,
                    "filter_attack": .001, "filter_decay": .08, "filter_sustain": 0.0, "filter_release": .16, "filter_amount": .80,
                    "fm_ratio": 1.003, "fm_shape": 2.0, "fm_shape_amount": .15, "fm_operator_mix": .28,
                    "fm_attack": .001, "fm_decay": .045, "fm_sustain": .02, "fm_release": .18, "fm_envelope_amount": 1.65,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_rate": .16, "lfo_pitch_depth": .045,
                    "space_mix": .09, "space_size": .32, "output": -2.10},
         "notes": [(0, 4, 60, 64), (5, 4, 60, 104), (11, 5, 48, 80), (11, 5, 55, 74), (11, 5, 59, 78), (11, 5, 64, 84),
                   (19, .5, 67, 88), (20, .6, 65, 76), (21, 2, 62, 92),
                   (25, 5, 53, 80), (25, 5, 60, 74), (25, 5, 64, 78), (25, 5, 69, 84), (34, 4, 72, 72)]},
        {"family": "slack-drum", "name": "Slack Drum", "category": "Physical materials", "host_category": "Drums & Percussion", "mono": False,
         "anchor": "review:floor-pulse:r1", "axis": "loose pitched drum with beating body", "lens": "Beak> / Portishead",
         "bpm": 88, "seconds": 27, "tuned_ratio": 1.0,
         "brief": "A loose tom-like electronic drum, rather than a polished kick: short common downward pitch entrance, near-unison 1.011 carrier/tap relationship and a decaying body long enough for the offset to matter. Held notes expose the ring; short low hits serve a slower groove.",
         "exclude": "Not a drum kit, sampled acoustic shell, independently tuned membrane modes or per-hit random pitch. The five-semitone attack drop is a separate transient gesture, not the persistent detuning. No pitch LFO is needed for the detuned body.",
         "values": {"coupling": .035, "stress": .01, "response": .025, "fm_depth": .12,
                    "amp_attack": .001, "amp_decay": .65, "amp_sustain": 0.0, "amp_release": .35,
                    "cutoff": 2200.0, "resonance": .28, "filter_key_tracking": .55, "filter_velocity": .50,
                    "filter_attack": .001, "filter_decay": .05, "filter_sustain": 0.0, "filter_release": .16, "filter_amount": 1.0,
                    "fm_ratio": 1.011, "fm_operator_mix": .32,
                    "fm_attack": .001, "fm_decay": .035, "fm_sustain": 0.0, "fm_release": .20, "fm_envelope_amount": 1.0,
                    "autobend_enabled": 1.0, "autobend_time": .028, "autobend_depth": 5.0, "autobend_target": 2.0,
                    "space_mix": .08, "space_size": .35, "output": 0.0},
         "notes": [(0, 4, 48, 88), (5, 4, 55, 88), (10, .35, 36, 104), (12, .35, 36, 96), (13.5, .5, 43, 72),
                   (16, 1, 48, 96), (18, .3, 36, 88), (19.5, .4, 43, 80), (22, 3, 48, 64), (28, 3, 48, 112), (33, 2, 60, 88)]},
        {"family": "brown-console", "name": "Brown Console", "category": "Leads", "host_category": "Keys & Organs", "mono": False,
         "anchor": "review:clear-stops:r1", "axis": "dark beating combo-organ surrogate", "lens": "Beak>",
         "bpm": 108, "seconds": 32, "tuned_ratio": 1.0,
         "brief": "A blunt, dark combo-organ surrogate for motorik repetitions and held clusters. A 0.997 primary tap beats below the carrier; tiny independent ratio wandering changes relative tuning while shallow common pitch motion and amplitude tremolo move the whole sound. Fast keying, little room.",
         "exclude": "Not tonewheel/crosstalk modeling, a Leslie, stereo chorus or separately drifting voices. The three motions are global; the note-dependent beat rates arise from the actual relative frequency offset.",
         "values": {"coupling": .045, "stress": 0.0, "response": .025, "fm_depth": .18,
                    "amp_attack": .006, "amp_decay": .4, "amp_sustain": .94, "amp_release": .11,
                    "cutoff": 2700.0, "resonance": .12, "filter_key_tracking": .30, "filter_velocity": .10,
                    "fm_ratio": .997, "fm_shape": 1.0, "fm_shape_amount": .38, "fm_operator_mix": .42,
                    "fm_attack": .001, "fm_decay": .035, "fm_sustain": 0.0, "fm_release": .10, "fm_envelope_amount": .10,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_rate": .19, "lfo_pitch_depth": .06,
                    "lfo_fm_enabled": 1.0, "lfo_fm_rate": .085, "lfo_fm_depth": 0.0,
                    "fm_route_1_source": 1.0, "fm_route_1_target": 1.0, "fm_route_1_amount": .0015,
                    "lfo_amplitude_enabled": 1.0, "lfo_amplitude_rate": 3.3, "lfo_amplitude_depth": .08,
                    "space_mix": .04, "space_size": .20, "output": -6.80},
         "notes": [(0, 10, 60, 88), (0, 10, 67, 80), (0, 10, 70, 84)]
                  + [(14 + i * .5, .30, (60, 67, 70, 65)[i % 4], 88) for i in range(16)]
                  + [(26, 10, 53, 84), (26, 10, 60, 80), (26, 10, 64, 88), (42, 6, 72, 88)]},
        {"family": "silt-bass", "name": "Silt Bass", "category": "Basses", "host_category": "Bass", "mono": True,
         "anchor": "review:felt-weight:r1", "axis": "dry round bass with shallow relative detune", "lens": "Portishead",
         "bpm": 84, "seconds": 29, "tuned_ratio": 1.0,
         "brief": "A dry round bass that stays a foundation rather than a wobble effect. A small 0.998 near-unison tap adds a slow uneven body; slow common drift is only a few cents. Short filter/FM strikes and restrained all-pitch legato glide support deliberately spaced, low-register phrases.",
         "exclude": "No added sub-oscillator, tape saturation, pitch sequencer or big modulated cutoff. The quiet tap is an existing FM branch, not a new analog oscillator. No Space to blur the low end.",
         "values": {"coupling": .09, "stress": .015, "response": .035, "fm_depth": .32,
                    "amp_attack": .008, "amp_decay": .45, "amp_sustain": .78, "amp_release": .16,
                    "cutoff": 700.0, "resonance": .25, "filter_key_tracking": .70, "filter_velocity": .35,
                    "filter_attack": .002, "filter_decay": .09, "filter_sustain": .12, "filter_release": .15, "filter_amount": 1.1,
                    "fm_ratio": .998, "fm_operator_mix": .12,
                    "fm_attack": .002, "fm_decay": .08, "fm_sustain": .05, "fm_release": .15, "fm_envelope_amount": .55,
                    "glide_enabled": 1.0, "glide_time": .065, "glide_target": 2.0, "glide_legato": 1.0, "legato": 1.0,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_rate": .09, "lfo_pitch_depth": .045, "output": -4.97},
         "notes": [(0, 9, 36, 88), (11, 1.2, 36, 96), (12, 1.4, 43, 84), (14, .4, 41, 80), (16, 4, 36, 96),
                   (22, 4, 41, 88), (28, .7, 36, 80), (29, 1.3, 43, 92), (30, 1.5, 46, 84), (33, 3, 48, 88)]},
        {"family": "bent-aerial", "name": "Bent Aerial", "category": "Leads", "host_category": "Leads & Brass", "mono": True,
         "anchor": "review:plastic-hero:r1", "axis": "narrow fuzzy lead with relative and common drift", "lens": "Beak>",
         "bpm": 106, "seconds": 29, "tuned_ratio": 1.0,
         "brief": "A narrow, slightly fuzzy lead with an off-center beating edge rather than bright pop gloss. A 1.0065 primary tap plus tiny slow ratio modulation changes the intervals inside the tone; a different slow common pitch lane supplies wow. Legato moves the phrase without restarting its envelopes.",
         "exclude": "Not a named vintage monosynth, saw oscillator bank, tape echo or independently drifting analog VCOs. No fast vibrato or large pitch scoop; strain comes from partials, not a blanket high-stress distortion setting.",
         "values": {"coupling": .12, "stress": .025, "response": .06, "fm_depth": .42,
                    "amp_attack": .018, "amp_decay": .45, "amp_sustain": .85, "amp_release": .22,
                    "cutoff": 1900.0, "resonance": .24, "filter_key_tracking": .50, "filter_velocity": .15,
                    "filter_attack": .055, "filter_decay": .35, "filter_sustain": .50, "filter_release": .20, "filter_amount": .70,
                    "fm_ratio": 1.0065, "fm_shape": 2.0, "fm_shape_amount": .22, "fm_operator_mix": .24,
                    "fm_attack": .045, "fm_decay": .60, "fm_sustain": .40, "fm_release": .20, "fm_envelope_amount": .35,
                    "glide_enabled": 1.0, "glide_time": .10, "glide_target": 2.0, "glide_legato": 1.0, "legato": 1.0,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_rate": .14, "lfo_pitch_depth": .09,
                    "lfo_fm_enabled": 1.0, "lfo_fm_rate": .21, "lfo_fm_depth": 0.0,
                    "fm_route_1_source": 1.0, "fm_route_1_target": 1.0, "fm_route_1_amount": .003,
                    "space_mix": .12, "space_size": .38, "output": -4.21},
         "notes": [(0, 6, 60, 88), (8, 1.3, 60, 88), (9, 1.3, 63, 96), (10, 2, 67, 88), (13, 1, 65, 80),
                   (16, 7, 72, 92), (26, 1.4, 67, 88), (27, 1.3, 65, 84), (28, 2, 60, 92), (34, 8, 55, 88)]},
        {"family": "melted-ensemble", "name": "Melted Ensemble", "category": "Pads", "host_category": "Pads", "mono": False,
         "anchor": "review:budget-heaven:r1", "axis": "dark detuned ensemble without a chorus engine", "lens": "Portishead / Beak>",
         "bpm": 80, "seconds": 40, "tuned_ratio": 1.0,
         "brief": "A dark, sagging ensemble at the edge of Nitride's analog illusion. Triangle-rich 1.0035 near-unison tap, slower relative-ratio wandering and a separate shallow common pitch drift produce moving beat rates. A gentle entrance, sustained chords and restrained Space leave the artificial ensemble exposed.",
         "exclude": "Unlike Budget Heaven, this is dark, slowly moving and deliberately less glossy. No chorus, stereo VCO spread, independently phased drift per voice, giant ambient wash or new oscillator model is implied.",
         "values": {"coupling": .04, "stress": 0.0, "response": .035, "fm_depth": .28,
                    "amp_attack": .65, "amp_decay": 2.0, "amp_sustain": .90, "amp_release": 1.5,
                    "cutoff": 2400.0, "resonance": .10, "filter_key_tracking": .30, "filter_velocity": .20,
                    "fm_ratio": 1.0035, "fm_shape": 1.0, "fm_shape_amount": .65, "fm_operator_mix": .45,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_rate": .11, "lfo_pitch_depth": .055,
                    "lfo_fm_enabled": 1.0, "lfo_fm_wave": 1.0, "lfo_fm_rate": .071, "lfo_fm_depth": 0.0,
                    "fm_route_1_source": 1.0, "fm_route_1_target": 1.0, "fm_route_1_amount": .004,
                    "space_mix": .18, "space_size": .48, "output": -7.11},
         "notes": [(0, 10, 48, 80), (0, 10, 55, 76), (0, 10, 59, 80), (0, 10, 64, 84),
                   (15, 10, 53, 80), (15, 10, 60, 76), (15, 10, 64, 80), (15, 10, 69, 84),
                   (30, 10, 55, 80), (30, 10, 62, 76), (30, 10, 65, 80), (30, 10, 71, 84), (44, 3, 60, 88)]},
        {"family": "oxide-rim", "name": "Oxide Rim", "category": "Physical materials", "host_category": "Drums & Percussion", "mono": False,
         "anchor": "review:skin-knock:r1", "axis": "short inharmonic rim-like edge", "lens": "Beak>",
         "bpm": 108, "seconds": 22, "tuned_ratio": 2.0,
         "brief": "A dry papery rim-like clack to contrast Slack Drum's loose low body. A slightly displaced 2.013 ratio and very short shaped-FM strike give the edge an uneasy upper partial; moderate network excitation roughens the surface without turning the whole batch into the same grit.",
         "exclude": "Not a snare-wire/noise source, sampled rim, tape clipping or a kit mapping. The short sound emphasizes inharmonic attack color rather than claiming a slow audible unison beat in every hit.",
         "values": {"coupling": .10, "stress": .08, "response": .025, "fm_depth": .30,
                    "amp_attack": .001, "amp_decay": .13, "amp_sustain": 0.0, "amp_release": .10,
                    "cutoff": 3300.0, "resonance": .45, "filter_key_tracking": .40, "filter_velocity": .60, "filter_fm_depth": .60,
                    "filter_attack": .001, "filter_decay": .04, "filter_sustain": 0.0, "filter_release": .10, "filter_amount": 1.0,
                    "fm_ratio": 2.013, "fm_shape": 2.0, "fm_shape_amount": .65, "fm_operator_mix": .04,
                    "fm_attack": .001, "fm_decay": .014, "fm_sustain": 0.0, "fm_release": .08, "fm_envelope_amount": 2.2,
                    "space_mix": .06, "space_size": .22, "output": -1.34},
         "notes": [(i * .75, .12, (60, 60, 67, 60)[i % 4], (64, 96, 80, 104)[i % 4]) for i in range(16)]
                  + [(16, 4, 60, 88), (22, .2, 48, 72), (24, .2, 72, 96), (28, .12, 60, 88), (30, .12, 60, 88), (32, .12, 60, 88)]},
        {"family": "reel-clav", "name": "Reel Clav", "category": "Physical materials", "host_category": "Plucks & Mallets", "mono": False,
         "anchor": "review:rosewood-tap:r1", "axis": "damped key with held-cycle pitch wander", "lens": "Portishead",
         "bpm": 90, "seconds": 27, "tuned_ratio": 2.0,
         "brief": "A damped, woody electric-key/clav surrogate for repeating small figures. Slight 2.003 inharmonicity colors the strike; very shallow free sample-and-hold common pitch drift changes the played phrase over time. Keep it dry enough that rhythmic gaps remain gaps.",
         "exclude": "Not sampled clavinet, random tuning per key or actual tape wow/flutter processing. The sample-and-hold lane is global and deterministic by cycle; short repeated notes can share a held pitch offset.",
         "values": {"coupling": .06, "stress": .005, "response": .03, "fm_depth": .50,
                    "amp_attack": .002, "amp_decay": .28, "amp_sustain": .02, "amp_release": .12,
                    "cutoff": 3000.0, "resonance": .23, "filter_key_tracking": .65, "filter_velocity": .80,
                    "filter_attack": .001, "filter_decay": .06, "filter_sustain": .05, "filter_release": .12, "filter_amount": .65,
                    "fm_ratio": 2.003, "fm_operator_mix": .05,
                    "fm_attack": .001, "fm_decay": .055, "fm_sustain": 0.0, "fm_release": .10, "fm_envelope_amount": .80,
                    "lfo_pitch_enabled": 1.0, "lfo_pitch_wave": 4.0, "lfo_pitch_rate": .37, "lfo_pitch_depth": .035,
                    "space_mix": .03, "space_size": .18, "output": -3.87},
         "notes": [(i * .75, .22, 60, 88) for i in range(12)]
                  + [(12 + i * .5, .20, (60, 67, 70, 65)[i % 4], (88, 72, 96, 80)[i % 4]) for i in range(16)]
                  + [(24, 4, 60, 88), (24, 4, 67, 80), (31, .3, 48, 80), (33, .3, 72, 88)]}
    ]


def files():
    content, entries = {}, []
    for sketch in sketches():
        values = base.neutral()
        if set(sketch["values"]) - set(values): raise ValueError("Unknown analog-study parameter")
        values.update(sketch["values"])
        values["lfo_tempo"] = float(sketch["bpm"])
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:analog-mood-study:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                  "mono": sketch["mono"], "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        content[stem + ".mid"] = base.encoder.performance(sketch["name"] + " / original detuned analog mood", sketch["bpm"], sketch["notes"])
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"],
                        "seconds": sketch["seconds"], "study_axis": sketch["axis"],
                        "design_lens": sketch["lens"] + " (imagined mood brief, not artist-authentic)",
                        "intent": sketch["brief"] + " " + sketch["exclude"]})
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
            if (HERE / name).read_bytes() != data: raise SystemExit("Frozen analog study differs: " + name)
        else:
            with (HERE / name).open("xb") as stream: stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
