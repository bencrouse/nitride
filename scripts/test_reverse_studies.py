#!/usr/bin/env python3
"""Reverse-instrument recipes, append-only recall and safe native audition publication."""
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
from test_factory_presets import synthetic_native, historical_selection

BATCH = fp.ROOT / "presets/review/batch-20/r1"
spec = importlib.util.spec_from_file_location("reverse_recipes", BATCH / "make_reverse_studies.py")
recipes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipes)
FAMILIES = ["backspun-tines", "unstruck-bell", "spool-guitar", "inward-tom", "suction-bass", "rewind-reed", "folded-strings", "vanishing-clav"]
R2_BATCH = BATCH.parent / "r2"
spec = importlib.util.spec_from_file_location("reverse_r2_recipes", R2_BATCH / "make_reverse_r2.py")
r2_recipes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r2_recipes)


class ReverseStudyTests(unittest.TestCase):
    def definitions(self):
        return [json.loads((BATCH / (family + "-r1.nitridepreset")).read_text()) for family in FAMILIES]

    def test_prior_90_programs_and_69_factory_sounds_unchanged(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:86])).hexdigest(),
                         "06751be7ddf7a94ee7205f7c77738b365aab720f77a94782bcea658cf76bcd28")
        entries = [e for e in registry if e["preset"].startswith("batch-20/r1/")]
        self.assertEqual([e["family"] for e in entries], FAMILIES)
        self.assertEqual([e["program"] for e in entries], list(range(90, 98)))
        selection, selected = historical_selection()
        programs = {e["id"]: e["program"] for e in selected}
        prior = [e for e in selection["entries"] if programs[e["id"]] < 98]
        self.assertEqual(hashlib.sha256(ev.canonical(prior)).hexdigest(),
                         "26f22e36e26101cd0e1bde266f0e188041edf2721fa219aaf086e09b7a8942f8")
        self.assertEqual(len(selected), 77)
        self.assertFalse({e["id"] for e in entries} & set(fp.acceptance()["retained"]))

    def test_generator_is_exact_deterministic_and_non_overwriting(self):
        command = [sys.executable, str(BATCH / "make_reverse_studies.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 17)
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace", result.stderr)
        self.assertEqual(len({d["id"] for d in self.definitions()}), 8)

    def test_reverse_gesture_is_envelope_led_not_a_free_running_riser(self):
        for definition, sketch in zip(self.definitions(), recipes.sketches()):
            v = definition["values"]
            self.assertEqual(set(v), set(recipes.base.neutral()))
            self.assertFalse(definition["chord"])
            self.assertEqual(definition["mono"], sketch["mono"])
            self.assertEqual(v["amp_attack"], sketch["attack"])
            self.assertGreaterEqual(v["amp_attack"], .2)
            self.assertLessEqual(v["amp_decay"], .03)
            self.assertEqual((v["amp_sustain"], v["fm_sustain"], v["filter_sustain"]), (0, 0, 0))
            self.assertLessEqual(v["amp_release"], .03)
            self.assertGreater(v["fm_attack"], v["amp_attack"] * .8)
            self.assertLessEqual(v["fm_attack"], v["amp_attack"])
            self.assertGreater(v["fm_envelope_amount"], .9)
            self.assertGreater(v["filter_amount"], 1)
            self.assertLessEqual(v["space_mix"], .03)
            self.assertEqual((v["glide_enabled"], v["autobend_enabled"], v["legato"]), (0, 0, 0))
            for lane in ("coupling", "pitch", "fm", "cutoff", "amplitude", "stress"):
                self.assertEqual(v["lfo_" + lane + "_enabled"], 0)

    def test_midi_reaches_endpoint_and_demonstrates_early_release_and_long_hold(self):
        for sketch in recipes.sketches():
            notes = recipes.performance(sketch)
            durations = [n[1] * .6 for n in notes[:3]]
            self.assertGreater(durations[0], sketch["attack"])
            self.assertLess(durations[1], sketch["attack"] / 2)
            self.assertGreater(durations[2], sketch["attack"] + .5)
            self.assertLess(max((t + d) * .6 for t, d, _, _ in notes) + 1, 27)
            self.assertTrue(all(0 <= n <= 127 and 1 <= vel <= 127 for _, _, n, vel in notes))
        self.assertEqual(len({s["host_category"] for s in recipes.sketches()}), 6)
        self.assertGreater(len({d["values"]["fm_ratio"] for d in self.definitions()}), 4)

    def test_native_study_export_and_reinstall_preserve_existing_files(self):
        definitions = {90 + i: d for i, d in enumerate(self.definitions())}
        with tempfile.TemporaryDirectory(prefix="nitride-reverse-study-test-") as folder:
            root = Path(folder).resolve(); component = root / "nitride.component"; tool = root / "tool"
            build = {"id": "unit-test", "version": "unit-test", "installed_path": str(component),
                     "binary_sha256": "unit-test", "bundle_hashes": {}}
            def native(command, chosen_tool, chosen_component):
                self.assertEqual((chosen_tool, chosen_component), (tool, component))
                if command[0] == "export":
                    program = int(command[1]); definition = definitions[program]
                    self.assertEqual(command[2], definition["name"])
                    entry = {"id": "review:" + FAMILIES[program - 90] + ":r1", "program": program, "name": command[3]}
                    Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
                    return "synthetic fixture only"
                return json.dumps({"full_document_exact": True, "parameters": 89, "audio_exact": True})
            package = root / "package"
            with mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")), \
                    mock.patch.object(fp, "component_build", return_value=build), mock.patch.object(fp, "native", side_effect=native):
                manifest = study.export(package, BATCH, None, "reverse-instruments-001", component, tool)
            self.assertEqual(len(manifest["entries"]), 8)
            destination = root / "installed"
            for e in manifest["entries"]: (destination / e["category"]).mkdir(parents=True, exist_ok=True)
            protected = {"Keys & Organs/My keys.aupreset": b"user keys", fp.FACTORY_RECEIPT: b"existing ownership",
                         ".nitride-study-analog-mood-001.json": b"r1 comparison ownership"}
            for p, data in protected.items(): (destination / p).write_bytes(data)
            receipt = study.install(package, destination)
            self.assertEqual(receipt, study.install(package, destination))
            for p, data in protected.items(): self.assertEqual((destination / p).read_bytes(), data)
            edited = destination / manifest["entries"][0]["file"]; edited.write_bytes(b"user edit")
            with self.assertRaisesRegex(ValueError, "never overwrite"): study.install(package, destination)
            self.assertEqual(edited.read_bytes(), b"user edit")

    def test_r2_preserves_all_98_prior_programs_r1_sources_and_midi(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:94])).hexdigest(),
                         "efc39a4c455d27a1f38343bb686eb460e3b85acbcce81bf0622c2b950dab99e0")
        entries = [e for e in registry if e["preset"].startswith("batch-20/r2/")]
        self.assertEqual([e["program"] for e in entries], list(range(98, 106)))
        self.assertEqual([e["family"] for e in entries], FAMILIES)
        for entry, old, sketch in zip(entries, self.definitions(), recipes.sketches()):
            new = json.loads((R2_BATCH / (entry["family"] + "-r2.nitridepreset")).read_text())
            self.assertEqual(entry["parent"], "review:" + entry["family"] + ":r1")
            self.assertEqual((new["mono"], new["chord"]), (old["mono"], old["chord"]))
            self.assertEqual(new["name"], old["name"][:-3] + " r2")
            self.assertNotEqual(new["id"], old["id"])
            self.assertGreater(sum(new["values"][k] != v for k, v in old["values"].items()), 10)
            self.assertEqual((R2_BATCH / (entry["family"] + "-r2.mid")).read_bytes(), (BATCH / (entry["family"] + "-r1.mid")).read_bytes())
            self.assertLess(new["values"]["amp_attack"], sketch["attack"] + .12)
            self.assertGreater(new["values"]["amp_attack"], sketch["attack"] * .30)
        self.assertTrue({e["id"] for e in entries} <= set(fp.acceptance()["retained"]))

    def test_r2_generator_is_frozen_non_overwriting_and_complete(self):
        command = [sys.executable, str(R2_BATCH / "make_reverse_r2.py")]
        result = subprocess.run(command + ["--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 17)
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace", result.stderr)
        for family in FAMILIES:
            values = json.loads((R2_BATCH / (family + "-r2.nitridepreset")).read_text())["values"]
            self.assertEqual(set(values), set(recipes.base.neutral()))
            self.assertEqual(values["amp_sustain"], 0)
            self.assertLessEqual(values["amp_decay"], .10)
            for lane in ("coupling", "pitch", "fm", "cutoff", "amplitude", "stress"):
                self.assertEqual(values["lfo_" + lane + "_sync"], 0)

    def test_r2_uses_distinct_source_and_gesture_mechanisms_not_one_shared_sweep(self):
        tines, bell, guitar, tom, bass, reed, strings, clav = [json.loads((R2_BATCH / (f + "-r2.nitridepreset")).read_text())["values"] for f in FAMILIES]
        self.assertEqual((tines["filter_amount"], tines["space_mix"], tines["stress"]), (0, 0, 0))
        self.assertEqual(tines["fm_ratio"], 4)
        self.assertGreater(bell["fm_ratio"], 7)
        self.assertGreater(bell["fm_operator_mix"], .5)
        self.assertGreater(bell["space_mix"], .2)
        self.assertEqual(guitar["fm_ratio"], .5)
        self.assertLess(guitar["filter_amount"], -1)
        self.assertGreater(guitar["resonance"], .5)
        self.assertEqual(tom["autobend_enabled"], 1)
        self.assertLess(tom["autobend_depth"], -12)
        self.assertEqual(tom["filter_amount"], 0)
        self.assertEqual((bass["fm_depth"], bass["fm_operator_mix"], bass["fm_envelope_amount"], bass["coupling"], bass["stress"], bass["space_mix"]), (0, 0, 0, 0, 0, 0))
        self.assertGreater(reed["stress"], .1)
        self.assertGreater(reed["coupling"], .4)
        self.assertLess(reed["fm_envelope_amount"], -1)
        self.assertEqual(reed["lfo_pitch_enabled"], 1)
        self.assertGreater(strings["fm_operator_mix"], .6)
        self.assertGreater(strings["space_mix"], .25)
        self.assertEqual(strings["fm_envelope_amount"], 0)
        self.assertGreater(clav["filter_fm_depth"], 2)
        self.assertLess(clav["amp_attack"], .12)
        self.assertEqual((clav["stress"], clav["filter_amount"], clav["space_mix"]), (0, 0, 0))
        self.assertGreater(len({v["amp_decay"] for v in (tines, bell, guitar, tom, bass, reed, strings, clav)}), 5)

    def test_r2_native_labels_install_beside_r1_without_claiming_old_paths(self):
        # Preserve the original audition-stage coverage; current finalized r2
        # cannot be republished as a new unapproved study.
        selection, selected = historical_selection()
        prior = [e for e in selected if e["program"] < 98]
        definitions = {98 + i: json.loads((R2_BATCH / (family + "-r2.nitridepreset")).read_text()) for i, family in enumerate(FAMILIES)}
        with tempfile.TemporaryDirectory(prefix="nitride-reverse-r2-test-") as folder:
            root = Path(folder).resolve(); component = root / "component"; tool = root / "tool"
            build = {"id": "unit-test", "version": "unit-test", "installed_path": str(component), "binary_sha256": "unit-test", "bundle_hashes": {}}
            def native(command, chosen_tool, chosen_component):
                self.assertEqual((chosen_tool, chosen_component), (tool, component))
                if command[0] == "export":
                    program = int(command[1]); definition = definitions[program]
                    self.assertTrue(command[3].endswith(" r2"))
                    entry = {"id": "review:" + FAMILIES[program - 98] + ":r2", "program": program, "name": command[3]}
                    Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
                    return "synthetic fixture only"
                return json.dumps({"full_document_exact": True, "parameters": 89, "audio_exact": True})
            package = root / "package"
            with mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")), \
                    mock.patch.object(fp, "validate_selection", return_value=(selection, prior)), \
                    mock.patch.object(fp, "component_build", return_value=build), mock.patch.object(fp, "native", side_effect=native):
                manifest = study.export(package, R2_BATCH, None, "reverse-instruments-002", component, tool, revision_label=True)
            destination = root / "installed"
            for e in manifest["entries"]:
                path = destination / e["category"] / (e["name"][:-3] + ".aupreset")
                path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(b"r1 comparison, possibly edited")
            (destination / ".nitride-study-reverse-instruments-001.json").write_bytes(b"r1 ownership")
            (destination / fp.FACTORY_RECEIPT).write_bytes(b"factory ownership")
            (destination / "Keys & Organs/My keys.aupreset").write_bytes(b"user keys")
            before = {p: p.read_bytes() for p in destination.rglob("*") if p.is_file()}
            receipt = study.install(package, destination)
            self.assertEqual(receipt, study.install(package, destination))
            self.assertEqual(len(receipt["files"]), 8)
            for p, data in before.items(): self.assertEqual(p.read_bytes(), data)

    def test_only_r2_is_finalized_with_explicit_verbatim_approval_and_clean_names(self):
        r2_ids = {"review:" + f + ":r2" for f in FAMILIES}
        r1_ids = {"review:" + f + ":r1" for f in FAMILIES}
        accepted = fp.acceptance()
        self.assertEqual(len(accepted["retained"]), 77)
        self.assertTrue(r2_ids <= set(accepted["retained"]))
        self.assertFalse(r1_ids & set(accepted["retained"]))
        feedback = next(f for f in accepted["additional_feedback"] if set(f["revisions"]) == r2_ids)
        self.assertEqual(feedback["user_feedback"], "ok the r2 versions are excellent, can you promote those to first class citizens? and then commit and push please")
        selected = fp.validate_selection()[1]
        for sketch in recipes.sketches():
            entry = next(e for e in selected if e["id"] == "review:" + sketch["family"] + ":r2")
            self.assertEqual((entry["name"], entry["category"]), (sketch["name"], sketch["host_category"]))

    def test_finalized_r2_cannot_be_exported_as_new_unapproved_studies(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder).resolve() / "package"
            build = {"id": "unit-test", "installed_path": folder, "bundle_hashes": {}}
            with mock.patch.object(fp, "component_build", return_value=build), \
                    mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed")):
                with self.assertRaisesRegex(ValueError, "outside the selected factory catalog"):
                    study.export(path, R2_BATCH, None, "reverse-instruments-002", Path(folder) / "component", revision_label=True)
            self.assertFalse(path.exists())

    def test_promotion_retires_only_exact_owned_r2_preserving_r1_and_old_69(self):
        selection, selected = historical_selection()
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve(); package = root / "package"; package.mkdir()
            destination = root / "installed"; archive = root / "archive"
            manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": "candidate",
                        "status": "awaiting-logic-category-confirmation", "installed_build": "unit-test",
                        "selection": selection["id"], "entries": [], "files": {}}
            r2_ids = {"review:" + f + ":r2" for f in FAMILIES}; r2_files = {}
            for entry in selected:
                source = fp.ROOT / entry["source"]; definition = json.loads(source.read_text())
                relative = entry["category"] + "/" + entry["name"] + ".aupreset"
                frozen = "sources/" + entry["family"] + ".nitridepreset"
                for relative_path, data in ((relative, plistlib.dumps(synthetic_native(definition, entry))), (frozen, source.read_bytes())):
                    path = package / relative_path; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)
                    manifest["files"][relative_path] = ev.digest(path)
                manifest["entries"].append({**entry, "file": relative, "frozen_source": frozen,
                    "native_verification": {"full_document_exact": True, "audio_exact": True, "parameters": 89}})
                if entry["id"] in r2_ids:
                    name = "Study - " + entry["name"] + " r2"
                    path = destination / entry["category"] / (name + ".aupreset")
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(plistlib.dumps(synthetic_native(definition, {**entry, "name": name})))
                    r2_files[str(path.relative_to(destination))] = ev.digest(path)
            r1_files = {}
            for i, (sketch, definition) in enumerate(zip(recipes.sketches(), self.definitions())):
                entry = {"id": "review:" + sketch["family"] + ":r1", "program": 90 + i, "name": "Study - " + sketch["name"]}
                path = destination / sketch["host_category"] / (entry["name"] + ".aupreset")
                path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(plistlib.dumps(synthetic_native(definition, entry)))
                r1_files[str(path.relative_to(destination))] = ev.digest(path)
            def study_receipt(identity, files):
                path = destination / (".nitride-study-" + identity + ".json")
                path.write_text(json.dumps({"format": "nitride-study-preset-install", "version": 1,
                    "study": identity, "status": "awaiting-feedback-not-factory-selected", "files": files}))
                return path
            r1_receipt = study_receipt("reverse-instruments-001", r1_files)
            r2_receipt = study_receipt("reverse-instruments-002", r2_files)
            user = destination / "Pads/Study - My own.aupreset"; user.write_bytes(b"user save")
            protected = {str(p.relative_to(destination)): p.read_bytes() for p in destination.rglob("*") if p.is_file()
                         and str(p.relative_to(destination)) not in r2_files and p != r2_receipt}
            all_entries = manifest["entries"]
            manifest["entries"] = [e for e in all_entries if e["program"] < 98]
            (package / "manifest.json").write_text(json.dumps(manifest))
            prior = fp.install_factory(package, destination); self.assertEqual(len(prior["files"]), 69)
            protected.update({p: (destination / p).read_bytes() for p in prior["files"]})
            manifest["entries"] = all_entries; (package / "manifest.json").write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, "no exact clean-label replacement"):
                fp.extend_factory(package, root / "must-not-retire-r1", destination, study_ids=["reverse-instruments-001"])
            self.assertFalse((root / "must-not-retire-r1").exists())
            result = fp.extend_factory(package, archive, destination, study_ids=["reverse-instruments-002"])
            self.assertEqual(len(result["files"]), 77)
            self.assertEqual(len(list(archive.rglob("*.aupreset"))), 8)
            self.assertFalse(r2_receipt.exists()); self.assertTrue(r1_receipt.exists())
            for p, data in protected.items(): self.assertEqual((destination / p).read_bytes(), data)
            for p, expected in r2_files.items():
                self.assertFalse((destination / p).exists())
                self.assertEqual(ev.digest(archive / p), expected)


if __name__ == "__main__": unittest.main()
