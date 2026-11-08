#!/usr/bin/env python3
"""Original detuned-analog study lineage and safe publication; no artist-fidelity or listening verdict."""
import hashlib
import importlib.util
import json
import math
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
from test_factory_presets import synthetic_native, historical_selection

BATCH = fp.ROOT / "presets/review/batch-19/r1"
spec = importlib.util.spec_from_file_location("analog_recipes", BATCH / "make_analog_studies.py")
recipes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipes)
R2_BATCH = BATCH.parent / "r2"
FAMILIES = ["low-battery-piano", "slack-drum", "brown-console", "silt-bass", "bent-aerial", "melted-ensemble", "oxide-rim", "reel-clav"]


class AnalogStudyTests(unittest.TestCase):
    def definitions(self):
        return [json.loads((BATCH / (family + "-r1.nitridepreset")).read_text()) for family in FAMILIES]

    def test_old_programs_and_unselected_r1_stay_intact(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:70])).hexdigest(),
                         "854a3dd8917935910bf763edc7b78e5217744b6da9b2d984311f5f3fb16906de")
        entries = [e for e in registry if e["preset"].startswith("batch-19/r1/")]
        self.assertEqual([e["family"] for e in entries], FAMILIES)
        self.assertEqual([e["program"] for e in entries], list(range(74, 82)))
        self.assertTrue(all("not artist-authentic" in e["design_lens"] for e in entries))
        selected = fp.validate_selection()[1]
        self.assertEqual(len(selected), 77)
        self.assertFalse({e["id"] for e in entries} & {e["id"] for e in selected})

    def test_generator_is_frozen_deterministic_and_non_overwriting(self):
        command = [sys.executable, str(BATCH / "make_analog_studies.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 17)
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace", result.stderr)
        definitions = self.definitions()
        self.assertEqual(len({d["id"] for d in definitions}), 8)
        for definition, sketch in zip(definitions, recipes.sketches()):
            values = definition["values"]
            self.assertEqual(set(values), set(recipes.base.neutral()))
            self.assertFalse(definition["chord"])
            self.assertEqual(values["lfo_tempo"], sketch["bpm"])
            for lane in ("pitch", "fm", "cutoff", "amplitude", "coupling", "stress"):
                self.assertEqual(values["lfo_" + lane + "_sync"], 0)
            end = max(beat + duration for beat, duration, _, _ in sketch["notes"]) * 60 / sketch["bpm"]
            self.assertLess(end + values["amp_release"] + 1, sketch["seconds"])
            self.assertTrue(any(duration >= 3 for _, duration, _, _ in sketch["notes"]))

    def test_relative_detune_is_not_just_a_shared_vibrato_setting(self):
        definitions = self.definitions()
        for definition, sketch in zip(definitions, recipes.sketches()):
            values = definition["values"]
            cents = 1200 * math.log2(values["fm_ratio"] / sketch["tuned_ratio"])
            self.assertGreater(abs(cents), 2)
            self.assertLess(abs(cents), 20)
            self.assertGreater(values["fm_operator_mix"], 0)
            self.assertLessEqual(abs(values["lfo_pitch_depth"]), .09)
            self.assertLessEqual(values["stress"], .08)
        piano, drum, organ, bass, lead, ensemble, rim, clav = [d["values"] for d in definitions]
        self.assertEqual(piano["fm_ratio"], 1.003)
        self.assertLess(piano["amp_sustain"], .1)
        self.assertGreater(piano["filter_velocity"], 1)
        self.assertEqual((drum["fm_ratio"], drum["lfo_pitch_enabled"], drum["amp_sustain"]), (1.011, 0, 0))
        self.assertGreater(drum["amp_decay"], .5)
        self.assertEqual(drum["autobend_target"], 2)
        self.assertGreater(organ["amp_sustain"], .9)
        self.assertLess(organ["amp_attack"], .01)
        self.assertEqual(bass["space_mix"], 0)
        self.assertEqual(bass["legato"], 1)
        self.assertGreater(ensemble["amp_attack"], .5)
        self.assertLess(ensemble["cutoff"], 3000)
        self.assertGreater(ensemble["fm_operator_mix"], .4)
        for values in (organ, lead, ensemble):
            self.assertEqual((values["lfo_fm_enabled"], values["lfo_fm_depth"], values["fm_route_1_target"]), (1, 0, 1))
            self.assertLessEqual(abs(values["fm_route_1_amount"]), .004)
        self.assertLess(rim["amp_decay"], drum["amp_decay"] / 4)
        self.assertEqual((clav["lfo_pitch_wave"], clav["lfo_pitch_sync"]), (4, 0))

    def test_portable_native_export_and_additive_install_preserve_user_files(self):
        definitions = {74 + i: d for i, d in enumerate(self.definitions())}
        with tempfile.TemporaryDirectory(prefix="nitride-analog-study-test-") as folder:
            root = Path(folder).resolve(); component = root / "nitride.component"; tool = root / "tool"
            build = {"id": "unit-test", "version": "unit-test", "installed_path": str(component),
                     "binary_sha256": "unit-test", "bundle_hashes": {}}
            def native(command, chosen_tool, chosen_component):
                self.assertEqual((chosen_tool, chosen_component), (tool, component))
                if command[0] == "export":
                    program = int(command[1]); definition = definitions[program]
                    self.assertEqual(command[2], definition["name"])
                    entry = {"id": "review:" + FAMILIES[program - 74] + ":r1", "program": program, "name": command[3]}
                    Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
                    return "unit fixture only"
                return json.dumps({"full_document_exact": True, "parameters": 89, "audio_exact": True})
            package = root / "package"
            with mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")), \
                    mock.patch.object(fp, "component_build", return_value=build), mock.patch.object(fp, "native", side_effect=native):
                manifest = study.export(package, BATCH, None, "analog-mood-001", component, tool)
            self.assertEqual(len(manifest["entries"]), 8)
            destination = root / "installed"
            for entry in manifest["entries"]: (destination / entry["category"]).mkdir(parents=True, exist_ok=True)
            user = destination / "Keys & Organs/My piano.aupreset"; user.write_bytes(b"user piano")
            receipt = destination / fp.FACTORY_RECEIPT; receipt.write_bytes(b"existing factory ownership")
            first = study.install(package, destination)
            self.assertEqual(first, study.install(package, destination))
            self.assertEqual(user.read_bytes(), b"user piano")
            self.assertEqual(receipt.read_bytes(), b"existing factory ownership")
            modified = destination / manifest["entries"][0]["file"]; modified.write_bytes(b"user edit")
            with self.assertRaisesRegex(ValueError, "never overwrite"): study.install(package, destination)
            self.assertEqual(modified.read_bytes(), b"user edit")

    def test_only_r2_is_finalized_with_explicit_user_feedback(self):
        ids = {"review:" + family + ":r1" for family in FAMILIES}
        self.assertFalse(ids & set(fp.acceptance()["retained"]))
        self.assertEqual(len(fp.acceptance()["retained"]), 77)
        r2_ids = {"review:" + family + ":r2" for family in FAMILIES}
        # Subsequent filter revisions supersede six r2 selections, not their approval/history.
        selection, selected = historical_selection()
        self.assertTrue(r2_ids <= {e["id"] for e in selected})
        programs = {e["id"]: e["program"] for e in selected}
        prior = [e for e in selection["entries"] if programs[e["id"]] < 82]
        self.assertEqual(hashlib.sha256(ev.canonical(prior)).hexdigest(),
                         "a6fea3726967df023b4c8ccf38ef24417a5dda5841f8bd054d76cf1c4a951692")
        feedback = next(f for f in fp.acceptance()["additional_feedback"] if set(f["revisions"]) == r2_ids)
        self.assertEqual(feedback["user_feedback"], "ok yes very good. can you finalize/promote these and commit and push?")
        for sketch in recipes.sketches():
            entry = next(e for e in selected if e["id"] == "review:" + sketch["family"] + ":r2")
            self.assertEqual((entry["name"], entry["category"]), (sketch["name"], sketch["host_category"]))
        self.assertEqual(json.loads((BATCH / "batch.json").read_text())["revision"], "r1")

    def test_r2_changes_only_static_ratio_and_keeps_original_midi(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:78])).hexdigest(),
                         "5ec55c6ca58e8c4c2efd196e30e38e1e447de01184d1396937d282553bc10810")
        entries = [e for e in registry if e["preset"].startswith("batch-19/r2/")]
        self.assertEqual([e["program"] for e in entries], list(range(82, 90)))
        self.assertEqual([e["family"] for e in entries], FAMILIES)
        for sketch, old, entry in zip(recipes.sketches(), self.definitions(), entries):
            family = sketch["family"]
            new = json.loads((R2_BATCH / (family + "-r2.nitridepreset")).read_text())
            changed = {key for key, value in old["values"].items() if new["values"][key] != value}
            self.assertEqual(changed, {"fm_ratio"})
            self.assertEqual((new["mono"], new["chord"]), (old["mono"], old["chord"]))
            self.assertEqual(new["name"], old["name"][:-3] + " r2")
            self.assertNotEqual(new["id"], old["id"])
            self.assertEqual(entry["parent"], "review:" + family + ":r1")
            self.assertEqual((R2_BATCH / (family + "-r2.mid")).read_bytes(), (BATCH / (family + "-r1.mid")).read_bytes())
            before = 1200 * math.log2(old["values"]["fm_ratio"] / sketch["tuned_ratio"])
            after = 1200 * math.log2(new["values"]["fm_ratio"] / sketch["tuned_ratio"])
            self.assertGreater(before * after, 0)
            self.assertGreater(abs(after / before), 1.5)
            self.assertLess(abs(after / before), 2.05)
            self.assertLess(abs(after), 30)
        result = subprocess.run([sys.executable, str(R2_BATCH / "make_analog_r2.py"), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 17)
        result = subprocess.run([sys.executable, str(R2_BATCH / "make_analog_r2.py")], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace", result.stderr)

    def test_revision_label_installs_r2_beside_r1_without_overwriting(self):
        # Keep coverage of the original audition stage without republishing a now-
        # finalized batch as unapproved. Current policy is checked separately below.
        selection, selected = historical_selection()
        prior = [e for e in selected if e["program"] < 82]
        definitions = {82 + i: json.loads((R2_BATCH / (family + "-r2.nitridepreset")).read_text()) for i, family in enumerate(FAMILIES)}
        with tempfile.TemporaryDirectory(prefix="nitride-analog-r2-test-") as folder:
            root = Path(folder).resolve(); component = root / "component"; tool = root / "tool"
            build = {"id": "unit-test", "version": "unit-test", "installed_path": str(component), "binary_sha256": "unit-test", "bundle_hashes": {}}
            def native(command, chosen_tool, chosen_component):
                self.assertEqual((chosen_tool, chosen_component), (tool, component))
                if command[0] == "export":
                    program = int(command[1]); definition = definitions[program]
                    self.assertTrue(command[3].endswith(" r2"))
                    entry = {"id": "review:" + FAMILIES[program - 82] + ":r2", "program": program, "name": command[3]}
                    Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
                    return "unit fixture only"
                return json.dumps({"full_document_exact": True, "parameters": 89, "audio_exact": True})
            package = root / "package"
            with mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")), \
                    mock.patch.object(fp, "validate_selection", return_value=(selection, prior)), \
                    mock.patch.object(fp, "component_build", return_value=build), mock.patch.object(fp, "native", side_effect=native):
                manifest = study.export(package, R2_BATCH, None, "analog-mood-002", component, tool, revision_label=True)
            destination = root / "installed"
            for entry in manifest["entries"]:
                path = destination / entry["category"] / (entry["name"][:-3] + ".aupreset")
                path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(b"old r1, possibly user-edited")
            old_receipt = destination / ".nitride-study-analog-mood-001.json"; old_receipt.write_bytes(b"old r1 ownership")
            before = {p: p.read_bytes() for p in destination.rglob("*") if p.is_file()}
            receipt = study.install(package, destination)
            self.assertEqual(receipt, study.install(package, destination))
            self.assertEqual(len(receipt["files"]), 8)
            for path, data in before.items(): self.assertEqual(path.read_bytes(), data)
            self.assertEqual(len(list(destination.rglob("*.aupreset"))), 16)

    def test_finalized_r2_cannot_be_exported_as_new_unapproved_studies(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder).resolve() / "package"
            build = {"id": "unit-test", "installed_path": folder, "bundle_hashes": {}}
            with mock.patch.object(fp, "component_build", return_value=build), \
                    mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")):
                with self.assertRaisesRegex(ValueError, "outside the selected factory catalog"):
                    study.export(path, R2_BATCH, None, "analog-mood-002", Path(folder) / "component", revision_label=True)
            self.assertFalse(path.exists())

    def test_r2_promotion_retires_only_exact_owned_r2_and_preserves_r1(self):
        selection, selected = historical_selection()
        selected = [e for e in selected if e["program"] < 90]  # Historical 61→69 fixture.
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve(); package = root / "package"; package.mkdir()
            destination = root / "installed"; archive = root / "archive"
            manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": "candidate",
                        "status": "awaiting-logic-category-confirmation", "installed_build": "unit-test",
                        "selection": selection["id"], "entries": [], "files": {}}
            r2_files = {}
            for entry in selected:
                source = fp.ROOT / entry["source"]; definition = json.loads(source.read_text())
                relative = entry["category"] + "/" + entry["name"] + ".aupreset"
                frozen = "sources/" + entry["family"] + ".nitridepreset"
                for relative_path, data in ((relative, plistlib.dumps(synthetic_native(definition, entry))), (frozen, source.read_bytes())):
                    path = package / relative_path; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)
                    manifest["files"][relative_path] = ev.digest(path)
                manifest["entries"].append({**entry, "file": relative, "frozen_source": frozen,
                    "native_verification": {"full_document_exact": True, "audio_exact": True, "parameters": 89}})
                if entry["id"] in {"review:" + family + ":r2" for family in FAMILIES}:
                    path = destination / entry["category"] / ("Study - " + entry["name"] + " r2.aupreset")
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(plistlib.dumps(synthetic_native(definition, {**entry, "name": "Study - " + entry["name"] + " r2"})))
                    r2_files[str(path.relative_to(destination))] = ev.digest(path)
            for i, sketch in enumerate(recipes.sketches()):
                definition = self.definitions()[i]
                entry = {"id": "review:" + sketch["family"] + ":r1", "program": 74 + i, "name": "Study - " + sketch["name"]}
                path = destination / sketch["host_category"] / (entry["name"] + ".aupreset")
                path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
            original_receipt = destination / ".nitride-study-analog-mood-001.json"
            original_receipt.write_bytes(b"original r1 comparison ownership; never touched")
            user = destination / "Pads/Study - My own.aupreset"; user.write_bytes(b"user save")
            existing = {str(p.relative_to(destination)): p.read_bytes() for p in destination.rglob("*") if p.is_file()
                        and str(p.relative_to(destination)) not in r2_files}
            all_entries = manifest["entries"]
            manifest["entries"] = [e for e in all_entries if e["program"] < 82]
            (package / "manifest.json").write_text(json.dumps(manifest))
            prior = fp.install_factory(package, destination)
            self.assertEqual(len(prior["files"]), 61)
            existing.update({p: (destination / p).read_bytes() for p in prior["files"]})
            r2_receipt = ".nitride-study-analog-mood-002.json"
            (destination / r2_receipt).write_text(json.dumps({"format": "nitride-study-preset-install", "version": 1,
                "study": "analog-mood-002", "status": "awaiting-feedback-not-factory-selected", "files": r2_files}))
            manifest["entries"] = all_entries; (package / "manifest.json").write_text(json.dumps(manifest))
            result = fp.extend_factory(package, archive, destination, study_ids=["analog-mood-002"])
            self.assertEqual(len(result["files"]), 69)
            self.assertEqual(len(list(archive.rglob("*.aupreset"))), 8)
            self.assertFalse((destination / r2_receipt).exists())
            for relative, data in existing.items(): self.assertEqual((destination / relative).read_bytes(), data)
            for relative, expected in r2_files.items():
                self.assertFalse((destination / relative).exists())
                self.assertEqual(ev.digest(archive / relative), expected)


if __name__ == "__main__": unittest.main()
