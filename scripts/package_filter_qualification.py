#!/usr/bin/env python3
"""Build an additive, offline-only Apple Silicon filter test handoff. No installer."""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def identity(tool):
    data = json.loads(subprocess.check_output([str(Path(tool).resolve()), "--identity"], text=True))
    if not data.get("synthVersion") or not data.get("evaluationBuild"):
        raise ValueError("Tool has no build identity")
    return data


def make(output, component, perf, native, spectral):
    output = Path(output).resolve(); component = Path(component).resolve()
    info = plistlib.loads((component/"Contents/Info.plist").read_bytes())
    if info.get("CFBundleIdentifier") != "com.nitride.audio.nitride": raise ValueError("Not a Nitride component")
    a, b, c = identity(perf), identity(native), identity(spectral)
    if any((a["synthVersion"], a["evaluationBuild"]) != (x["synthVersion"], x["evaluationBuild"]) for x in (b, c)) or a["synthVersion"] != info["CFBundleShortVersionString"]:
        raise ValueError("Component/tool build identities differ")
    if a.get("configuration") != "Release": raise ValueError("Qualification requires Release tools")
    if a["evaluationBuild"].encode() not in (component/"Contents/MacOS/nitride").read_bytes():
        raise ValueError("Component does not contain the claimed evaluation identity")
    output.mkdir(parents=True, exist_ok=False)
    payload = output/"payload"; payload.mkdir(); tools = output/"tools"; tools.mkdir()
    shutil.copytree(component, payload/"nitride.component", symlinks=True)
    for name, source in (("nitride_perf", perf), ("nitride_au_presets", native), ("nitride_rage_spectral_tests", spectral)):
        shutil.copy2(source, tools/name)
    if any(p.is_symlink() for p in output.rglob("*")): raise ValueError("Linked package resources need explicit review")
    binaries = [payload/"nitride.component/Contents/MacOS/nitride", tools/"nitride_perf", tools/"nitride_au_presets", tools/"nitride_rage_spectral_tests"]
    for binary in binaries:
        if subprocess.check_output(["lipo", "-archs", str(binary)], text=True).strip() != "arm64": raise ValueError("Expected Apple Silicon binaries")
        libraries = subprocess.check_output(["otool", "-L", str(binary)], text=True).splitlines()[1:]
        if any(not row.strip().split()[0].startswith(("/System/Library/", "/usr/lib/")) for row in libraries):
            raise ValueError("Unpackaged non-system dependency")
    for target in (payload/"nitride.component", tools/"nitride_perf", tools/"nitride_au_presets", tools/"nitride_rage_spectral_tests"):
        subprocess.run(["codesign", "--force", "--sign", "-", "--timestamp=none", str(target)], check=True, capture_output=True)
        subprocess.run(["codesign", "--verify", "--strict", str(target)], check=True, capture_output=True)
    shutil.copy2(ROOT/"scripts/filter_qualification_run.zsh", tools/"run.zsh")
    shutil.copy2(ROOT/"docs/FILTER_QUALIFICATION_MANUAL.md", output/"MANUAL-CHECKLIST.md")
    command = output/"Run Offline Checks.command"
    command.write_text('#!/bin/zsh\nset -euo pipefail\nROOT=${0:A:h}\nexec /bin/zsh "$ROOT/tools/run.zsh" "$ROOT"\n')
    command.chmod(0o755)
    candidate = {"format": "nitride-filter-qualification-handoff", "version": 1,
                 "synthVersion": a["synthVersion"], "evaluationBuild": a["evaluationBuild"],
                 "architecture": "arm64", "minimum_binary_macos": "15.0", "signed": "ad-hoc-internal-only", "notarized": False,
                 "installation": "none; explicit private AU loading only", "preset_files_installed": 0,
                 "external_checks": {"m2": "pending-actual-execution", "logic_reopen": "manual-pending", "logic_soak": "manual-pending"},
                 "privacy": "No projects, settings, audio, private evaluation journal, telemetry or network access packaged/requested.",
                 "artifact_hashes": {str(p.relative_to(output)): digest(p) for p in sorted(output.rglob("*")) if p.is_file()}}
    (output/"candidate.json").write_text(json.dumps(candidate, indent=2)+"\n")
    (output/"README.txt").write_text(
        "INTERNAL Apple Silicon filter qualification, not a notarized public release.\n"
        "Read MANUAL-CHECKLIST.md. Run Offline Checks.command loads the bundled AU privately; it does NOT install or replace anything.\n"
        "No compiler/Python is required on the test Mac. Results are metrics/logs plus one generated probe state, not Logic projects/settings/audio.\n"
        "Offline completion does NOT qualify Logic reopening, audio-device dropouts or a 30-minute real-time Logic soak.\n"
        "Report Gatekeeper blocks; do not disable Gatekeeper globally. No automatic upload or network access.\n")
    (output/"checksums.sha256").write_text("".join(digest(p)+"  "+str(p.relative_to(output))+"\n" for p in sorted(output.rglob("*")) if p.is_file()))
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--build", type=Path, default=ROOT/"build/local-release")
    args = parser.parse_args(); build = args.build.resolve()
    print(make(args.output, build/"nitride_artefacts/Release/AU/nitride.component",
               build/"nitride_perf_artefacts/Release/nitride_perf", build/"nitride_au_presets", build/"nitride_rage_spectral_tests"))


if __name__ == "__main__": main()
