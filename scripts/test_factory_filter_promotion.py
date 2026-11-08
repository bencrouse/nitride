"""Approved same-path factory revision replacement must never discard user work."""
import json
from pathlib import Path
import plistlib
import tempfile
import unittest
from unittest import mock

import evaluation as ev
import factory_presets as fp
from test_factory_presets import synthetic_native


class FactoryFilterPromotionTests(unittest.TestCase):
    def test_exact_sixteen_approved_revisions_replace_parents_not_add_families(self):
        selection, entries = fp.validate_selection(); registry = ev.validate_builtins()["entries"]
        promoted = {e["id"] for e in registry if e["preset"].startswith("batch-22/")}
        self.assertEqual(len(entries), 77); self.assertEqual(len(promoted), 16)
        self.assertTrue(promoted <= {e["id"] for e in entries})
        self.assertFalse({e["parent"] for e in registry if e["id"] in promoted} & {e["id"] for e in entries})
        self.assertEqual(sum(json.loads((fp.ROOT/e["source"]).read_text())["values"].get("filter_family", 0) == 1 for e in entries), 16)
        approval = fp.acceptance()["additional_feedback"][-1]
        self.assertEqual(set(approval["revisions"]), promoted)
        self.assertEqual(approval["user_feedback"], "they all sounds great, let's use them. please promote them.")

    def fixture(self, root):
        package = root/"package"; destination = root/"installed"; package.mkdir(); destination.mkdir()
        selection, entries = fp.validate_selection()
        chosen = [next(e for e in entries if e["family"] == family) for family in ("rubber-relay", "felt-weight")]
        manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": "candidate",
            "status": "awaiting-logic-category-confirmation", "selection": selection["id"], "installed_build": "test",
            "plugin_version": "0.7.46", "entries": [], "files": {}}
        previous = {}; changed = "Bass/Rubber Relay.aupreset"
        for entry in chosen:
            source = fp.ROOT/entry["source"]; definition = json.loads(source.read_text())
            relative = entry["category"]+"/"+entry["name"]+".aupreset"; frozen = "sources/"+entry["family"]+".nitridepreset"
            for name, data in ((relative, plistlib.dumps(synthetic_native(definition, entry))), (frozen, source.read_bytes())):
                path = package/name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data); manifest["files"][name] = ev.digest(path)
            manifest["entries"].append({**entry, "file": relative, "frozen_source": frozen,
                "native_verification": {"parameters": 92, "audio_exact": True, "full_document_exact": True}})
            path = destination/relative; path.parent.mkdir(parents=True, exist_ok=True)
            if relative == changed:
                old = fp.source_registry()["review:rubber-relay:r1"]
                path.write_bytes(plistlib.dumps(synthetic_native(json.loads((fp.ROOT/old["source"]).read_text()), {**entry, **old})))
            else: path.write_bytes((package/relative).read_bytes())
            previous[relative] = ev.digest(path)
        (package/"manifest.json").write_text(json.dumps(manifest))
        (destination/fp.FACTORY_RECEIPT).write_text(json.dumps({"format": "nitride-factory-preset-install", "version": 1, "files": previous}))
        study = "factory-filters-r2-001"; study_file = "Bass/Study - Rubber Relay r2.aupreset"
        new = plistlib.loads((package/changed).read_bytes()); new["name"] = "Study - Rubber Relay r2"
        (destination/study_file).write_bytes(plistlib.dumps(new))
        (destination/(".nitride-study-"+study+".json")).write_text(json.dumps({"format": "nitride-study-preset-install", "version": 1,
            "study": study, "status": "awaiting-feedback-not-factory-selected", "files": {study_file: ev.digest(destination/study_file)}}))
        (destination/"Bass/My own.aupreset").write_bytes(b"unrelated user sound")
        return package, destination, changed, study, selection, chosen

    def invoke(self, fixture, archive, replacements=None, study_ids=None):
        package, destination, changed, study, selection, entries = fixture
        with mock.patch.object(fp, "validate_selection", return_value=(selection, entries)):
            return fp.replace_factory(package, archive, [changed] if replacements is None else replacements, destination,
                                      [study] if study_ids is None else study_ids)

    def test_success_archives_previous_factory_and_exact_study_only(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve(); f = self.fixture(root); package, dest, changed, study, _, _ = f
            before = {str(p.relative_to(dest)): p.read_bytes() for p in dest.rglob("*") if p.is_file()}
            result = self.invoke(f, root/"archive")
            self.assertEqual((dest/changed).read_bytes(), (package/changed).read_bytes())
            self.assertEqual((root/"archive/previous-factory"/changed).read_bytes(), before[changed])
            for relative, data in before.items():
                if relative.startswith(".nitride-study-") or relative.startswith("Bass/Study -"):
                    self.assertFalse((dest/relative).exists()); self.assertEqual((root/"archive/retired-studies"/relative).read_bytes(), data)
                elif relative not in (changed, fp.FACTORY_RECEIPT): self.assertEqual((dest/relative).read_bytes(), data)
            self.assertEqual(len(result["files"]), 2)

    def test_wrong_scope_or_edited_owned_file_cannot_mutate_anything(self):
        for edit in (None, "Bass/Rubber Relay.aupreset", "Bass/Study - Rubber Relay r2.aupreset", "Bass/Felt Weight.aupreset"):
            with self.subTest(edit=edit), tempfile.TemporaryDirectory() as folder:
                root = Path(folder).resolve(); f = self.fixture(root); dest = f[1]
                if edit: (dest/edit).write_bytes(b"user edit")
                before = {p: p.read_bytes() for p in dest.rglob("*") if p.is_file()}
                with self.assertRaises(ValueError): self.invoke(f, root/"archive", replacements=[] if edit is None else None)
                self.assertFalse((root/"archive").exists())
                self.assertEqual(before, {p: p.read_bytes() for p in dest.rglob("*") if p.is_file()})

    def test_unrelated_study_document_cannot_be_retired(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve(); f = self.fixture(root); dest = f[1]
            path = dest/"Bass/Study - Rubber Relay r2.aupreset"
            data = plistlib.loads((f[0]/"Bass/Felt Weight.aupreset").read_bytes()); data["name"] = "Study - Rubber Relay r2"
            path.write_bytes(plistlib.dumps(data)); receipt_path = dest/(".nitride-study-"+f[3]+".json")
            receipt = json.loads(receipt_path.read_text()); receipt["files"][str(path.relative_to(dest))] = ev.digest(path); receipt_path.write_text(json.dumps(receipt))
            with self.assertRaisesRegex(ValueError, "no exact"): self.invoke(f, root/"archive")
            self.assertTrue(path.exists()); self.assertFalse((root/"archive").exists())

    def test_failed_receipt_commit_rolls_back_originals_and_studies(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve(); f = self.fixture(root); dest = f[1]
            before = {p: p.read_bytes() for p in dest.rglob("*") if p.is_file()}
            with mock.patch.object(fp.os, "replace", side_effect=OSError("injected commit failure")):
                with self.assertRaisesRegex(OSError, "commit failure"): self.invoke(f, root/"archive")
            self.assertEqual(before, {p: p.read_bytes() for p in dest.rglob("*") if p.is_file()})
            self.assertTrue((root/"archive/previous-factory"/f[2]).exists())

    def test_concurrent_user_save_wins_and_moved_original_is_preserved(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve(); f = self.fixture(root); dest = f[1]
            target = dest/f[2]; original = target.read_bytes(); receipt = (dest/fp.FACTORY_RECEIPT).read_bytes()
            link = fp.os.link
            def save_first(source, destination):
                if Path(destination) == target and Path(source).name == "0":
                    target.write_bytes(b"concurrent user save")
                    raise FileExistsError("concurrent user save")
                return link(source, destination)
            with mock.patch.object(fp.os, "link", side_effect=save_first):
                with self.assertRaises(FileExistsError): self.invoke(f, root/"archive")
            self.assertEqual(target.read_bytes(), b"concurrent user save")
            self.assertEqual((dest/fp.FACTORY_RECEIPT).read_bytes(), receipt)
            self.assertEqual((root/"archive/rollback-conflicts"/f[2]).read_bytes(), original)
            self.assertTrue((dest/"Bass/Study - Rubber Relay r2.aupreset").exists())


if __name__ == "__main__": unittest.main()
