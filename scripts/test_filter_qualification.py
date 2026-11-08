import json
from pathlib import Path
import plistlib
import tempfile
import unittest
from unittest import mock

import filter_qualification as q
import package_filter_qualification as p


def performance(cpu="Apple M4 Max"):
    rows = []
    for key in sorted(q.expected_cases()):
        name, rate, block = key.split("/"); voices = int(__import__("re").search(r"\d+$", name).group())
        rows.append({"key": key, "sampleRate": int(rate), "blockSize": int(block), "heldNotes": voices,
                     "minimumVoices": voices, "maximumVoices": voices, "budgetMicros": 1000,
                     "p99Micros": 700, "medianPassLoadPercent": 60, "overBudgetBlocks": 0})
    return {"format": "nitride-performance", "settings": {"passes": 3, "seconds": 2, "warmup": .5},
            "machine": {"configuration": "Release", "cpu": cpu}, "cases": rows}


def spectral():
    return {"format": "nitride-rage-spectral-probes", "version": 2, "results": [
        {"sampleRate": rate, "character": c, "probe": probe, "resonance": resonance,
         "maximumSolverResidual": 1e-12, "reference32xMaximumSolverResidual": 1e-12, "reference32xResidualDb": -50,
         "nonHarmonicEnergyDb": -70, "reference32xNonHarmonicEnergyDb": -75}
        for rate in (48000, 96000) for c in range(5)
        for probe, resonance in (("low-tone", .85), ("high-tone", .85), ("cutoff-lfo-and-audio-fm", .85), ("low-tone", .7), ("high-tone", .7))]}


def continuity():
    return {"format": "nitride-offline-continuity-soak", "elapsedWallSeconds": 500, "settings": {"passes": 1, "seconds": 1800, "soak": True},
            "cases": [{"key": "rage-diode4/48000/256", "sampleRate": 48000, "blockSize": 256, "heldNotes": 4,
                       "audioSeconds": 1800, "stateRoundTrips": 61, "numericalFaults": 0, "peak": .3, "minimumVoices": 4, "maximumVoices": 4,
                       "overBudgetBlocks": 0, "maxMicros": 1500}]}


class FilterQualificationTests(unittest.TestCase):
    def test_complete_matrix_and_both_headroom_conditions_required(self):
        report = performance(); self.assertTrue(q.performance(report)["all_offline_headroom_gates_pass"])
        report["cases"][0]["overBudgetBlocks"] = 1
        self.assertFalse(q.performance(report)["all_offline_headroom_gates_pass"])
        report["cases"][0]["overBudgetBlocks"] = 0; report["cases"][0]["p99Micros"] = 801
        self.assertFalse(q.performance(report)["all_offline_headroom_gates_pass"])
        with self.assertRaises(ValueError): q.performance({**report, "cases": report["cases"][:-1]})

    def test_wrong_clock_voices_or_nonfinite_timing_rejected(self):
        for key, value in (("sampleRate", 44100), ("minimumVoices", 0), ("p99Micros", float("nan"))):
            report = performance(); report["cases"][0][key] = value
            with self.assertRaises(ValueError): q.performance(report)

    def test_m2_is_reported_hardware_not_package_name(self):
        self.assertFalse(q.performance(performance())["m2_hardware_reported"])
        self.assertTrue(q.performance(performance("Apple M2 Max"))["m2_hardware_reported"])
        self.assertFalse(q.performance(performance("Apple M2 emulator"))["m2_hardware_reported"])

    def test_artifact_classification_never_means_clean_or_alias_free(self):
        report = spectral(); report["results"][0].update(nonHarmonicEnergyDb=-9, reference32xNonHarmonicEnergyDb=-9.2, reference32xResidualDb=-4)
        result = q.artifacts(report)
        self.assertTrue(result["numerical_guards_pass"]); self.assertFalse(result["spectral_cleanliness_qualified"])
        self.assertEqual(result["cases"][0]["classification"], "strong-nonharmonic-also-in-reference")
        self.assertTrue(result["cases"][0]["large_reference_residual"])

    def test_missing_spectral_rows_and_bad_solves_rejected(self):
        report = spectral(); report["results"][0]["maximumSolverResidual"] = 1e-3
        with self.assertRaises(ValueError): q.artifacts(report)
        with self.assertRaises(ValueError): q.artifacts({**report, "results": report["results"][:-1]})

    def test_continuity_does_not_qualify_logic(self):
        result = q.continuity(continuity()); self.assertTrue(result["passed_offline_continuity"])
        report = continuity(); report["cases"][0]["overBudgetBlocks"] = 21; report["cases"][0]["maxMicros"] = 83e6
        result = q.continuity(report)
        self.assertTrue(result["passed_offline_continuity"]); self.assertFalse(result["passed_offline_deadline_gate"])
        self.assertEqual(result["overdue_blocks"], 21); self.assertEqual(result["maximum_block_micros"], 83e6)
        for key, value in (("audioSeconds", 1799), ("audioSeconds", float("nan")), ("sampleRate", 96000), ("stateRoundTrips", 59), ("numericalFaults", 1), ("minimumVoices", 0)):
            report = continuity(); report["cases"][0][key] = value
            with self.assertRaises(ValueError): q.continuity(report)

    def test_missing_native_raw_and_manual_evidence_never_become_passes(self):
        with tempfile.TemporaryDirectory(prefix="nitride-filter-qual-") as folder:
            root = Path(folder); (root/"native-reopen.status").write_text("PASS\n")
            result = q.summarize(root)
            self.assertEqual(result["performance"]["status"], "missing-not-qualified")
            self.assertEqual(result["native_process_reopen"], "missing-or-failed-not-qualified")
            self.assertEqual(result["logic_project_reopen"], "manual-pending"); self.assertEqual(result["logic_soak"], "manual-pending")
            self.assertFalse(result["release_qualified"])

    def test_mixed_or_missing_evidence_builds_rejected(self):
        with tempfile.TemporaryDirectory(prefix="nitride-filter-qual-") as folder:
            root = Path(folder); report = performance()
            (root/"performance.json").write_text(json.dumps(report))
            with self.assertRaisesRegex(ValueError, "identity"): q.summarize(root)
            report.update(synthVersion="0.7.44", evaluationBuild="eval-044")
            (root/"performance.json").write_text(json.dumps(report))
            (root/"candidate.json").write_text(json.dumps({"synthVersion": "0.7.43", "evaluationBuild": "eval-043"}))
            with self.assertRaisesRegex(ValueError, "identities"): q.summarize(root)


class HandoffTests(unittest.TestCase):
    def fixture(self, root):
        component = root/"component"; (component/"Contents/MacOS").mkdir(parents=True)
        (component/"Contents/Info.plist").write_bytes(plistlib.dumps({"CFBundleIdentifier": "com.nitride.audio.nitride", "CFBundleShortVersionString": "0.7.44"}))
        (component/"Contents/MacOS/nitride").write_bytes(b"fake binary eval-044")
        binaries = []
        for name in ("perf", "native", "spectral"):
            path = root/name; path.write_bytes(b"fake"); binaries.append(path)
        return component, binaries

    def test_handoff_is_additive_no_installer_or_private_assets(self):
        with tempfile.TemporaryDirectory(prefix="nitride-filter-handoff-") as folder:
            root = Path(folder); component, binaries = self.fixture(root); output = root/"handoff"
            ident = {"synthVersion": "0.7.44", "evaluationBuild": "eval-044", "configuration": "Release"}
            def command(args, **kwargs): return "arm64\n" if args[0] == "lipo" else "binary:\n\t/usr/lib/libSystem.B.dylib (compatibility version 1.0.0)\n"
            with mock.patch.object(p, "identity", return_value=ident), mock.patch.object(p.subprocess, "check_output", side_effect=command), mock.patch.object(p.subprocess, "run"):
                p.make(output, component, *binaries)
                with self.assertRaises(FileExistsError): p.make(output, component, *binaries)
            data = json.loads((output/"candidate.json").read_text())
            self.assertEqual(data["preset_files_installed"], 0); self.assertEqual(data["external_checks"]["logic_soak"], "manual-pending")
            self.assertFalse((output/"Install.command").exists()); self.assertFalse((output/"evaluation").exists())
            self.assertTrue((output/"checksums.sha256").is_file()); self.assertTrue((output/"tools/nitride_rage_spectral_tests").is_file())

    def test_mixed_build_identities_rejected_before_packaging(self):
        with tempfile.TemporaryDirectory(prefix="nitride-filter-handoff-") as folder:
            root = Path(folder); component, binaries = self.fixture(root)
            a = {"synthVersion": "0.7.44", "evaluationBuild": "eval-044", "configuration": "Release"}
            b = {**a, "evaluationBuild": "eval-043"}
            with mock.patch.object(p, "identity", side_effect=[a, b, a]):
                with self.assertRaisesRegex(ValueError, "identities"): p.make(root/"output", component, *binaries)
            self.assertFalse((root/"output").exists())


if __name__ == "__main__": unittest.main()
