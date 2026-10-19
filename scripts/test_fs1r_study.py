#!/usr/bin/env python3
"""Offline source/mechanism/lineage regression tests, not FS1R hardware or listening verdicts."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import evaluation as ev

ROOT = Path(__file__).resolve().parents[1]
HERE = ROOT / "evaluation/fs1r-studies/pass-001"
spec = importlib.util.spec_from_file_location("selected_fs1r", HERE / "prepare_voices.py")
decoder = None
if (HERE / "prepare_voices.py").is_file():
    decoder = importlib.util.module_from_spec(spec); spec.loader.exec_module(decoder)
spec = importlib.util.spec_from_file_location("fs1r_recipes_test", ROOT / "presets/review/batch-16/r1/make_fs1r_studies.py")
recipes = importlib.util.module_from_spec(spec); spec.loader.exec_module(recipes)


@unittest.skipUnless((HERE / "source-manifest.json").is_file(), "Pinned private FS1R workspace is not distributed")
class Fs1rStudyTests(unittest.TestCase):
    def study_package(self, root):
        package, destination = root / "package", root / "destination"
        package.mkdir(); destination.mkdir()
        files, entries = {}, []
        for sketch in recipes.sketches():
            category = "Leads & Brass" if sketch["category"] == "Leads" else "Plucks & Mallets"
            relative = category + "/Study - " + sketch["name"] + ".aupreset"
            path = package / relative; path.parent.mkdir(exist_ok=True)
            path.write_bytes(sketch["family"].encode()); (destination / category).mkdir(exist_ok=True)
            files[relative] = ev.digest(path)
            entries.append({"id": "review:" + sketch["family"] + ":r1", "file": relative})
        manifest = {"format": "nitride-fs1r-native-trial", "version": 1, "trial": "test", "build": "eval-026",
                    "status": "awaiting-preference-not-factory-approved", "entries": entries, "files": files}
        (package / "manifest.json").write_text(json.dumps(manifest))
        return package, destination, manifest

    def native_installer(self):
        spec = importlib.util.spec_from_file_location("fs1r_installer_test", HERE / "native_trial.py")
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        return module

    def test_study_install_is_additive_and_idempotent_and_refuses_user_collision(self):
        installer = self.native_installer()
        with tempfile.TemporaryDirectory() as directory:
            package, destination, manifest = self.study_package(Path(directory).resolve())
            user = destination / "Leads & Brass/User.aupreset"; user.write_bytes(b"user")
            collision = destination / manifest["entries"][0]["file"]; collision.write_bytes(b"edited")
            with mock.patch.object(installer.fp, "FACTORY_DESTINATION", destination):
                with self.assertRaisesRegex(ValueError, "collision"): installer.install(package)
                self.assertEqual(collision.read_bytes(), b"edited")
                collision.unlink()
                receipt = installer.install(package)
                self.assertEqual(installer.install(package), receipt)
            self.assertEqual(user.read_bytes(), b"user")
            for relative, expected in manifest["files"].items(): self.assertEqual(ev.digest(destination / relative), expected)

    def test_study_install_rolls_back_only_its_own_new_files(self):
        installer = self.native_installer()
        with tempfile.TemporaryDirectory() as directory:
            package, destination, manifest = self.study_package(Path(directory).resolve())
            user = destination / "Leads & Brass/User.aupreset"; user.write_bytes(b"user")
            real_link = installer.os.link; count = 0
            def failing_link(source, target):
                nonlocal count
                count += 1
                if count == 2: raise OSError("injected publication failure")
                return real_link(source, target)
            with mock.patch.object(installer.fp, "FACTORY_DESTINATION", destination), mock.patch.object(installer.os, "link", side_effect=failing_link):
                with self.assertRaisesRegex(OSError, "injected"): installer.install(package)
            self.assertEqual(user.read_bytes(), b"user")
            self.assertFalse((destination / installer.RECEIPT).exists())
            self.assertTrue(all(not (destination / p).exists() for p in manifest["files"]))

    def test_frozen_source_blobs_and_requested_single_voice_slots(self):
        manifest = json.loads((HERE / "source-manifest.json").read_text())
        self.assertEqual(manifest["commit"], "81f2bef16b4437545a4e8b306aab5b2f38bcf93f")
        for relative, item in manifest["files"].items():
            data = (HERE / "sources/fsvr" / relative).read_bytes()
            self.assertEqual(hashlib.sha256(data).hexdigest(), item["sha256"])
            self.assertEqual(hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest(), item["git_blob"])
        voices = json.loads((HERE / "factory-analysis.json").read_text())["voices"]
        self.assertEqual([(v["bank"], v["program_one_based"], v["name"]) for v in voices], [("Pre A", 70, "Kuchibue"), ("Pre B", 5, "Mukkuri"), ("Pre B", 66, "Fmt-Pluck")])
        self.assertEqual([v["upstream_flat_zero_based_index"] for v in voices], [69, 132, 193])
        self.assertEqual([v["algorithm_one_based"] for v in voices], [78, 80, 67])
        for v, selection in zip(voices, decoder.SELECTION):
            source = (HERE / "sources/fsvr/presets/native" / selection[4]).read_bytes()
            selected = (HERE / "selected-factory" / (v["file"] + ".syx")).read_bytes()
            self.assertEqual(source, selected)
            self.assertEqual(len(decoder.payload(selected)), 608)
            bad = bytearray(selected); bad[100] ^= 1
            with self.assertRaisesRegex(ValueError, "checksum"): decoder.payload(bad)

    def test_neutral_part_has_no_layers_fseq_effects_or_hidden_pan_scaling(self):
        off = (HERE / "mechanism-inputs/neutral-filter-off.syx").read_bytes()
        on = (HERE / "mechanism-inputs/neutral-filter-on.syx").read_bytes()
        self.assertEqual(len(off), 411); self.assertEqual(sum(off[4:-1]) & 127, 0)
        a, b = off[9:-2], on[9:-2]
        self.assertEqual([i for i in range(400) if a[i] != b[i]], [199])
        self.assertEqual((a[199], b[199]), (0, 1))
        self.assertEqual(a[0x15], 0)
        for offset in (0x58, 0x5B, 0x5F): self.assertEqual(a[80 + offset], 0)
        for part in range(4):
            offset = 192 + part * 52
            self.assertEqual(a[offset + 1], 1 if part == 0 else 0)
            if part == 0:
                self.assertEqual(a[offset + 0x28], 50)
                self.assertEqual(a[offset + 0x11], 127)
                self.assertEqual(a[offset + 0x12:offset + 0x15], bytes(3))

    def test_probe_changes_are_only_documented_output_level_zeros(self):
        voices = json.loads((HERE / "factory-analysis.json").read_text())["voices"]
        for voice in voices:
            original = decoder.payload((HERE / "selected-factory" / (voice["file"] + ".syx")).read_bytes())
            self.assertEqual(voice["fseq_voiced_mask"], 0); self.assertEqual(voice["fseq_unvoiced_mask"], 0)
            for filename, changes in voice["probe_parameter_changes"].items():
                altered = decoder.payload((HERE / "mechanism-inputs" / filename).read_bytes())
                self.assertEqual([{"offset": i, "before": original[i], "after": altered[i]} for i in range(608) if original[i] != altered[i]], changes)
                self.assertTrue(all(c["after"] == 0 and c["offset"] >= 112 and (c["offset"] - 112) % 62 in (22, 46) for c in changes))

    def test_transfer_gate_rejects_faithful_copies_when_formants_are_essential(self):
        audit = json.loads((HERE / "transfer-audit.json").read_text())
        self.assertFalse(audit["release_selection_changed"]); self.assertIsNone(audit["listening_verdicts"])
        items = {r["source"]: r for r in audit["decisions"]}
        self.assertLess(items["kuchibue"]["source_formant_removal_difference_energy_fraction"], .01)
        self.assertLess(items["kuchibue"]["source_unvoiced_removal_difference_energy_fraction"], .001)
        for name in ("mukkuri", "fmt-pluck"):
            self.assertIn("REJECTED", items[name]["faithful_copy_gate"])
            self.assertGreater(items[name]["source_formant_removal_difference_energy_fraction"], .9)
            self.assertEqual(items[name]["source_sine_only_active_rms"], 0)
        probes = json.loads((HERE / "mechanism-audio/report.json").read_text())
        for probe in probes:
            self.assertEqual(probe["conditions"]["repeat"]["maximum_error"], 0)
            self.assertEqual(probe["conditions"]["block257"]["maximum_error"], 0)
            self.assertIsNone(probe["musical_verdict"])

    def test_new_ids_append_while_factory_selection_and_old_programs_stay_intact(self):
        catalog = ev.validate_builtins()["entries"]
        studies = [e for e in catalog if e["preset"].startswith("batch-16/r1/")]
        self.assertEqual([e["program"] for e in studies], [60, 61, 62])
        self.assertEqual([e["family"] for e in studies], ["whistle-stem", "jaw-twang", "vowel-pick"])
        selection = json.loads((ROOT / "presets/factory/candidate-001.json").read_text())
        self.assertEqual(len(selection["entries"]), 50)
        self.assertTrue({e["id"] for e in studies} <= {e["id"] for e in selection["entries"]})
        source = (ROOT / "CMakeLists.txt").read_text()
        self.assertNotIn("fs1rLib", source); self.assertNotIn("vop3host", source)

    def test_generator_register_and_explicit_root_alignment(self):
        for sketch in recipes.sketches():
            self.assertEqual(sketch["reference_note_offset"], 0)
            preset = json.loads((ROOT / "presets/review/batch-16/r1" / (sketch["family"] + "-r1.nitridepreset")).read_text())
            self.assertFalse(preset["mono"]); self.assertFalse(preset["chord"])
            for key in ("stress", "space_mix", "autobend_enabled", "glide_enabled", "legato"):
                self.assertEqual(preset["values"][key], 0)
            self.assertEqual(preset["values"]["pitch"], sketch["anchor_note_offset"])
        result = subprocess.run([sys.executable, str(ROOT / "presets/review/batch-16/r1/make_fs1r_studies.py"), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 7)
        result = subprocess.run([sys.executable, str(ROOT / "presets/review/batch-16/r1/make_fs1r_studies.py")], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace", result.stderr)


if __name__ == "__main__": unittest.main()
