#!/usr/bin/env python3
"""Motion/envelope recipes, immutable lineage and portable native-study export; no listening verdict."""
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

BATCH = fp.ROOT / "presets/review/batch-18/r1"
spec = importlib.util.spec_from_file_location("motion_recipes", BATCH / "make_motion_studies.py")
recipes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipes)
FAMILIES = ["late-glass", "rubber-relay", "gate-garden", "paper-motor", "closing-argument", "undertint", "loose-change", "carousel-wire"]


class MotionStudyTests(unittest.TestCase):
    def definitions(self):
        return [json.loads((BATCH / (family + "-r1.nitridepreset")).read_text()) for family in FAMILIES]

    def test_append_only_programs_and_explicit_factory_promotion(self):
        registry = ev.validate_builtins()["entries"]
        # Freeze the entire old table, not merely its size or a few notable IDs.
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:62])).hexdigest(),
                         "f0335b887cdc99c62db224575330c584ce5b9b0268bd0a4957b5b31004a92e97")
        entries = [e for e in registry if e["preset"].startswith("batch-18/")]
        self.assertEqual([e["family"] for e in entries], FAMILIES)
        self.assertEqual([e["program"] for e in entries], list(range(66, 74)))
        selection, selected = fp.validate_selection()
        motion_ids = {e["id"] for e in entries}
        self.assertEqual(len(selected), 69)
        self.assertTrue(motion_ids <= {e["id"] for e in selected})
        selected_programs = {e["id"]: e["program"] for e in selected}
        prior = [e for e in selection["entries"] if selected_programs[e["id"]] < 66]
        self.assertEqual(hashlib.sha256(ev.canonical(prior)).hexdigest(),
                         "ef0b0981aa6ec0180bdd55d66d3a80d75f444470463e90d700734464bc31ddef")
        feedback = next(f for f in fp.acceptance()["additional_feedback"] if set(f["revisions"]) == motion_ids)
        self.assertEqual(feedback["user_feedback"], "ok please promote them all and then commit and push")
        for sketch in recipes.sketches():
            entry = next(e for e in selected if e["id"] == "review:" + sketch["family"] + ":r1")
            self.assertEqual((entry["name"], entry["category"]), (sketch["name"], sketch["host_category"]))

    def test_frozen_generator_and_non_overwrite_guard(self):
        command = [sys.executable, str(BATCH / "make_motion_studies.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 17)
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace", result.stderr)
        definitions = self.definitions()
        self.assertEqual(len({d["id"] for d in definitions}), 8)
        for definition, sketch in zip(definitions, recipes.sketches()):
            self.assertEqual(set(definition["values"]), set(recipes.base.neutral()))
            self.assertFalse(definition["chord"])
            self.assertEqual(definition["mono"], sketch["mono"])
            self.assertEqual(definition["values"]["lfo_tempo"], sketch["bpm"])
            self.assertEqual(definition["values"]["lfo_pitch_enabled"], 0)
            self.assertLessEqual(max(beat + duration for beat, duration, _, _ in sketch["notes"]) * 60 / sketch["bpm"]
                                 + definition["values"]["amp_release"] + 1, sketch["seconds"])
            self.assertTrue(any(duration >= 3 for _, duration, _, _ in sketch["notes"]))
            self.assertTrue(any(duration <= 3 for _, duration, _, _ in sketch["notes"]))

    def test_all_eight_distinct_motion_envelope_mechanisms(self):
        late, rubber, gate, paper, closing, tint, loose, carousel = [d["values"] for d in self.definitions()]
        self.assertGreater(late["fm_attack"], .3)
        self.assertLess(late["amp_attack"], .01)
        self.assertEqual(late["filter_amount"], 0)
        self.assertGreater(late["fm_envelope_amount"], 1)
        self.assertEqual((rubber["lfo_fm_sync"], rubber["lfo_fm_wave"], rubber["lfo_fm_division"]), (1, 1, 12))
        self.assertGreater(rubber["filter_amount"], 1)
        self.assertEqual((gate["lfo_amplitude_wave"], gate["lfo_amplitude_division"], gate["lfo_cutoff_division"]), (3, 12, 13))
        self.assertGreater(gate["filter_attack"], 1)
        self.assertEqual((paper["lfo_stress_wave"], paper["lfo_stress_sync"]), (4, 1))
        self.assertGreater(paper["lfo_coupling_depth"], 0)
        self.assertLess(paper["stress"] + paper["lfo_stress_depth"], .3)
        self.assertLess(closing["filter_amount"], -2)
        self.assertGreater(closing["amp_sustain"], .8)
        self.assertLess(closing["fm_decay"], closing["filter_decay"])
        self.assertGreater(tint["amp_release"], tint["fm_release"] * 4)
        self.assertGreater(tint["fm_attack"], tint["amp_attack"])
        self.assertEqual(tint["lfo_coupling_rate"], tint["lfo_cutoff_rate"])
        self.assertLess(tint["lfo_coupling_depth"] * tint["lfo_cutoff_depth"], 0)
        self.assertEqual((loose["amp_sustain"], loose["lfo_fm_wave"], loose["lfo_fm_sync"]), (0, 4, 0))
        self.assertLess(loose["fm_decay"], loose["amp_decay"])
        self.assertEqual((carousel["lfo_fm_enabled"], carousel["lfo_fm_depth"]), (1, 0))
        self.assertEqual((carousel["fm_route_1_source"], carousel["fm_route_1_target"]), (1, 1))
        self.assertEqual((carousel["fm_route_2_source"], carousel["fm_route_2_target"]), (1, 3))
        self.assertTrue(all(d["values"]["glide_enabled"] == d["values"]["autobend_enabled"] == 0 for d in self.definitions()))

    def test_portable_study_export_and_safe_install_without_private_journal(self):
        # Preserve coverage of the original audition workflow using its pre-promotion
        # selection. Current finalized sounds must not be re-exported as new studies.
        selection, selected = fp.validate_selection(check_history=False)
        prior = [e for e in selected if e["program"] < 66]
        definitions = {66 + i: d for i, d in enumerate(self.definitions())}
        with tempfile.TemporaryDirectory(prefix="nitride-motion-study-test-") as folder:
            root = Path(folder).resolve()
            component, tool = root / "nitride.component", root / "tool"
            build = {"id": "source-unit-test", "installed_path": str(component), "bundle_hashes": {},
                     "binary_sha256": "unit-test", "version": "unit-test"}
            def native(command, chosen_tool, chosen_component):
                self.assertEqual(chosen_tool, tool)
                self.assertEqual(chosen_component, component)
                if command[0] == "export":
                    program = int(command[1]); definition = definitions[program]
                    self.assertEqual(command[2], definition["name"])
                    entry = {"id": "review:" + FAMILIES[program - 66] + ":r1", "program": program, "name": command[3]}
                    Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
                    return "unit test fixture only"
                return json.dumps({"full_document_exact": True, "parameters": 89, "audio_exact": True})
            package = root / "package"
            with mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")), \
                    mock.patch.object(fp, "validate_selection", return_value=(selection, prior)), \
                    mock.patch.object(fp, "component_build", return_value=build), mock.patch.object(fp, "native", side_effect=native):
                manifest = study.export(package, BATCH, None, "motion-envelopes-001", component, tool)
            self.assertEqual(len(manifest["entries"]), 8)
            self.assertEqual(manifest["selection"], "not-factory-selected")
            destination = root / "installed"
            for entry in manifest["entries"]:
                (destination / entry["category"]).mkdir(parents=True, exist_ok=True)
            user = destination / "Pads/My own.aupreset"; user.write_bytes(b"user save")
            factory = destination / fp.FACTORY_RECEIPT; factory.write_bytes(b"old factory receipt")
            first = study.install(package, destination)
            self.assertEqual(first, study.install(package, destination))
            self.assertEqual(user.read_bytes(), b"user save")
            self.assertEqual(factory.read_bytes(), b"old factory receipt")
            edited = destination / manifest["entries"][0]["file"]; edited.write_bytes(b"user edit")
            with self.assertRaisesRegex(ValueError, "never overwrite"):
                study.install(package, destination)
            self.assertEqual(edited.read_bytes(), b"user edit")

    def test_promoted_batch_cannot_be_published_as_an_unapproved_study(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder).resolve() / "package"
            build = {"id": "unit-test", "installed_path": folder, "bundle_hashes": {}}
            with mock.patch.object(fp, "component_build", return_value=build), \
                    mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")):
                with self.assertRaisesRegex(ValueError, "outside the selected factory catalog"):
                    study.export(path, BATCH, None, "motion-envelopes-001", Path(folder) / "component")
            self.assertFalse(path.exists())

    def test_all_eight_owned_duplicates_retire_only_after_exact_factory_replacement(self):
        selection, selected = fp.validate_selection()
        selected = [e for e in selected if e["program"] < 74]  # Historical 53→61 promotion fixture.
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve(); package = root / "package"; package.mkdir()
            destination = root / "installed"; archive = root / "archive"
            manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": "candidate",
                        "status": "awaiting-logic-category-confirmation", "installed_build": "unit-test",
                        "selection": selection["id"], "entries": [], "files": {}}
            motion_files = {}
            for entry in selected:
                source = fp.ROOT / entry["source"]; definition = json.loads(source.read_text())
                relative = entry["category"] + "/" + entry["name"] + ".aupreset"
                frozen = "sources/" + entry["family"] + ".nitridepreset"
                for relative_path, data in ((relative, plistlib.dumps(synthetic_native(definition, entry))), (frozen, source.read_bytes())):
                    path = package / relative_path; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)
                    manifest["files"][relative_path] = ev.digest(path)
                manifest["entries"].append({**entry, "file": relative, "frozen_source": frozen,
                    "native_verification": {"full_document_exact": True, "audio_exact": True, "parameters": 89}})
                if entry["program"] >= 66:
                    study_path = destination / entry["category"] / ("Study - " + entry["name"] + ".aupreset")
                    study_path.parent.mkdir(parents=True, exist_ok=True)
                    study_path.write_bytes(plistlib.dumps(synthetic_native(definition, {**entry, "name": "Study - " + entry["name"]})))
                    motion_files[str(study_path.relative_to(destination))] = ev.digest(study_path)
            all_entries = manifest["entries"]
            manifest["entries"] = [e for e in all_entries if e["program"] < 66]
            (package / "manifest.json").write_text(json.dumps(manifest))
            prior = fp.install_factory(package, destination)
            before = {p: (destination / p).read_bytes() for p in prior["files"]}
            study_receipt = ".nitride-study-motion-envelopes-001.json"
            (destination / study_receipt).write_text(json.dumps({"format": "nitride-study-preset-install", "version": 1,
                "study": "motion-envelopes-001", "status": "awaiting-feedback-not-factory-selected", "files": motion_files}))
            user = destination / "Pads/Study - My own.aupreset"; user.write_bytes(b"personal save")
            manifest["entries"] = all_entries; (package / "manifest.json").write_text(json.dumps(manifest))
            result = fp.extend_factory(package, archive, destination, study_ids=["motion-envelopes-001"])
            self.assertEqual(len(result["files"]), 61)
            self.assertEqual(len(list(archive.rglob("*.aupreset"))), 8)
            self.assertEqual(user.read_bytes(), b"personal save")
            self.assertFalse((destination / study_receipt).exists())
            for relative, data in before.items(): self.assertEqual((destination / relative).read_bytes(), data)
            for relative, expected in motion_files.items():
                self.assertFalse((destination / relative).exists())
                self.assertEqual(ev.digest(archive / relative), expected)

    def test_export_requires_unambiguous_build_provenance(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "package"
            with self.assertRaisesRegex(ValueError, "recorded --trial"):
                study.export(path, BATCH, None, "motion-envelopes-001")
            with self.assertRaisesRegex(ValueError, "not both"):
                study.export(path, BATCH, "some-trial", "motion-envelopes-001", Path(folder) / "component")
            self.assertFalse(path.exists())


if __name__ == "__main__": unittest.main()
