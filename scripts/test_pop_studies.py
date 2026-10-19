#!/usr/bin/env python3
"""Original pop-brief lineage and safe native study publication; no listening verdicts."""
import importlib.util
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import evaluation as ev
import factory_presets as fp
import study_presets as study
from test_factory_presets import synthetic_native

BATCH = fp.ROOT / "presets/review/batch-17/r1"
spec = importlib.util.spec_from_file_location("pop_study_recipes", BATCH / "make_pop_studies.py")
recipes = importlib.util.module_from_spec(spec); spec.loader.exec_module(recipes)


class PopStudyTests(unittest.TestCase):
    def test_three_finalized_roles_preserve_programs_and_enter_factory_selection(self):
        entries = [e for e in ev.validate_builtins()["entries"] if e["preset"].startswith("batch-17/")]
        self.assertEqual([e["program"] for e in entries], [63, 64, 65])
        self.assertEqual([e["family"] for e in entries], ["plastic-hero", "budget-heaven", "sugar-teeth"])
        self.assertTrue(all("imagined taste brief" in e["design_lens"] for e in entries))
        _, selected = fp.validate_selection(); self.assertEqual(len(selected), 53)
        self.assertTrue({e["id"] for e in entries} <= {e["id"] for e in selected})

    def test_frozen_generator_and_contrasting_controls(self):
        result = subprocess.run([sys.executable, str(BATCH / "make_pop_studies.py"), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr); self.assertEqual(result.stdout.count("Verified "), 7)
        result = subprocess.run([sys.executable, str(BATCH / "make_pop_studies.py")], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0); self.assertIn("never replace", result.stderr)
        definitions = [json.loads((BATCH / (s["family"] + "-r1.nitridepreset")).read_text()) for s in recipes.sketches()]
        hero, heaven, teeth = [d["values"] for d in definitions]
        self.assertEqual([d["mono"] for d in definitions], [True, False, False])
        self.assertTrue(all(not d["chord"] for d in definitions))
        self.assertEqual((hero["glide_target"], hero["autobend_target"], hero["autobend_phrase"]), (2, 2, 1))
        self.assertLess(abs(hero["autobend_depth"]), 2); self.assertLess(hero["glide_time"], .06)
        self.assertGreater(heaven["amp_sustain"], .9); self.assertGreater(heaven["cutoff"], 10000)
        self.assertGreater(heaven["fm_operator_mix"], .3); self.assertEqual(heaven["fm_envelope_amount"], 0)
        self.assertEqual(teeth["fm_ratio"], 1); self.assertLess(teeth["fm_decay"], .025)
        self.assertGreater(teeth["fm_envelope_amount"], 3); self.assertGreater(teeth["filter_velocity"], 1)
        for values in (hero, heaven, teeth):
            self.assertEqual(values["stress"], 0); self.assertEqual(values["filter_fm_depth"], 0)
            for n in (1, 2, 3): self.assertEqual(values[f"fm_route_{n}_source"], 0)
            for lane in ("pitch", "coupling", "stress", "fm", "cutoff", "amplitude"):
                self.assertEqual(values[f"lfo_{lane}_sync"], 0)

    def package(self, root):
        package = root / "package"; package.mkdir(); destination = root / "destination"; destination.mkdir()
        manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": "study",
                    "status": "awaiting-logic-category-confirmation", "study": "ag-cook-001", "trial": "unit-test",
                    "selection": "not-factory-selected", "installed_build": "unit-test", "entries": [], "files": {}}
        for i, sketch in enumerate(recipes.sketches()):
            source = BATCH / (sketch["family"] + "-r1.nitridepreset"); definition = json.loads(source.read_text())
            relative = sketch["host_category"] + "/Study - " + sketch["name"] + ".aupreset"
            path = package / relative; path.parent.mkdir(exist_ok=True); (destination / sketch["host_category"]).mkdir(exist_ok=True)
            frozen = "sources/" + source.name; (package / "sources").mkdir(exist_ok=True); shutil.copy2(source, package / frozen)
            entry = {"id": "review:" + sketch["family"] + ":r1", "program": 63 + i, "name": "Study - " + sketch["name"],
                     "category": sketch["host_category"], "file": relative, "frozen_source": frozen, "source_sha256": ev.digest(source),
                     "native_verification": {"full_document_exact": True, "audio_exact": True, "parameters": 89}}
            path.write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
            manifest["entries"].append(entry); manifest["files"][relative] = ev.digest(path); manifest["files"][frozen] = ev.digest(package / frozen)
        (package / "manifest.json").write_text(json.dumps(manifest))
        return package, destination, manifest

    def test_install_is_additive_receipted_and_idempotent(self):
        with tempfile.TemporaryDirectory() as folder:
            package, destination, manifest = self.package(Path(folder).resolve())
            factory = destination / fp.FACTORY_RECEIPT; factory.write_bytes(b"unchanged factory ownership")
            user = destination / "Pads/My setting.aupreset"; user.write_bytes(b"user")
            first = study.install(package, destination); self.assertEqual(first, study.install(package, destination))
            self.assertEqual(factory.read_bytes(), b"unchanged factory ownership"); self.assertEqual(user.read_bytes(), b"user")
            for entry in manifest["entries"]: self.assertEqual(ev.digest(destination / entry["file"]), manifest["files"][entry["file"]])

    def test_collision_and_tamper_are_refused_before_publication(self):
        with tempfile.TemporaryDirectory() as folder:
            package, destination, manifest = self.package(Path(folder).resolve())
            path = destination / manifest["entries"][1]["file"]; path.write_bytes(b"user")
            with self.assertRaisesRegex(ValueError, "collision"): study.install(package, destination)
            self.assertEqual(path.read_bytes(), b"user"); path.unlink()
            (package / manifest["entries"][0]["file"]).write_bytes(b"tampered")
            with self.assertRaisesRegex(ValueError, "checksum"): study.install(package, destination)
            self.assertFalse(list(destination.rglob("*.aupreset")))

    def test_receipt_publication_failure_rolls_back_only_new_files(self):
        with tempfile.TemporaryDirectory() as folder:
            package, destination, manifest = self.package(Path(folder).resolve())
            user = destination / "Pads/My setting.aupreset"; user.write_bytes(b"user")
            real = study.os.link; calls = 0
            def fail_receipt(source, target):
                nonlocal calls
                calls += 1
                if calls == 4: raise OSError("injected receipt failure")
                real(source, target)
            with mock.patch.object(study.os, "link", side_effect=fail_receipt):
                with self.assertRaisesRegex(OSError, "injected"): study.install(package, destination)
            self.assertEqual(user.read_bytes(), b"user")
            self.assertTrue(all(not (destination / e["file"]).exists() for e in manifest["entries"]))
            self.assertFalse((destination / ".nitride-study-ag-cook-001.json").exists())


if __name__ == "__main__": unittest.main()
