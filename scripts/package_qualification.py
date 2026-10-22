#!/usr/bin/env python3
"""Make a pinned INTERNAL Apple Silicon test handoff; not a notarized release installer."""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import shutil
import subprocess

import evaluation as ev
import factory_presets as fp
from qualification import MATRIX

ROOT = Path(__file__).resolve().parents[1]


def digest(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def make(output, build, canonical):
    output, build, canonical = Path(output).resolve(), Path(build).resolve(), Path(canonical).resolve()
    manifest = fp.verify_package(canonical)
    if manifest["scope"] != "candidate" or len(manifest["entries"]) != 53: raise ValueError("Expected the finalized53sound package")
    output.mkdir(parents=True, exist_ok=False)
    payload = output / "payload"; payload.mkdir(); tools = output / "tools"; tools.mkdir()
    for source, target in ((build / "nitride_artefacts/Release/AU/nitride.component", payload / "nitride.component"),
                           (build / "nitride_artefacts/Release/Standalone/nitride.app", payload / "nitride.app")):
        shutil.copytree(source, target)
        if any(p.is_symlink() for p in target.rglob("*")): raise ValueError("Review linked bundle resources explicitly")
        info = plistlib.loads((target / "Contents/Info.plist").read_bytes())
        if info["CFBundleShortVersionString"] != "0.7.30": raise ValueError("Wrong qualification product version")
        subprocess.run(["codesign", "--force", "--sign", "-", "--timestamp=none", str(target)], check=True, capture_output=True)
        subprocess.run(["codesign", "--verify", "--strict", str(target)], check=True, capture_output=True)
    binaries = {"nitride_perf": build / "nitride_perf_artefacts/Release/nitride_perf",
                "nitride_socket_tests": build / "nitride_socket_tests_artefacts/Release/nitride_socket_tests",
                "nitride_au_component_tests": build / "nitride_au_component_tests", "nitride_au_presets": build / "nitride_au_presets"}
    for name, source in binaries.items():
        shutil.copy2(source, tools / name)
        subprocess.run(["codesign", "--force", "--sign", "-", "--timestamp=none", str(tools / name)], check=True, capture_output=True)
    for binary in [*(tools / name for name in binaries), payload / "nitride.component/Contents/MacOS/nitride", payload / "nitride.app/Contents/MacOS/nitride"]:
        if subprocess.check_output(["lipo", "-archs", str(binary)], text=True).strip() != "arm64": raise ValueError("Only Apple Silicon is qualified")
        libraries = subprocess.check_output(["otool", "-L", str(binary)], text=True).splitlines()[1:]
        if any(not line.strip().split()[0].startswith(("/System/Library/", "/usr/lib/")) for line in libraries):
            raise ValueError("Unpackaged non-system runtime dependency: " + str(binary))
    presets = payload / "presets"; presets.mkdir(); files = {}; native = []
    for entry in manifest["entries"]:
        source = fp.package_file(canonical, entry["file"]); target = presets / entry["file"]; target.parent.mkdir(exist_ok=True)
        shutil.copy2(source, target); files[entry["file"]] = digest(target)
        definition = json.loads(fp.package_file(canonical, entry["frozen_source"]).read_text())
        native.append("\t".join((entry["file"], str(entry["program"]), definition["name"])))
    (presets / fp.FACTORY_RECEIPT).write_text(json.dumps({"format": "nitride-factory-preset-install", "version": 1,
        "status": "organized-candidate-not-release-approved", "selection": manifest["selection"],
        "installed_build": "eval-030", "native_export_origin": manifest["installed_build"],
        "package_sha256": digest(canonical / "manifest.json"), "host_confirmation_feedback": fp.host_confirmation(), "files": files}, indent=2) + "\n")
    (output / "native.tsv").write_text("\n".join(native) + "\n")
    shutil.copy2(MATRIX, output / "matrix.tsv")
    shutil.copy2(ROOT / "docs/QUALIFICATION_MANUAL.md", output / "MANUAL-CHECKLIST.md")
    for source, target in (("qualification_install.zsh", "install.zsh"), ("qualification_run.zsh", "run.zsh")):
        shutil.copy2(ROOT / "scripts" / source, tools / target)
    for name, action in (("Install", "install"), ("Uninstall", "uninstall")):
        (output / (name + ".command")).write_text('#!/bin/zsh\nset -euo pipefail\nROOT=${0:A:h}\nexec /bin/zsh "$ROOT/tools/install.zsh" ' + action + ' "$ROOT"\n')
    (output / "Run Tests.command").write_text('#!/bin/zsh\nset -euo pipefail\nROOT=${0:A:h}\nexec /bin/zsh "$ROOT/tools/run.zsh" "$ROOT"\n')
    for path in output.glob("*.command"): path.chmod(0o755)
    inputs = output / "inputs"; inputs.mkdir()
    for name, relative in {"socket-120.mid": "batch-09/r1/buzz-loom-r1.mid", "socket-118.mid": "batch-08/r2/socket-dance-r2.mid",
                           "plastic-hero.mid": "batch-17/r1/plastic-hero-r1.mid", "budget-heaven.mid": "batch-17/r1/budget-heaven-r1.mid",
                           "reverse-veil.mid": "batch-13/r1/reverse-veil-r1.mid", "tine-keys.mid": "batch-09/r2/tine-keys-r2.mid"}.items():
        shutil.copy2(ROOT / "presets/review" / relative, inputs / name)
    rows = []
    for kind, folder in (("au", payload / "nitride.component"), ("app", payload / "nitride.app"), ("preset", presets)):
        for path in sorted(folder.rglob("*")):
            if path.is_file(): rows.append("\t".join((kind, str(path.relative_to(folder)), digest(path))))
    (output / "install.tsv").write_text("\n".join(rows) + "\n")
    source_files = [p for folder in ("Source", "cmake", "presets", "scripts") for p in (ROOT / folder).rglob("*") if p.is_file() and "__pycache__" not in p.parts]
    source_files += [ROOT / p for p in ("CMakeLists.txt", "CMakePresets.json", "Makefile", "docs/QUALIFICATION_MANUAL.md")]
    (output / "candidate.json").write_text(json.dumps({"format": "nitride-internal-qualification-candidate", "version": 1,
        "identity": "qualification-001", "synth_version": "0.7.30", "build": "eval-030", "architecture": "arm64",
        "minimum_binary_macos": "15.0 (not a qualification claim for macOS15)", "signed": "ad-hoc-internal-only", "notarized": False,
        "base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "source_hashes": {str(p.relative_to(ROOT)): digest(p) for p in sorted(source_files)},
        "artifact_hashes": {str(p.relative_to(output)): digest(p) for p in sorted(output.rglob("*")) if p.is_file()},
        "external_checks": {"m2": "pending-user-execution", "logic_reopen": "pending-manual", "logic_soak": "pending-manual"},
        "privacy": "No downloaded research engines/Yamaha data/private journal/projects/settings are packaged; no network/telemetry."}, indent=2) + "\n")
    (output / "README.txt").write_text("Nitride qualification001 / Apple Silicon / internal ad-hoc candidate, NOT notarized public release.\nRead MANUAL-CHECKLIST.md first. Fresh install only; no existing Nitride/user sound is overwritten.\nInstall.command, then close Logic and Run Tests.command; finish manual project-reopen/soak checks.\nNo compiler/Python or project upload required on the M2. Gatekeeper blocks must be reported, not bypassed globally.\n")
    (output / "checksums.sha256").write_text("".join(digest(p) + "  " + str(p.relative_to(output)) + "\n" for p in sorted(output.rglob("*")) if p.is_file()))
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("output", type=Path)
    parser.add_argument("--build", type=Path, default=ROOT / "build/local-release")
    parser.add_argument("--presets", type=Path, default=ROOT / "evaluation/preset-finalization/pass-005/package")
    args = parser.parse_args(); output = make(args.output, args.build, args.presets)
    print("Prepared internal qualification candidate:", output)


if __name__ == "__main__": main()
