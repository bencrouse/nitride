#!/usr/bin/env python3
"""Explicit, local-only validation. Never regenerates approved fixtures."""
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def run(command, *, log=None, env=None):
    print("+ " + " ".join(map(str, command)), flush=True)
    if log:
        with log.open("w") as stream:
            result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, env=env)
        if result.returncode:
            print(log.read_text(), flush=True)
            raise subprocess.CalledProcessError(result.returncode, command)
    else:
        subprocess.run(command, cwd=ROOT, check=True, env=env)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--full", action="store_true")
    parser.add_argument("--profile", choices=["debug", "release", "asan", "tsan"])
    parser.add_argument("--resume-from", choices=["debug", "release", "asan", "tsan"], help="Resume a full check without cleaning completed build trees")
    args = parser.parse_args()
    reports = ROOT / "build" / "checks"
    reports.mkdir(parents=True, exist_ok=True)
    metadata: dict[str, object] = {"timestamp": time.strftime("%Y-%m-%dT%H:%M:%S%z"), "machine": platform.platform(), "architecture": platform.machine()}
    for name, command in {"clang": ["clang", "--version"], "cmake": ["cmake", "--version"], "sdk": ["xcrun", "--show-sdk-version"], "revision": ["git", "rev-parse", "HEAD"]}.items():
        metadata[name] = subprocess.check_output(command, cwd=ROOT, text=True).strip()
    profiles = [args.profile] if args.profile else (["debug", "release", "asan", "tsan"] if args.full else ["debug"])
    if args.resume_from:
        if not args.full:
            parser.error("--resume-from requires --full")
        profiles = profiles[profiles.index(args.resume_from):]
    for profile in profiles:
        preset = "local-" + profile
        directory = ROOT / "build" / preset
        if args.full and not args.resume_from and directory.exists():
            shutil.rmtree(directory)
        run(["cmake", "--preset", preset], log=reports / (profile + "-configure.log"))
        targets = ["nitride_safety_tests"] if profile in ("asan", "tsan") else ["nitride", "nitride_ui_tests", "nitride_automation_tests", "nitride_pitch_tests", "nitride_reference_tests", "nitride_study_tests", "nitride_safety_tests"]
        if profile == "release" or (args.full and profile == "debug"):
            targets += ["nitride_AU", "nitride_Standalone", "nitride_review", "nitride_perf", "nitride_au_component_tests"]
        run(["cmake", "--build", "--preset", preset, "--target", *targets], log=reports / (profile + "-build.log"))
        env = os.environ.copy()
        if profile == "asan":
            env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
        if profile == "tsan":
            env["TSAN_OPTIONS"] = "halt_on_error=1"
        selection = ["-R", "nitride_safety_tests"] if profile in ("asan", "tsan") else []
        run(["ctest", "--preset", preset, *selection], log=reports / (profile + "-tests.log"), env=env)
        if profile == "release":
            benchmark = directory / "nitride_perf_artefacts/Release/nitride_perf"
            for suffix, options in [("ordinary", []), ("pitch", ["--pitch-motion"]), ("filter", ["--filter-envelope"])]:
                run([str(benchmark), "--quick", *options, "--output", str(reports / (suffix + ".json"))], log=reports / (suffix + "-benchmark.log"))
            component_dir = Path.home() / "Library/Audio/Plug-Ins/Components"
            component_dir.mkdir(parents=True, exist_ok=True)
            component = component_dir / "nitride.component"
            # Replace only this instrument's component after the build completes.
            if component.exists():
                shutil.rmtree(component)
            shutil.copytree(directory / "nitride_artefacts/Release/AU/nitride.component", component)
            # Local ad-hoc sealing includes the bundle resources; it is not
            # distribution signing/notarization or a Developer ID requirement.
            run(["codesign", "--force", "--sign", "-", "--timestamp=none", str(component)], log=reports / "au-sign.log")
            run(["codesign", "--verify", "--deep", "--strict", str(component)], log=reports / "au-sign-verify.log")
            component.touch()
            gui = ["launchctl", "asuser", str(os.getuid())]
            run([*gui, "/usr/bin/auval", "-v", "aumu", "NT01", "NTRD"], log=reports / "auval.log")
            run([*gui, str(directory / "nitride_au_component_tests")], log=reports / "installed-au.log")
    metadata["profiles"] = profiles
    (reports / "environment.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print("Local checks passed. Logs and environment: " + str(reports))


if __name__ == "__main__":
    main()
