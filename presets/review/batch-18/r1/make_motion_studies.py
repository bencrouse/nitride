#!/usr/bin/env python3
"""Eight original LFO/envelope auditions; deterministic creation, never overwrite frozen files."""
import argparse
import importlib.util
import json
from pathlib import Path
import uuid

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("motion_study_base", HERE.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def sketches():
    return [
        {"family": "late-glass", "name": "Late Glass", "category": "Leads", "host_category": "Keys & Organs", "mono": False,
         "anchor": "review:afterimage-keys:r1", "axis": "loudness before harmonic bloom", "bpm": 100, "seconds": 28,
         "brief": "A warm immediate key becomes glassy after the strike: slow independent FM attack, not a second oscillator layer or a delayed note. Sparse holds expose the bloom; shallow free cutoff motion animates the ring.",
         "exclude": "Unlike Afterimage Keys, no fast FM strike or shared-filter-envelope-to-index route. No pitch modulation, noise or tempo-retriggered LFO.",
         "values": {"coupling": .06, "response": .04, "fm_depth": .08,
                    "amp_attack": .004, "amp_decay": 1.8, "amp_sustain": .45, "amp_release": .70,
                    "cutoff": 7000.0, "resonance": .10, "filter_key_tracking": .45, "filter_velocity": .25,
                    "fm_ratio": 3.0, "fm_attack": .50, "fm_decay": 1.6, "fm_sustain": .65,
                    "fm_release": .55, "fm_envelope_amount": 1.5,
                    "lfo_cutoff_enabled": 1.0, "lfo_cutoff_wave": 1.0, "lfo_cutoff_rate": .11, "lfo_cutoff_depth": .12,
                    "space_mix": .12, "space_size": .35, "output": -6.32},
         "notes": [(0, 4, 60, 88), (6, .25, 64, 88), (7, .25, 67, 88), (8, 4, 72, 96),
                   (14, 6, 53, 80), (14, 6, 60, 84), (14, 6, 64, 88),
                   (24, 5, 55, 84), (24, 5, 62, 80), (24, 5, 69, 92), (34, 4, 60, 64)]},
        {"family": "rubber-relay", "name": "Rubber Relay", "category": "Basses", "host_category": "Bass", "mono": True,
         "anchor": "review:crooked-velvet:r2", "axis": "filter bite into clocked FM chewing", "bpm": 120, "seconds": 26,
         "brief": "A rounded bass with a brief opening filter bite and continuing eighth-note triangle FM motion. Short detached notes articulate; longer roots reveal the chewing timbre while carrier pitch stays put.",
         "exclude": "No pitch wobble, glide, sub-oscillator claim, high stress or amplitude gating. Integer-ratio primary FM, not a guarantee of an unchanging bass spectrum.",
         "values": {"coupling": .12, "stress": .01, "response": .035, "fm_depth": .65,
                    "amp_attack": .004, "amp_decay": .24, "amp_sustain": .80, "amp_release": .12,
                    "cutoff": 550.0, "resonance": .30, "filter_key_tracking": .75, "filter_velocity": .50,
                    "filter_attack": .002, "filter_decay": .09, "filter_sustain": .08, "filter_release": .12, "filter_amount": 2.0,
                    "fm_ratio": 1.0, "fm_attack": .002, "fm_decay": .055, "fm_sustain": .05, "fm_release": .10, "fm_envelope_amount": .65,
                    "lfo_fm_enabled": 1.0, "lfo_fm_wave": 1.0, "lfo_fm_sync": 1.0, "lfo_fm_division": 12.0, "lfo_fm_depth": .38,
                    "output": -5.38},
         "notes": [(0, 6, 36, 96), (8, .4, 36, 96), (9, .4, 36, 96), (10, .4, 43, 88), (11, .4, 41, 92),
                   (12, 6, 36, 96), (20, 6, 41, 92), (28, .5, 36, 72), (29, .5, 43, 96), (30, .5, 46, 84),
                   (32, 6, 48, 92), (40, 4, 31, 96)]},
        {"family": "gate-garden", "name": "Gate Garden", "category": "Sequence elements", "host_category": "Sequences", "mono": False,
         "anchor": "review:socket-dance:r2", "axis": "straight gate against dotted timbre", "bpm": 120, "seconds": 30,
         "brief": "A held chord becomes a pattern: straight-eighth square amplitude attenuation against dotted-eighth triangle cutoff, revealed by a slow shared filter attack. Played note changes are explicit; the patch is not an arpeggiator.",
         "exclude": "No MIDI generation, independent per-note filters, note-reset LFO or hard-gated reverb. Chopping precedes Space, whose small tail can bridge gaps.",
         "values": {"coupling": .10, "response": .04, "fm_depth": 1.05,
                    "amp_attack": .04, "amp_decay": 1.0, "amp_sustain": .85, "amp_release": .28,
                    "cutoff": 950.0, "resonance": .18, "filter_key_tracking": .25,
                    "filter_attack": 1.2, "filter_decay": 1.0, "filter_sustain": .80, "filter_release": .30, "filter_amount": 2.3,
                    "lfo_amplitude_enabled": 1.0, "lfo_amplitude_wave": 3.0, "lfo_amplitude_sync": 1.0,
                    "lfo_amplitude_division": 12.0, "lfo_amplitude_depth": .88,
                    "lfo_cutoff_enabled": 1.0, "lfo_cutoff_wave": 1.0, "lfo_cutoff_sync": 1.0,
                    "lfo_cutoff_division": 13.0, "lfo_cutoff_depth": 1.0,
                    "space_mix": .13, "space_size": .38, "output": -9.69},
         "notes": [(0, 12, 48, 88), (0, 12, 55, 80), (0, 12, 60, 84), (0, 12, 64, 88),
                   (16, 12, 53, 88), (16, 12, 60, 80), (16, 12, 64, 84), (16, 12, 67, 88),
                   (32, .5, 60, 88), (33, .5, 64, 88), (34, .5, 67, 88), (35, .5, 72, 88),
                   (40, 10, 55, 88), (40, 10, 62, 80), (40, 10, 65, 84)]},
        {"family": "paper-motor", "name": "Paper Motor", "category": "Experimental", "host_category": "Textures", "mono": True,
         "anchor": "review:tendon-drift:r1", "axis": "stepped stress and fluttering material", "bpm": 100, "seconds": 28,
         "brief": "A small papery motor: synced sixteenth-note sample-and-hold Stress, slower smooth Coupling movement and shallow free tremolo. A short FM-envelope tick marks the entrance; both taps and holds are demonstrated.",
         "exclude": "Not noise synthesis, sampled paper or a retriggered percussion sequencer. Stepped stress changes the existing network; its level stays modest rather than becoming a distortion blast.",
         "values": {"coupling": .32, "stress": .13, "response": .06, "fm_depth": .55,
                    "amp_attack": .008, "amp_decay": .40, "amp_sustain": .45, "amp_release": .18,
                    "cutoff": 2400.0, "resonance": .28, "filter_key_tracking": .35,
                    "filter_attack": .003, "filter_decay": .06, "filter_sustain": .12, "filter_release": .15, "filter_amount": .60,
                    "fm_ratio": 1.414, "fm_attack": .001, "fm_decay": .04, "fm_sustain": 0.0, "fm_release": .10, "fm_envelope_amount": 1.1,
                    "lfo_stress_enabled": 1.0, "lfo_stress_wave": 4.0, "lfo_stress_sync": 1.0,
                    "lfo_stress_division": 15.0, "lfo_stress_depth": .10,
                    "lfo_coupling_enabled": 1.0, "lfo_coupling_rate": .43, "lfo_coupling_depth": .12,
                    "lfo_amplitude_enabled": 1.0, "lfo_amplitude_rate": 7.2, "lfo_amplitude_depth": .24,
                    "space_mix": .04, "output": -1.30},
         "notes": [(0, 6, 48, 88), (8, .20, 48, 88), (9, .20, 48, 88), (10, .20, 48, 88), (11, .20, 48, 88),
                   (14, 7, 55, 96), (24, 6, 60, 80), (33, .30, 43, 96), (34, .30, 55, 88), (36, 4, 48, 88)]},
        {"family": "closing-argument", "name": "Closing Argument", "category": "Leads", "host_category": "Leads & Brass", "mono": True,
         "anchor": "review:reed-snap:r2", "axis": "negative Tone envelope with sustained body", "bpm": 100, "seconds": 27,
         "brief": "A speaking lead: the initial bright vowel folds inward while amplitude holds. A negative shared filter envelope supplies the closing gesture, independent FM decay adds articulation and restrained free tremolo gives a living held center.",
         "exclude": "No formant-filter model, pitch entrance or delayed vibrato. Detached notes retrigger the gesture; overlapping notes still obey the existing global/filter ownership rules.",
         "values": {"coupling": .15, "stress": .015, "response": .05, "fm_depth": 1.05,
                    "amp_attack": .008, "amp_decay": .40, "amp_sustain": .82, "amp_release": .18,
                    "cutoff": 5200.0, "resonance": .32, "filter_key_tracking": .45,
                    "filter_attack": .16, "filter_decay": 1.8, "filter_sustain": .85, "filter_release": .20, "filter_amount": -2.6,
                    "fm_ratio": 1.0, "fm_attack": .002, "fm_decay": .30, "fm_sustain": .15, "fm_release": .15, "fm_envelope_amount": .80,
                    "lfo_amplitude_enabled": 1.0, "lfo_amplitude_rate": 4.1, "lfo_amplitude_depth": .15,
                    "space_mix": .06, "space_size": .25, "output": -2.27},
         "notes": [(0, 4, 60, 88), (6, .45, 60, 88), (7, .45, 64, 88), (8, .45, 67, 88), (9, 3, 72, 100),
                   (14, 5, 65, 88), (22, 3, 62, 72), (27, .5, 67, 88), (28, .5, 69, 96), (30, 6, 72, 92)]},
        {"family": "undertint", "name": "Undertint", "category": "Pads", "host_category": "Pads", "mono": False,
         "anchor": "review:stillwater:r1", "axis": "slow index light with early spectral release", "bpm": 100, "seconds": 38,
         "brief": "A dark chord grows a luminous edge without obvious wobble. Slow FM attack and opposite-polarity slow cutoff/Coupling motion create the breathing body. On release the FM contour closes before amplitude, leaving a darker audible tail.",
         "exclude": "No independently phased voices or stereo chorus. Both LFOs are global; equal free rates with opposite depth signs supply the opposing motion. FM release cannot prolong voice lifetime.",
         "values": {"coupling": .16, "response": .06, "fm_depth": .15,
                    "amp_attack": 1.2, "amp_decay": 3.0, "amp_sustain": .80, "amp_release": 2.8,
                    "cutoff": 2400.0, "resonance": .12, "filter_key_tracking": .30,
                    "filter_attack": 1.5, "filter_decay": 2.0, "filter_sustain": .50, "filter_release": .60, "filter_amount": .70,
                    "fm_ratio": 2.0, "fm_attack": 2.4, "fm_decay": 2.0, "fm_sustain": .75, "fm_release": .40, "fm_envelope_amount": 1.7,
                    "lfo_cutoff_enabled": 1.0, "lfo_cutoff_rate": .07, "lfo_cutoff_depth": .50,
                    "lfo_coupling_enabled": 1.0, "lfo_coupling_rate": .07, "lfo_coupling_depth": -.08,
                    "space_mix": .23, "space_size": .60, "output": -10.12},
         "notes": [(0, 10, 48, 80), (0, 10, 55, 76), (0, 10, 59, 80), (0, 10, 64, 84),
                   (16, 10, 53, 80), (16, 10, 60, 76), (16, 10, 64, 80), (16, 10, 69, 84),
                   (34, 10, 55, 80), (34, 10, 62, 76), (34, 10, 65, 80), (34, 10, 71, 84),
                   (50, 3, 60, 88), (50, 3, 67, 80)]},
        {"family": "loose-change", "name": "Loose Change", "category": "Physical materials", "host_category": "Drums & Percussion", "mono": False,
         "anchor": "review:skin-knock:r1", "axis": "short metallic envelope under phrase-level variation", "bpm": 120, "seconds": 24,
         "brief": "A pitched metallic knock with a short amplitude/FM contour and slow free sample-and-hold FM variation. Identical repeated MIDI strikes encounter different global phases across a phrase; a held key dies away rather than sustaining the motor.",
         "exclude": "Not round-robin sampling, per-note random modulation, coin recordings or automatic hits. Random-like cycle values are deterministic on reprepare; the LFO does not reset with each note.",
         "values": {"coupling": .10, "response": .035, "fm_depth": .50,
                    "amp_attack": .001, "amp_decay": .22, "amp_sustain": 0.0, "amp_release": .22,
                    "cutoff": 7000.0, "resonance": .18, "filter_key_tracking": .50, "filter_velocity": .20,
                    "fm_ratio": 2.756, "fm_attack": .001, "fm_decay": .065, "fm_sustain": 0.0, "fm_release": .14, "fm_envelope_amount": 2.0,
                    "lfo_fm_enabled": 1.0, "lfo_fm_wave": 4.0, "lfo_fm_rate": .65, "lfo_fm_depth": .20,
                    "space_mix": .08, "space_size": .20, "output": -1.45},
         "notes": [(i * .75, .20, 60, 88) for i in range(12)] + [(12, 4, 60, 88)]
                  + [(20 + i * .75, .20, 67, 88) for i in range(12)]
                  + [(32, .25, 48, 72), (34, .25, 72, 96), (36, .25, 60, 88)]},
        {"family": "carousel-wire", "name": "Carousel Wire", "category": "Physical materials", "host_category": "Plucks & Mallets", "mono": False,
         "anchor": "review:rosewood-tap:r1", "axis": "zero-depth LFO routing around a fixed pluck", "bpm": 120, "seconds": 26,
         "brief": "A fixed repeated pluck alternates between hollow, wiry and rounded. The enabled FM LFO has zero direct index depth but routes gently to primary ratio and operator mix; a brief independent FM envelope keeps the attack recognizable.",
         "exclude": "No note generation, pitch LFO, envelope retrigger from the LFO or changing MIDI velocities in the repeated section. Ratio motion changes FM partials, not the carrier note; it is not a tuning-stable acoustic model.",
         "values": {"coupling": .07, "response": .04, "fm_depth": .40,
                    "amp_attack": .003, "amp_decay": .40, "amp_sustain": .10, "amp_release": .18,
                    "cutoff": 5000.0, "resonance": .12, "filter_key_tracking": .55,
                    "filter_attack": .002, "filter_decay": .08, "filter_sustain": .10, "filter_release": .16, "filter_amount": .30,
                    "fm_ratio": 2.0, "fm_operator_mix": .12,
                    "fm_attack": .001, "fm_decay": .055, "fm_sustain": 0.0, "fm_release": .14, "fm_envelope_amount": 1.15,
                    "lfo_fm_enabled": 1.0, "lfo_fm_wave": 1.0, "lfo_fm_sync": 1.0, "lfo_fm_division": 10.0, "lfo_fm_depth": 0.0,
                    "fm_route_1_source": 1.0, "fm_route_1_target": 1.0, "fm_route_1_amount": .14,
                    "fm_route_2_source": 1.0, "fm_route_2_target": 3.0, "fm_route_2_amount": .08,
                    "space_mix": .08, "space_size": .28, "output": -1.57},
         "notes": [(i, .35, 60, 88) for i in range(12)]
                  + [(16 + i * .75, .30, (60, 64, 67, 72)[i % 4], 88) for i in range(12)]
                  + [(28, 5, 55, 84), (28, 5, 62, 84), (36, 4, 72, 88)]}
    ]


def files():
    content, entries = {}, []
    for sketch in sketches():
        values = base.neutral()
        if set(sketch["values"]) - set(values):
            raise ValueError("Unknown motion-study parameter")
        values.update(sketch["values"])
        values["lfo_tempo"] = float(sketch["bpm"])
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:motion-envelope-study:" + stem).hex,
                  "name": "Review / " + sketch["category"] + " / " + sketch["name"] + " r1",
                  "mono": sketch["mono"], "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        content[stem + ".mid"] = base.encoder.performance(sketch["name"] + " / LFO and envelope audition", sketch["bpm"], sketch["notes"])
        entries.append({"file": stem, "category": sketch["category"], "host_category": sketch["host_category"],
                        "seconds": sketch["seconds"], "study_axis": sketch["axis"], "intent": sketch["brief"] + " " + sketch["exclude"]})
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
            if (HERE / name).read_bytes() != data:
                raise SystemExit("Frozen motion study differs: " + name)
        else:
            with (HERE / name).open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__": main()
