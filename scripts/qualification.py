#!/usr/bin/env python3
"""Local qualification reporting. Never changes audio devices, projects, presets or telemetry."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import platform
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MATRIX = Path(__file__).with_name("qualification_matrix.tsv")


def matrix(path=MATRIX):
    result = []
    for line in Path(path).read_text().splitlines():
        if not line or line.startswith("#"): continue
        group, rate, block, name = line.split("\t")
        if group not in ("ordinary", "fm-expansion") or not name.replace("-", "").isalnum(): raise ValueError("Invalid workload")
        rate, block = int(rate), int(block)
        if rate not in (44100, 48000, 96000) or block not in (64, 128, 256): raise ValueError("Unexpected qualification clock")
        result.append((group, rate, block, name))
    if len(result) != len(set(result)) or not result: raise ValueError("Duplicate or empty matrix")
    return result


def assess(case):
    budget = case["budgetMicros"]
    if any(not math.isfinite(case[k]) for k in ("budgetMicros", "p99Micros", "medianPassLoadPercent")):
        raise ValueError("Non-finite benchmark result")
    if not budget > 0 or not 0 <= case["minimumVoices"] <= case["maximumVoices"] <= 12 or case["overBudgetBlocks"] < 0:
        raise ValueError("Invalid benchmark result")
    return {"key": case["key"], "p99_budget_percent": case["p99Micros"] / budget * 100,
            "median_load_percent": case["medianPassLoadPercent"], "over_budget_blocks": case["overBudgetBlocks"],
            "passed_offline_gate": case["p99Micros"] <= .8 * budget and case["overBudgetBlocks"] == 0,
            "qualification": "Offline p99<=80%deadline AND zero observed overdue blocks. Not device xruns, Logic validation or a universal hardware guarantee."}


def summarize(folder):
    folder = Path(folder); rows = []
    expected_matrix = matrix()
    if (folder / "matrix.tsv").exists() and matrix(folder / "matrix.tsv") != expected_matrix:
        raise ValueError("Returned qualification matrix differs from the complete pinned matrix")
    for group, rate, block, name in expected_matrix:
        path = folder / f"{group}-{rate}-{block}-{name}.json"
        if not path.is_file():
            rows.append({"key": f"{name}/{rate}/{block}", "status": "missing-not-qualified"}); continue
        report = json.loads(path.read_text())
        if report.get("format") != "nitride-performance" or len(report["cases"]) != 1: raise ValueError("Unexpected benchmark document")
        if report["settings"] != {"passes": 3, "seconds": 10, "warmup": 2} or report["machine"]["configuration"] != "Release":
            raise ValueError("Qualification duration/warmup/passes/build differs from the fixed matrix")
        case = report["cases"][0]
        if (case["sampleRate"], case["blockSize"], case["workload"]) != (rate, block, name): raise ValueError("Workload mismatch")
        rows.append({**assess(case), "status": "passed-offline-gate" if assess(case)["passed_offline_gate"] else "not-qualified-offline", "machine": report["machine"]})
    result = {"format": "nitride-release-qualification", "version": 1, "cases": rows,
              "m2_execution": "not-inferred; actual machine field identifies the executing computer",
              "logic_project_reopen": "manual-pending", "logic_soak": "manual-pending",
              "note": "No missing/failed/manual-pending case is treated as passed. Raw timing outliers are retained; no threshold relaxation or automatic quality/polyphony change."}
    return result


def run(tool, output):
    output = Path(output); output.mkdir(parents=True, exist_ok=False)
    tool = Path(tool).resolve()
    rows = matrix(); (output / "matrix.tsv").write_bytes(MATRIX.read_bytes())
    with (output / "run-identity.json").open("x") as stream:
        stream.write(json.dumps({"tool_sha256": hashlib.sha256(tool.read_bytes()).hexdigest(), "architecture": platform.machine(),
            "cpu": subprocess.check_output(["sysctl", "-n", "machdep.cpu.brand_string"], text=True).strip(),
            "macos": subprocess.check_output(["sw_vers", "-productVersion"], text=True).strip(),
            "macos_build": subprocess.check_output(["sw_vers", "-buildVersion"], text=True).strip(),
            "privacy": "No hostname/serial/project/settings capture, network, audio device or live playback."}, indent=2) + "\n")
    for index, (group, rate, block, name) in enumerate(rows):
        stem = f"{group}-{rate}-{block}-{name}"
        command = [str(tool), "--passes", "3", "--seconds", "10", "--warmup", "2", "--rate", str(rate), "--block", str(block), "--case", name,
                   "--label", "qualification-001", "--output", str((output / (stem + ".json")).resolve())]
        if group != "ordinary": command.append("--" + group)
        with (output / (stem + ".log")).open("x") as stream: result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT)
        result.check_returncode()
        print(f"{index + 1}/{len(rows)} {stem}", flush=True)
    with (output / "summary.json").open("x") as stream: stream.write(json.dumps(summarize(output), indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__); sub = parser.add_subparsers(dest="action", required=True)
    measure = sub.add_parser("measure"); measure.add_argument("tool", type=Path); measure.add_argument("output", type=Path)
    summary = sub.add_parser("summarize"); summary.add_argument("folder", type=Path)
    args = parser.parse_args()
    if args.action == "measure": run(args.tool, args.output)
    else: print(json.dumps(summarize(args.folder), indent=2))


if __name__ == "__main__": main()
