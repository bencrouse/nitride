"""Append-only Ripple studies, v9 native state and additive publication."""
import hashlib
import importlib.util
import json
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import evaluation as ev
import factory_presets as fp
import study_presets as study
import ripple_qualification as q
import ripple_audition_comparisons as contrast
import ripple_gain_comparisons as gain_audit
from test_factory_presets import synthetic_native

BATCH = fp.ROOT / "presets/review/batch-23/r1"
spec = importlib.util.spec_from_file_location("ripple_recipes", BATCH / "make_ripple_studies.py")
recipes = importlib.util.module_from_spec(spec); spec.loader.exec_module(recipes)


class RippleStudies(unittest.TestCase):
    def test_gain_calibration_is_control_only_symmetric_and_bounded(self):
        values = {"ripple_mode": 0, "ripple_dry": 0., "ripple_drive": 0., "ripple_feedback": 0.}
        self.assertEqual(gain_audit.expected_gain(values), 1.)
        values["ripple_mode"] = 1
        for amount in (0., .5, .9, 1.):
            values["ripple_feedback"] = amount; positive = gain_audit.expected_gain(values)
            values["ripple_feedback"] = -amount; self.assertEqual(gain_audit.expected_gain(values), positive)
            self.assertTrue(1 <= positive <= 4)
        self.assertEqual(gain_audit.expected_gain(values), 4.)

    def test_gain_audit_recovers_pre_ceiling_color_without_loudness_locking(self):
        import math
        def samples(makeup):
            return [int(8388608*.5*2*.8*math.tanh(makeup*(.05*math.sin(n*.073)+.025*math.sin(n*.171)))) for n in range(12000) for _ in range(2)]
        a = gain_audit.undo_existing_ceiling(samples(1.), 20*math.log10(.5))
        b = gain_audit.undo_existing_ceiling(samples(5.), 20*math.log10(.5))
        error = sum((y-5*x)**2 for x, y in zip(a, b))/sum(y*y for y in b)
        self.assertLess(10*math.log10(error), -90)
        with self.assertRaises(ValueError): gain_audit.undo_existing_ceiling([0]*100, 0)
        with self.assertRaises(ValueError): gain_audit.undo_existing_ceiling([8388607]*12000, 0)

    def test_gain_headroom_and_reference_failures_are_not_waived(self):
        rows = [{"preset": str(i), "peak_headroom_dbfs": 6., "corrected_vs_unity_reference_db": -2.} for i in range(6)]
        gain_audit.require_levels(rows, True)
        rows[0]["peak_headroom_dbfs"] = 2.9
        with self.assertRaises(ValueError): gain_audit.require_levels(rows, True)
        rows[0]["peak_headroom_dbfs"] = 6.; rows[0]["corrected_vs_unity_reference_db"] = -6.1
        with self.assertRaises(ValueError): gain_audit.require_levels(rows, True)

    def test_gain_reference_keeps_output_and_frozen_source_settings(self):
        with tempfile.TemporaryDirectory(prefix="nitride-ripple-gain-") as folder:
            destination = Path(folder)/"reference"; batch = BATCH.parent/"r2"
            gain_audit.stage_reference(batch, destination)
            with self.assertRaises(FileExistsError): gain_audit.stage_reference(batch, destination)
            for sketch in recipes.sketches():
                stem = sketch["family"]+"-r2"
                source = json.loads((batch/(stem+".nitridepreset")).read_text())
                reference = json.loads((destination/(stem+".nitridepreset")).read_text())
                self.assertEqual(source["values"]["output"], reference["values"]["output"])
                for key in source["values"]:
                    if key.startswith(("fm_", "amp_")): self.assertEqual(source["values"][key], reference["values"][key])
                self.assertEqual((batch/(stem+".mid")).read_bytes(), (destination/(stem+".mid")).read_bytes())

    def test_tonal_contrast_rejects_gain_and_phase_only_changes(self):
        import math
        def signal(phase=0., gain=1., extra=0.):
            return [int(8388608*gain*(.25*math.sin(2*math.pi*17*n/4096+phase)+extra*math.sin(2*math.pi*51*n/4096))) for n in range(8192) for _ in range(2)]
        a = contrast.magnitude_profile(signal())
        b = contrast.magnitude_profile(signal(phase=1., gain=.5))
        c = contrast.magnitude_profile(signal(extra=.15))
        self.assertLess(contrast.audio.residual(a, b), -60)
        self.assertGreater(contrast.audio.residual(a, c), -16)
        rows = [{"family": str(i), "r1_spectral_residual_db": -30., "r2_spectral_residual_db": -12.} for i in range(6)]
        contrast.require_contrast(rows)
        rows[0]["r2_spectral_residual_db"] = -29.
        with self.assertRaises(ValueError): contrast.require_contrast(rows)

    def test_r2_comparison_staging_keeps_source_and_midi(self):
        with tempfile.TemporaryDirectory(prefix="nitride-ripple-r2-") as folder:
            destination = Path(folder) / "inputs"; contrast.stage(BATCH.parent / "r2", destination)
            with self.assertRaises(FileExistsError): contrast.stage(BATCH.parent / "r2", destination)
            for sketch in recipes.sketches():
                family = sketch["family"]
                self.assertEqual((destination / "r1" / (family + ".mid")).read_bytes(), (destination / "r2" / (family + ".mid")).read_bytes())
                expected = json.loads((BATCH.parent / "r2" / (family + "-r2.nitridepreset")).read_text())
                self.assertEqual(json.loads((destination / "r2" / (family + ".nitridepreset")).read_text()), expected)

    def test_r2_preserves_every_historical_entry_and_original_source(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:134])).hexdigest(), "239de27349be243cefe9d229289b7fe549e35e73b858a890515e0d88d7e756e7")
        self.assertEqual([e["program"] for e in registry[134:140]], list(range(138, 144)))
        allowed = {"cutoff", "resonance", "ripple_dry", "ripple_drive", "ripple_feedback", "filter_amount", "lfo_cutoff_rate", "lfo_cutoff_depth", "output"}
        for entry in registry[134:140]:
            old = BATCH / (entry["family"] + "-r1.nitridepreset"); new = fp.ROOT / "presets/review" / entry["preset"]
            a, b = json.loads(old.read_text()), json.loads(new.read_text())
            self.assertEqual(entry["parent"], "review:" + entry["family"] + ":r1")
            self.assertNotEqual(a["id"], b["id"]); self.assertEqual(a["version"], b["version"])
            self.assertEqual((a["mono"], a["chord"]), (b["mono"], b["chord"]))
            self.assertEqual(set(a["values"]), set(b["values"]))
            self.assertTrue({k for k in a["values"] if a["values"][k] != b["values"][k]} <= allowed)
            self.assertLessEqual(b["values"]["cutoff"], 600); self.assertLessEqual(b["values"]["resonance"], .18)
            self.assertEqual(old.with_suffix(".mid").read_bytes(), new.with_suffix(".mid").read_bytes())
        _, selected = fp.validate_selection()
        self.assertFalse({e["id"] for e in registry[134:140]} & {e["id"] for e in selected})

    def test_r2_generator_verifies_and_refuses_overwrite(self):
        command = [sys.executable, str(BATCH.parent / "r2/make_ripple_r2.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr); self.assertEqual(result.stdout.count("Verified "), 13)
        self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)

    def test_complete_performance_matrix_retains_failures(self):
        rows = []
        for t in ("subtle", "medium", "hard"):
            for m in ("shape", "negative", "positive"):
                for v in (1, 4, 12):
                    for rate in (48000, 96000):
                        for block in (64, 256):
                            rows.append({"key": f"ripple-{t}-{m}{v}/{rate}/{block}", "sampleRate": rate, "blockSize": block, "heldNotes": v,
                                         "minimumVoices": v, "maximumVoices": v, "budgetMicros": 1000., "p99Micros": 700., "medianPassLoadPercent": 60., "overBudgetBlocks": 0, "numericalFaults": 0})
        report = {"format": "nitride-performance", "settings": {"passes": 3, "seconds": 2, "warmup": .5}, "machine": {"configuration": "Release", "cpu": "test-not-M2"}, "cases": rows}
        self.assertTrue(q.performance(report)["all_offline_headroom_gates_pass"])
        rows[0]["overBudgetBlocks"] = 1; self.assertFalse(q.performance(report)["all_offline_headroom_gates_pass"])
        rows[0]["overBudgetBlocks"] = 0; rows[0]["p99Micros"] = 801; self.assertFalse(q.performance(report)["all_offline_headroom_gates_pass"])
        with self.assertRaises(ValueError): q.performance({**report, "cases": rows[:-1]})
        rows[0]["numericalFaults"] = 1
        with self.assertRaises(ValueError): q.performance(report)

    def test_staging_is_exact_except_explicit_filter_controls(self):
        with tempfile.TemporaryDirectory(prefix="nitride-ripple-stage-") as folder:
            output = Path(folder) / "inputs"; q.stage(BATCH, output)
            with self.assertRaises(FileExistsError): q.stage(BATCH, output)
            for sketch in recipes.sketches():
                stem = sketch["family"] + "-r1"; source = json.loads((BATCH / (stem + ".nitridepreset")).read_text())
                for mode in q.MODES:
                    candidate = json.loads((output / mode / (stem + ".nitridepreset")).read_text())
                    self.assertEqual((BATCH / (stem + ".mid")).read_bytes(), (output / mode / (stem + ".mid")).read_bytes())
                    if mode == "ripple": self.assertEqual(candidate, source)
                    elif mode == "linear":
                        changed = {k for k, v in source["values"].items() if candidate["values"][k] != v}
                        self.assertTrue(changed <= {"ripple_mode", "ripple_drive", "ripple_feedback"})
                    else:
                        self.assertEqual(candidate["version"], 6)
                        self.assertEqual(candidate["values"], {k: v for k, v in source["values"].items() if not k.startswith(("ripple_", "rage_")) and k != "filter_family"})

    def test_missing_evidence_never_means_qualified(self):
        with tempfile.TemporaryDirectory(prefix="nitride-ripple-missing-") as folder:
            result = q.summarize(folder)
            self.assertFalse(result["release_qualified"]); self.assertEqual(result["m2"], "unverified")
            self.assertEqual(result["logic_device_soak"], "manual-pending")
            self.assertEqual(result["performance"]["status"], "missing-not-qualified")

    def test_historical_programs_and_factory_selection_unchanged(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:128])).hexdigest(), "100978f84d9e2955a7f83ff6855fc0efbe412dc77360d53af2310cf42e98fb70")
        self.assertEqual([e["program"] for e in registry[128:134]], list(range(132, 138)))
        _, selected = fp.validate_selection(); self.assertEqual(len(selected), 77)
        self.assertFalse({e["id"] for e in registry[128:134]} & {e["id"] for e in selected})

    def test_frozen_recipe_midi_and_six_type_modes(self):
        command = [sys.executable, str(BATCH / "make_ripple_studies.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr); self.assertEqual(result.stdout.count("Verified "), 13)
        self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
        combinations = set()
        for sketch in recipes.sketches():
            source = json.loads((BATCH / (sketch["family"] + "-r1.nitridepreset")).read_text())
            self.assertEqual(source["version"], 8); self.assertEqual(len(source["values"]), 97)
            self.assertEqual(source["values"]["ripple_enabled"], 1)
            self.assertEqual(source["values"]["filter_family"], 0)
            combinations.add((source["values"]["ripple_type"], source["values"]["ripple_mode"]))
        self.assertEqual(combinations, {(t, m) for t in range(3) for m in range(2)})

    def test_v9_native_and_no_overwrite_publication(self):
        sketches = recipes.sketches()
        definitions = {132+i: json.loads((BATCH / (s["family"] + "-r1.nitridepreset")).read_text()) for i, s in enumerate(sketches)}
        with tempfile.TemporaryDirectory(prefix="nitride-ripple-test-") as folder:
            root = Path(folder).resolve(); component = root / "component"; tool = root / "tool"
            build = {"id": "test", "version": "0.7.47", "installed_path": str(component), "binary_sha256": "test", "bundle_hashes": {}}
            def native(command, chosen_tool, chosen_component):
                self.assertEqual((chosen_tool, chosen_component), (tool, component))
                if command[0] == "export":
                    program = int(command[1]); entry = {"id": "review:" + sketches[program-132]["family"] + ":r1", "program": program, "name": command[3]}
                    Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(definitions[program], entry))); return "synthetic only"
                return json.dumps({"full_document_exact": True, "audio_exact": True, "parameters": 98})
            package = root / "package"
            with mock.patch.object(ev, "read_events", side_effect=AssertionError("private journal accessed")), mock.patch.object(fp, "component_build", return_value=build), mock.patch.object(fp, "native", side_effect=native):
                manifest = study.export(package, BATCH, None, "ripple-001", component, tool)
            self.assertEqual(len(manifest["entries"]), 6)
            destination = root / "installed"
            for entry in manifest["entries"]: (destination / entry["category"]).mkdir(parents=True, exist_ok=True)
            protected = destination / "Pads/User sound.aupreset"; protected.write_bytes(b"user")
            self.assertEqual(study.install(package, destination), study.install(package, destination))
            self.assertEqual(protected.read_bytes(), b"user")
            edited = destination / manifest["entries"][0]["file"]; edited.write_bytes(b"edited")
            with self.assertRaisesRegex(ValueError, "never overwrite"): study.install(package, destination)


if __name__ == "__main__": unittest.main()
