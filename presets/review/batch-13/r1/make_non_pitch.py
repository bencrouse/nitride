#!/usr/bin/env python3
"""Three non-pitch jobs selected from raw mechanism probes; --verify, never overwrite."""
import argparse
import importlib.util
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("nitride_non_pitch_base", ROOT.parents[1] / "batch-09/r1/make_studies.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
encoder = base.encoder


def selected_values(probe):
    # Freeze the measured harmonic-probe recipe here so source.zip is self-contained.
    values = base.neutral()
    values.update({"fm_depth": 1.4, "fm_ratio": 4.0, "fm_shape": 2.0, "fm_shape_amount": .5,
                   "amp_attack": .004, "amp_decay": .1, "amp_sustain": .9, "amp_release": .06,
                   "cutoff": 12000.0, "resonance": .05, "filter_key_tracking": 0.0,
                   "filter_attack": 3.5, "filter_decay": .1, "filter_sustain": 1.0,
                   "filter_release": .06, "space_size": .9, "output": -6.0})
    if probe == "swell-level-and-tone":
        values.update({"amp_attack": 3.4, "cutoff": 350.0, "filter_amount": 4.5})
    elif probe == "shutdown-closing":
        values.update({"filter_attack": 3.4, "filter_amount": -5.7})
    elif probe == "flash-room":
        values.update({"amp_attack": .0015, "amp_decay": .065, "amp_sustain": 0.0,
                       "amp_release": .08, "cutoff": 8500.0, "space_mix": .55})
    else:
        raise ValueError("Unknown selected probe")
    return values


def sketches():
    return [
        {"family": "reverse-veil", "name": "Reverse Veil", "probe": "swell-level-and-tone", "output": -4.65,
         "axis": "fixed-pitch level and spectral build", "seconds": 20,
         "intent": "Reverse-like four-second held build from amplitude/Tone opening, no reversed audio or pitch/ratio movement. Raw staged probes separate static dark, amplitude-only and added Tone motion. Release into boundary; no automatic bar-length handoff.",
         "notes": [(0, 8, 48, 96), (12, 8, 48, 96), (24, 1, 48, 96)]},
        {"family": "spectral-cut", "name": "Spectral Cut", "probe": "shutdown-closing", "output": -5.8,
         "axis": "fixed-pitch spectral shutdown", "seconds": 20,
         "intent": "Bright fixed harmonic source closes to a muffled core over3.4s, then note-off clears the boundary. Fixed note/ratio/index, zero network/motion/routes; filter rather than pitch descent. Compare against identical static Tone control.",
         "notes": [(0, 8, 48, 96), (12, 8, 48, 96), (24, 1, 48, 96)]},
        {"family": "flash-room", "name": "Flash Room", "probe": "flash-room", "output": 0.0,
         "axis": "fixed-pitch transient to diffuse tail", "seconds": 20,
         "intent": "Brief fixed harmonic excitation leaves a diffuse room tail, not a pitch drop/noise crash/one-shot mode. Compare dry at same excitation/output/room-size. Output0dB intentional for short-duty source, not active-RMS matched; assess raw/context level and masking.",
         "notes": [(0, .5, 48, 96), (12, .5, 48, 96), (24, 8, 48, 96)]}
    ]


def fixed_paths(values):
    for key in ("pitch", "coupling", "stress", "glide_enabled", "autobend_enabled", "autobend_depth", "legato", "fm_envelope_amount", "filter_fm_depth"):
        assert values[key] == 0, key
    for lane in ("pitch", "coupling", "stress", "fm", "cutoff", "amplitude"):
        assert values[f"lfo_{lane}_enabled"] == 0 and values[f"lfo_{lane}_depth"] == 0
    for n in (1, 2, 3):
        assert values[f"fm_route_{n}_source"] == 0 and values[f"fm_route_{n}_amount"] == 0
    assert values["fm_ratio"] == 4 and values["fm_depth"] == 1.4


def files():
    content, manifest = {}, []
    for sketch in sketches():
        values = selected_values(sketch["probe"])
        values["output"] = sketch["output"]
        fixed_paths(values)
        stem = sketch["family"] + "-r1"
        preset = {"format": "nitride-preset", "version": 6,
                  "id": uuid.uuid5(uuid.NAMESPACE_URL, "nitride:non-pitch:" + stem).hex,
                  "name": "Review / Experimental / " + sketch["name"] + " r1",
                  "mono": True, "chord": False, "values": values}
        content[stem + ".nitridepreset"] = (json.dumps(preset, indent=2) + "\n").encode()
        content[stem + ".mid"] = encoder.performance(sketch["name"] + " r1 / fixed note48 original audition", 120, sketch["notes"])
        manifest.append({"file": stem, "category": "Experimental", "study_axis": sketch["axis"], "intent": sketch["intent"], "seconds": sketch["seconds"]})
    content["batch.json"] = (json.dumps({"format": "nitride-review-batch", "version": 1, "revision": "r1", "sketches": manifest}, indent=2) + "\n").encode()
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Non-pitch files exist; use --verify, never replace frozen presets/performances")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Frozen non-pitch candidate differs: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
