"""Append-only filter studies, schema boundaries and safe native publication."""
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
from test_factory_presets import synthetic_native

BATCH = fp.ROOT / "presets/review/batch-21/r1"
spec = importlib.util.spec_from_file_location("rage_recipes", BATCH / "make_rage_studies.py")
recipes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipes)


class RageStudies(unittest.TestCase):
    def test_append_only_and_not_factory_selected(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:102])).hexdigest(),
                         "17af28b2053573eb6d00113131b922487321cdffa1ef90f940f3e86ecf4574b7")
        self.assertEqual([e["program"] for e in registry[102:107]], list(range(106, 111)))
        _, selected = fp.validate_selection()
        self.assertEqual(len(selected), 77)
        self.assertFalse({e["id"] for e in registry[102:112]} & {e["id"] for e in selected})

    def test_frozen_generator_and_distinct_roles(self):
        command = [sys.executable, str(BATCH / "make_rage_studies.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 11)
        self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
        for i, sketch in enumerate(recipes.sketches()):
            definition = json.loads((BATCH / (sketch["family"] + "-r1.nitridepreset")).read_text())
            self.assertEqual(definition["version"], 7)
            self.assertEqual(set(definition["values"]), set(recipes.base.neutral()) | {"filter_family", "rage_character", "rage_amount"})
            self.assertEqual(definition["values"]["filter_family"], 1)
            self.assertEqual(definition["values"]["rage_character"], i)
            self.assertGreater(definition["values"]["resonance"], .5)
        self.assertEqual(len({s["host_category"] for s in recipes.sketches()}), 5)

    def test_native_v8_and_additive_publication_without_journal(self):
        sketches = recipes.sketches()
        definitions = {106+i: json.loads((BATCH / (s["family"] + "-r1.nitridepreset")).read_text()) for i, s in enumerate(sketches)}
        with tempfile.TemporaryDirectory(prefix="nitride-rage-test-") as folder:
            root = Path(folder).resolve(); component = root / "component"; tool = root / "tool"
            build = {"id": "test", "version": "0.7.39", "installed_path": str(component), "binary_sha256": "test", "bundle_hashes": {}}
            def native(command, chosen_tool, chosen_component):
                self.assertEqual((chosen_tool, chosen_component), (tool, component))
                if command[0] == "export":
                    program = int(command[1]); entry = {"id": "review:" + sketches[program-106]["family"] + ":r1", "program": program, "name": command[3]}
                    Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(definitions[program], entry)))
                    return "synthetic only"
                return json.dumps({"full_document_exact": True, "audio_exact": True, "parameters": 92})
            package = root / "package"
            with mock.patch.object(ev, "read_events", side_effect=AssertionError("private journal accessed")), \
                    mock.patch.object(fp, "component_build", return_value=build), mock.patch.object(fp, "native", side_effect=native):
                manifest = study.export(package, BATCH, None, "rage-characters-001", component, tool)
            destination = root / "installed"
            for e in manifest["entries"]: (destination / e["category"]).mkdir(parents=True, exist_ok=True)
            protected = destination / "Bass/User bass.aupreset"; protected.write_bytes(b"user edited")
            self.assertEqual(study.install(package, destination), study.install(package, destination))
            self.assertEqual(protected.read_bytes(), b"user edited")
            edited = destination / manifest["entries"][0]["file"]; edited.write_bytes(b"edited study")
            with self.assertRaisesRegex(ValueError, "never overwrite"): study.install(package, destination)
            self.assertEqual(edited.read_bytes(), b"edited study")

    def test_versioned_host_contract(self):
        self.assertEqual([fp.native_parameter_count(v) for v in (None, "unit-test", "0.7.38", "0.7.39", "0.8.0")], [89, 89, 89, 92, 92])

    def test_r2_append_only_and_frozen_source_midi(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:107])).hexdigest(),
                         "6aa0cc3c1e6a1b28ab995e49f2260ad2de87cbac4cc4cd150991205ff91ababc")
        self.assertEqual([e["program"] for e in registry[107:112]], list(range(111, 116)))
        command = [sys.executable, str(BATCH.parent / "r2/make_rage_r2.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 11)
        self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
        for entry in registry[107:112]:
            family = entry["family"]; old = BATCH / (family + "-r1.nitridepreset")
            new = fp.ROOT / "presets/review" / entry["preset"]
            a, b = json.loads(old.read_text()), json.loads(new.read_text())
            self.assertNotEqual(a["id"], b["id"])
            self.assertEqual(a["mono"], b["mono"])
            self.assertEqual(set(a["values"]), set(b["values"]))
            self.assertEqual(b["values"]["rage_amount"], 1)
            self.assertEqual(old.with_suffix(".mid").read_bytes(), new.with_suffix(".mid").read_bytes())
            self.assertEqual({key for key in a["values"] if a["values"][key] != b["values"][key]},
                             {"rage_amount", "cutoff", "resonance", "output"})


if __name__ == "__main__": unittest.main()
