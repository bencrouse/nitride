#!/usr/bin/env python3
"""Independent Ripple evidence; failures and unavailable host/hardware checks stay visible."""
import argparse
import json
import math
from pathlib import Path
import re
import shutil

import factory_filter_comparisons as audio
import filter_qualification as filters

MODES = ("classic", "linear", "ripple")


def stage(batch, output):
    batch, output = Path(batch), Path(output)
    output.mkdir(parents=True, exist_ok=False)
    metadata = json.loads((batch / "batch.json").read_text())
    for mode in MODES:
        destination = output / mode; destination.mkdir()
        (destination / "batch.json").write_text(json.dumps(metadata, indent=2) + "\n")
        for sketch in metadata["sketches"]:
            stem = sketch["file"]; definition = json.loads((batch / (stem + ".nitridepreset")).read_text())
            if mode == "classic":
                definition["version"] = 6
                definition["values"] = {k: v for k, v in definition["values"].items() if not k.startswith(("ripple_", "rage_")) and k != "filter_family"}
            elif mode == "linear":
                definition["values"].update(ripple_mode=0., ripple_drive=0., ripple_feedback=0.)
            (destination / (stem + ".nitridepreset")).write_text(json.dumps(definition, indent=2) + "\n")
            shutil.copy2(batch / (stem + ".mid"), destination / (stem + ".mid"))


def compare(inputs, renders, output):
    inputs, renders, output = Path(inputs), Path(renders), Path(output)
    output.mkdir(parents=True, exist_ok=False); rows = []
    reports = {mode: {r["name"]: r for r in json.loads((renders / mode / "report.json").read_text())} for mode in MODES}
    metadata = json.loads((inputs / "ripple/batch.json").read_text())
    for sketch in metadata["sketches"]:
        stem = sketch["file"]; metrics, banks = {}, []
        for mode in MODES:
            definition = json.loads((inputs / mode / (stem + ".nitridepreset")).read_text()); metric = reports[mode][definition["name"]]
            if metric["faults"] != 0 or not metric["deterministic"] or metric["otherBlockMaximumError"] >= 1e-5:
                raise ValueError("Failed numerical/determinism/block guard: " + stem)
            metrics[mode] = metric; banks.append(audio.pcm(renders / mode / (stem + "-raw.wav")))
        target = min(-24, *(m["rawActiveRmsDbfs"] - m["rawPeakDbfs"] - 3 for m in metrics.values()))
        gains = [10**((target - metrics[mode]["rawActiveRmsDbfs"]) / 20) for mode in MODES]
        filename = stem + " - Classic then linear Ripple then voiced Ripple.wav"
        audio.sequence(output / filename, banks, gains)
        rows.append({"file": filename, "target_active_rms_dbfs": target, "metrics": metrics,
                     "classic_to_linear_gain_aligned_residual_db": audio.residual(banks[0], banks[1]),
                     "linear_to_voiced_gain_aligned_residual_db": audio.residual(banks[1], banks[2]), "listening_approved": False})
    result = {"format": "nitride-ripple-musical-comparisons", "version": 1, "presets": rows,
              "note": "Identical original MIDI/settings except filter family/mode/drive/feedback. Classic -> linear Ripple -> voiced Ripple, half-second gaps; shared active-RMS target with >=3 dB peak headroom, not LUFS or a quality verdict. Dry addition and phase cancellation remain audible. No Pigments reference."}
    (output / "report.json").write_text(json.dumps(result, indent=2) + "\n")
    (output / "LISTEN.txt").write_text(result["note"] + "\nStart quietly. Factory selections remain unchanged.\n")
    return result


def performance(report):
    expected = {f"ripple-{t}-{m}{v}/{r}/{b}" for t in ("subtle", "medium", "hard") for m in ("shape", "negative", "positive")
                for v in (1, 4, 12) for r in (48000, 96000) for b in (64, 256)}
    rows = report.get("cases", [])
    if report.get("format") != "nitride-performance" or report.get("settings") != {"passes": 3, "seconds": 2, "warmup": .5} or report.get("machine", {}).get("configuration") != "Release":
        raise ValueError("Expected full Release Ripple timing settings")
    if len(rows) != 108 or {r["key"] for r in rows} != expected: raise ValueError("Complete 108-case matrix required")
    assessed = []
    for row in rows:
        name, rate, block = row["key"].split("/"); voices = int(re.search(r"\d+$", name).group())
        if (row["sampleRate"], row["blockSize"], row["heldNotes"], row["minimumVoices"], row["maximumVoices"]) != (int(rate), int(block), voices, voices, voices):
            raise ValueError("Incorrect clock/voice identity")
        if any(not math.isfinite(row[k]) for k in ("budgetMicros", "p99Micros", "medianPassLoadPercent")) or row["budgetMicros"] <= 0 or row["overBudgetBlocks"] < 0:
            raise ValueError("Invalid timing")
        if row.get("numericalFaults") != 0: raise ValueError("Missing or nonzero numerical fault count")
        assessed.append({"key": row["key"], "passed_offline_headroom_gate": row["overBudgetBlocks"] == 0 and row["p99Micros"] <= .8*row["budgetMicros"],
                         "median_load_percent": row["medianPassLoadPercent"], "p99_budget_percent": 100*row["p99Micros"]/row["budgetMicros"], "overdue_blocks": row["overBudgetBlocks"]})
    return {"all_offline_headroom_gates_pass": all(r["passed_offline_headroom_gate"] for r in assessed), "cpu": report["machine"].get("cpu"), "cases": assessed,
            "boundary": "p99 <= 80% deadline AND zero overdue blocks. Not a real-time device, M2 or Logic qualification."}


def summarize(folder):
    folder = Path(folder); result = {"format": "nitride-ripple-qualification", "version": 1, "release_qualified": False,
        "pigments_sonic_equivalence": "not-claimed-not-tested", "m2": "unverified", "logic_reopen": "manual-pending", "logic_device_soak": "manual-pending"}
    identities = set()
    for name in ("performance", "spectral", "continuity"):
        path = folder / (name + ".json")
        if not path.is_file(): result[name] = {"status": "missing-not-qualified"}; continue
        data = json.loads(path.read_text()); identity = data.get("synthVersion"), data.get("evaluationBuild")
        if not all(isinstance(x, str) and x for x in identity): raise ValueError("Missing build identity")
        identities.add(identity)
        if name == "performance": result[name] = performance(data)
        elif name == "continuity":
            if len(data.get("cases", [])) != 1 or data["cases"][0]["key"] != "ripple-hard-positive4/48000/256": raise ValueError("Unexpected Ripple soak")
            data["cases"][0]["key"] = "rage-diode4/48000/256"  # Reuse clock/duration/state/deadline guards, not Rage evidence.
            result[name] = filters.continuity(data)
        else:
            expected = {(r, t, m, p) for r in (48000, 96000) for t in range(3) for m in ("waveshape", "negative-feedback", "positive-feedback") for p in ("low-tone", "high-tone", "cutoff-lfo-and-audio-fm")}
            rows = data.get("results", [])
            if data.get("format") != "nitride-ripple-spectral-probes" or len(rows) != 54 or {(r["sampleRate"], r["type"], r["mode"], r["probe"]) for r in rows} != expected: raise ValueError("Complete 54-case spectral matrix required")
            if any(not math.isfinite(r["reference32xResidualDb"]) or any(not 0 <= r[k] < 1e-9 for k in ("maximumSolverResidual", "reference32xMaximumSolverResidual")) for r in rows): raise ValueError("Spectral numerical guard failed")
            if any(any(not math.isfinite(r[k]) for k in ("nonHarmonicEnergyDb", "reference32xNonHarmonicEnergyDb")) for r in rows if r["probe"] != "cutoff-lfo-and-audio-fm"): raise ValueError("Invalid spectral energy")
            result[name] = {"numerical_guards_pass": True, "spectral_cleanliness_qualified": False, "large_reference_residuals": [r for r in rows if r["reference32xResidualDb"] > -20], "boundary": data["note"]}
    if len(identities) > 1: raise ValueError("Evidence identities differ")
    if identities: result["synthVersion"], result["evaluationBuild"] = next(iter(identities))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__); sub = parser.add_subparsers(dest="action", required=True)
    p = sub.add_parser("stage"); p.add_argument("batch", type=Path); p.add_argument("output", type=Path)
    p = sub.add_parser("compare"); p.add_argument("inputs", type=Path); p.add_argument("renders", type=Path); p.add_argument("output", type=Path)
    p = sub.add_parser("summarize"); p.add_argument("folder", type=Path)
    args = parser.parse_args()
    if args.action == "stage": stage(args.batch, args.output)
    elif args.action == "compare": print(json.dumps(compare(args.inputs, args.renders, args.output), indent=2))
    else: print(json.dumps(summarize(args.folder), indent=2))


if __name__ == "__main__": main()
