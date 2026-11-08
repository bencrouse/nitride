#!/usr/bin/env python3
"""Summarize filter evidence without inferring M2/Logic passes or hiding failures."""
import argparse
import json
import math
from pathlib import Path
import re


def expected_cases():
    return {f"rage-{character}{voices}/{rate}/{block}" for character in ("diode", "distortion", "softclip", "tape", "transistor")
            for voices in (1, 4, 12) for rate in (48000, 96000) for block in (64, 256)} | {
                f"rage-advanced12/{rate}/{block}" for rate in (48000, 96000) for block in (64, 256)}


def performance(report):
    if report.get("format") != "nitride-performance" or report.get("settings") != {"passes": 3, "seconds": 2, "warmup": .5}:
        raise ValueError("Unexpected performance format/settings")
    if report.get("machine", {}).get("configuration") != "Release": raise ValueError("Expected Release timing")
    rows = report["cases"]
    if len(rows) != 64 or {r["key"] for r in rows} != expected_cases(): raise ValueError("Complete 64-case matrix required")
    assessed = []
    for row in rows:
        name, rate, block = row["key"].split("/")
        voices = 12 if name == "rage-advanced12" else int(re.search(r"\d+$", name).group())
        if (row["sampleRate"], row["blockSize"], row["heldNotes"]) != (int(rate), int(block), voices):
            raise ValueError("Case key/workload identity differs")
        if any(not math.isfinite(row[k]) for k in ("budgetMicros", "p99Micros", "medianPassLoadPercent")) or row["budgetMicros"] <= 0:
            raise ValueError("Invalid timing")
        if row["overBudgetBlocks"] < 0 or row["minimumVoices"] != row["heldNotes"] or row["maximumVoices"] != row["heldNotes"]:
            raise ValueError("Invalid workload voices/deadlines")
        passed = row["overBudgetBlocks"] == 0 and row["p99Micros"] <= .8*row["budgetMicros"]
        assessed.append({"key": row["key"], "passed_offline_headroom_gate": passed,
                         "median_load_percent": row["medianPassLoadPercent"], "p99_budget_percent": 100*row["p99Micros"]/row["budgetMicros"],
                         "overdue_blocks": row["overBudgetBlocks"]})
    cpu = report["machine"].get("cpu", "unknown")
    return {"cpu_reported_by_executable": cpu, "m2_hardware_reported": bool(re.fullmatch(r"Apple M2(?: Pro| Max| Ultra)?", cpu)),
            "all_offline_headroom_gates_pass": all(r["passed_offline_headroom_gate"] for r in assessed), "cases": assessed,
            "boundary": "p99 <= 80% deadline AND zero overdue blocks; not audio-device xruns, Logic or a universal hardware guarantee."}


def artifacts(report):
    if report.get("format") != "nitride-rage-spectral-probes" or report.get("version") != 2: raise ValueError("Unknown spectral format")
    expected = {(rate, c, probe, resonance) for rate in (48000, 96000) for c in range(5)
                for probe, resonance in (("low-tone", .85), ("high-tone", .85), ("cutoff-lfo-and-audio-fm", .85), ("low-tone", .7), ("high-tone", .7))}
    rows = report["results"]
    if len(rows) != 50 or {(r["sampleRate"], r["character"], r["probe"], r["resonance"]) for r in rows} != expected:
        raise ValueError("Complete 50-case spectral matrix required")
    assessed = []
    for row in rows:
        keys = ["maximumSolverResidual", "reference32xMaximumSolverResidual", "reference32xResidualDb"]
        tonal = row["probe"] != "cutoff-lfo-and-audio-fm"
        if tonal: keys += ["nonHarmonicEnergyDb", "reference32xNonHarmonicEnergyDb"]
        if any(not math.isfinite(row[k]) for k in keys): raise ValueError("Non-finite spectral evidence")
        if any(not 0 <= row[k] < 1e-6 for k in keys[:2]): raise ValueError("Implicit solver failed its numerical guard")
        kind = "modulated-response-and-trajectory"
        if tonal:
            a, b = row["nonHarmonicEnergyDb"], row["reference32xNonHarmonicEnergyDb"]
            kind = "strong-nonharmonic-also-in-reference" if min(a, b) > -40 and abs(a-b) < 6 else (
                "rate-dependent-nonharmonic-excess-not-isolated-aliasing" if a-b > 6 else "no-large-reference-nonharmonic-excess")
        assessed.append({"sample_rate": row["sampleRate"], "character": row["character"], "probe": row["probe"], "resonance": row["resonance"],
                         "classification": kind, "large_reference_residual": row["reference32xResidualDb"] > -20,
                         "reference_residual_db": row["reference32xResidualDb"]})
    return {"numerical_guards_pass": True, "spectral_cleanliness_qualified": False, "cases": assessed,
            "boundary": "Diagnostic thresholds are not audibility claims. Intrinsic nonlinear response, phase and trajectory differences are not pure alias measurements."}


def continuity(report):
    if report.get("format") != "nitride-offline-continuity-soak": raise ValueError("Not an offline continuity report")
    if report["settings"].get("soak") is not True or report["settings"]["passes"] != 1 or not math.isfinite(report["settings"]["seconds"]) or report["settings"]["seconds"] < 1800 or len(report["cases"]) != 1:
        raise ValueError("At least 30 audio minutes in one continuous pass required")
    row = report["cases"][0]
    if row.get("key") != "rage-diode4/48000/256" or (row.get("sampleRate"), row.get("blockSize"), row.get("heldNotes")) != (48000, 256, 4):
        raise ValueError("Unexpected continuity workload/clock")
    if not math.isfinite(row["audioSeconds"]) or row["audioSeconds"] < 1800 or row.get("stateRoundTrips", 0) < 60 or row.get("numericalFaults") != 0:
        raise ValueError("Missing duration/state round trips or numerical faults")
    if not math.isfinite(row["peak"]) or row["peak"] <= 1e-6 or not 0 < row["minimumVoices"] <= row["maximumVoices"] <= 12:
        raise ValueError("Invalid continuity audio/voice evidence")
    if row["overBudgetBlocks"] < 0 or not math.isfinite(row["maxMicros"]) or row["maxMicros"] < 0 or not math.isfinite(report["elapsedWallSeconds"]) or report["elapsedWallSeconds"] <= 0:
        raise ValueError("Invalid continuity timing evidence")
    return {"passed_offline_continuity": True, "audio_seconds": row["audioSeconds"], "state_round_trips": row["stateRoundTrips"],
            "overdue_blocks": row["overBudgetBlocks"], "maximum_block_micros": row["maxMicros"], "elapsed_wall_seconds": report["elapsedWallSeconds"],
            "passed_offline_deadline_gate": row["overBudgetBlocks"] == 0,
            "boundary": "Continuous offline processor audio and in-memory state round trips, NOT a wall-clock Logic/project/device soak."}


def summarize(folder):
    folder = Path(folder); result = {"format": "nitride-filter-qualification-summary", "version": 1,
        "logic_project_reopen": "manual-pending", "logic_soak": "manual-pending", "release_qualified": False}
    identities = set()
    for name, evaluate in (("performance", performance), ("spectral", artifacts), ("continuity", continuity)):
        path = folder/(name+".json")
        if path.is_file():
            data = json.loads(path.read_text())
            identity = data.get("synthVersion"), data.get("evaluationBuild")
            if not all(isinstance(x, str) and x for x in identity): raise ValueError("Evidence lacks build identity")
            identities.add(identity)
            result[name] = evaluate(data)
        else:
            result[name] = {"status": "missing-not-qualified"}
    candidate = folder/"candidate.json"
    if candidate.is_file():
        data = json.loads(candidate.read_text()); identities.add((data["synthVersion"], data["evaluationBuild"]))
    if len(identities) > 1: raise ValueError("Evidence/package build identities differ")
    if identities:
        version, build = next(iter(identities)); result.update(synthVersion=version, evaluationBuild=build)
    native = folder/"native-reopen.json"; status = folder/"native-reopen.status"
    raw = json.loads(native.read_text()) if native.is_file() else {}
    passed = status.is_file() and status.read_text().strip() == "PASS" and raw.get("full_document_exact") is True and raw.get("audio_exact") is True and raw.get("parameters") == 92
    result["native_process_reopen"] = "passed-offline-probe" if passed else "missing-or-failed-not-qualified"
    result["boundary"] = "Missing/manual/external evidence never becomes a pass; no quality reduction, project/settings capture, installer or telemetry."
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("folder", type=Path)
    args = parser.parse_args(); print(json.dumps(summarize(args.folder), indent=2))


if __name__ == "__main__": main()
