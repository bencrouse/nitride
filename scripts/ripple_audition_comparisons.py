#!/usr/bin/env python3
"""Same-source r1/r2 Ripple contrasts; magnitude evidence is not listening approval."""
import argparse
import json
import math
from pathlib import Path
import shutil

import factory_filter_comparisons as audio
import rage_comparisons as spectral

MODES = ("neutral", "r1", "r2")
ALLOWED = {"cutoff", "resonance", "ripple_dry", "ripple_drive", "ripple_feedback", "filter_amount", "lfo_cutoff_rate", "lfo_cutoff_depth", "output"}


def stage(batch, output):
    batch, output = Path(batch), Path(output); output.mkdir(parents=True, exist_ok=False)
    metadata = json.loads((batch / "batch.json").read_text())
    for mode in MODES:
        destination = output / mode; destination.mkdir(); entries = []
        for entry in metadata["sketches"]:
            family = entry["file"].removesuffix("-r2")
            old = batch.parent / "r1" / (family + "-r1.nitridepreset"); new = batch / (family + "-r2.nitridepreset")
            a, b = json.loads(old.read_text()), json.loads(new.read_text())
            if a["values"].keys() != b["values"].keys() or {k for k in a["values"] if a["values"][k] != b["values"][k]} - ALLOWED:
                raise ValueError("Non-filter/source change in comparison: " + family)
            if (a["mono"], a["chord"]) != (b["mono"], b["chord"]) or old.with_suffix(".mid").read_bytes() != new.with_suffix(".mid").read_bytes():
                raise ValueError("MIDI/voicing mismatch")
            definition = b if mode == "r2" else a
            if mode == "neutral": definition["values"].update(cutoff=18000., resonance=0., filter_amount=0., filter_key_tracking=0., filter_velocity=0., filter_fm_depth=0., lfo_cutoff_enabled=0., ripple_type=0., ripple_mode=0., ripple_dry=1., ripple_drive=0., ripple_feedback=0.)
            (destination / (family + ".nitridepreset")).write_text(json.dumps(definition, indent=2) + "\n")
            shutil.copy2(old.with_suffix(".mid"), destination / (family + ".mid"))
            entries.append({**entry, "file": family})
        (destination / "batch.json").write_text(json.dumps({**metadata, "sketches": entries}, indent=2) + "\n")


def magnitude_profile(bank):
    size = 4096; frames = len(bank)//2; step = max(size, frames//64)
    window = [.5-.5*math.cos(2*math.pi*n/(size-1)) for n in range(size)]
    power = [0.]*(size//2+1); count = 0
    for offset in range(0, frames-size+1, step):
        values = [bank[2*(offset+n)]/8388608 for n in range(size)]
        if sum(x*x for x in values)/size < 1e-9: continue
        bins = spectral._fft([x*w for x, w in zip(values, window)])
        for k in range(1, size//2): power[k] += abs(bins[k])**2
        count += 1
    if not count: raise ValueError("Silent magnitude profile")
    return [math.sqrt(p/count) for p in power[2:int(18000*size/48000)+1]]


def require_contrast(rows):
    if len(rows) != 6: raise ValueError("Six complete auditions required")
    for row in rows:
        a, b = row["r1_spectral_residual_db"], row["r2_spectral_residual_db"]
        if not all(math.isfinite(x) for x in (a, b)) or b < -16 or b-a < 10:
            raise ValueError("Insufficient gain/phase-independent tonal contrast: " + row["family"])


def compare(inputs, renders, output):
    inputs, renders, output = Path(inputs), Path(renders), Path(output); output.mkdir(parents=True, exist_ok=False)
    metadata = json.loads((inputs / "r2/batch.json").read_text())
    reports = {mode: {r["name"]: r for r in json.loads((renders / mode / "report.json").read_text())} for mode in MODES}
    rows = []
    for entry in metadata["sketches"]:
        family = entry["file"]; banks, metrics = [], []
        for mode in MODES:
            name = json.loads((inputs / mode / (family + ".nitridepreset")).read_text())["name"]
            metric = reports[mode][name]
            if metric["faults"] or not metric["deterministic"] or metric["otherBlockMaximumError"] >= 1e-5: raise ValueError("Failed rendering guards")
            banks.append(audio.pcm(renders / mode / (family + "-raw.wav"))); metrics.append(metric)
        profiles = [magnitude_profile(bank) for bank in banks]
        target = min(-24., *(m["rawActiveRmsDbfs"]-m["rawPeakDbfs"]-3 for m in metrics))
        gains = [10**((target-m["rawActiveRmsDbfs"])/20) for m in metrics]
        filename = family + " - neutral then r1 then r2.wav"; audio.sequence(output / filename, banks, gains)
        rows.append({"family": family, "file": filename, "r1_spectral_residual_db": audio.residual(profiles[0], profiles[1]),
            "r2_spectral_residual_db": audio.residual(profiles[0], profiles[2]), "r1_to_r2_spectral_residual_db": audio.residual(profiles[1], profiles[2]),
            "target_active_rms_dbfs": target, "metrics": dict(zip(MODES, metrics)), "listening_approved": False})
    require_contrast(rows)
    report = {"format": "nitride-ripple-r2-comparisons", "version": 1, "presets": rows,
        "note": "Near-flat same-route reference -> r1 -> r2; same MIDI/oscillator/FM/amplitude settings. One common active-RMS target with >=3dB peak headroom, not LUFS. Welch magnitude profiles remove phase and constant gain. Numerical contrast gates require >-16dB r2 residual and >=10dB greater separation than r1, not audibility/quality approval. Raw levels/timings remain visible."}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "LISTEN.txt").write_text(report["note"] + "\nStart quietly. No factory promotion or DSP change.\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__); sub = parser.add_subparsers(dest="action", required=True)
    p = sub.add_parser("stage"); p.add_argument("batch", type=Path); p.add_argument("output", type=Path)
    p = sub.add_parser("compare"); p.add_argument("inputs", type=Path); p.add_argument("renders", type=Path); p.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.action == "stage": stage(args.batch, args.output)
    else: print(json.dumps(compare(args.inputs, args.renders, args.output), indent=2))


if __name__ == "__main__": main()
