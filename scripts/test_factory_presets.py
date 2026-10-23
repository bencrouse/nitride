#!/usr/bin/env python3
"""Selection, clean-state and non-destructive installer regressions (no AU/Logic launch)."""
import json
import hashlib
from pathlib import Path
import plistlib
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import xml.etree.ElementTree as ET

import evaluation as ev
import factory_presets as fp
import prepare_factory_catalog as catalog


def synthetic_native(source, entry, document=None):
    """Unit-test envelope ONLY; production export always uses native AU ClassInfo."""
    if document is None:
        document = {"format": "nitride-session", "version": 7,
                    "current": {"patch": {**source, "id": "snapshot"}, "reference": {**source, "id": "reference"},
                                "selected": entry["id"], "program": entry["program"], "wholePatch": True, "parameters": []},
                    "undo": [], "redo": [], "kept": []}
    xml = ET.Element("NITRIDE_INSTRUMENT", version="7")
    ET.SubElement(xml, "DOCUMENT").text = json.dumps(document)
    data = ET.tostring(xml, encoding="utf-8")
    return {"type": int.from_bytes(b"aumu", "big"), "subtype": int.from_bytes(b"NT01", "big"),
            "manufacturer": int.from_bytes(b"NTRD", "big"), "version": 0, "name": entry["name"],
            "jucePluginState": struct.pack("<II", 0x21324356, len(data)) + data + b"\0"}


class SelectionTests(unittest.TestCase):
    def test_final_generators_do_not_require_private_reference_data(self):
        for relative in ("batch-13/r1/make_non_pitch.py", "batch-14/r1/make_classics.py",
                         "batch-15/r1/make_classics.py", "batch-16/r1/make_fs1r_studies.py"):
            result = subprocess.run([sys.executable, str(fp.ROOT / "presets/review" / relative), "--verify"], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, relative + ": " + result.stderr)
            self.assertEqual(result.stdout.count("Verified "), 7)

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="nitride-selection-test-")
        self.root = Path(self.temporary.name).resolve()
        self.selection = json.loads(fp.SELECTION.read_text())

    def tearDown(self):
        self.temporary.cleanup()

    def check_changed(self):
        path = self.root / "selection.json"; path.write_text(json.dumps(self.selection))
        return fp.validate_selection(path)

    def test_retained_selection_is_separate_from_historical_programs(self):
        selection, entries = fp.validate_selection()
        self.assertEqual(len(entries), 53)
        self.assertEqual(len(selection["categories"]), 9)
        self.assertEqual(len({e["family"] for e in entries}), 53)
        self.assertEqual(len(ev.validate_builtins()["entries"]), 62)
        self.assertEqual([e["program"] for e in entries if e["family"] == "cutoff-alloy"], [39])
        self.assertFalse(any(e["name"] == "Cutoff Alloy" for e in entries))
        self.assertEqual({e["id"] for e in entries if e["id"] in selection["pilot"]}, set(selection["pilot"]))
        self.assertEqual(selection["pending_feedback"], [])
        self.assertEqual(selection["original_programs_pending_audition"], [])
        self.assertEqual(sorted(e["program"] for e in entries if e["id"].startswith("factory:")), [0, 1, 2, 3])
        self.assertEqual(set(e["id"] for e in entries), set(fp.acceptance()["retained"]))
        for entry in entries:
            self.assertNotIn("Review /", entry["name"])
            self.assertEqual(ev.digest(fp.ROOT / entry["source"]), entry["source_sha256"])

    def test_unaccepted_voice_cannot_be_silently_promoted(self):
        self.selection["entries"][0]["id"] = "review:soft-alloy:r1"
        with self.assertRaisesRegex(ValueError, "retained-direction feedback"):
            self.check_changed()

    def test_only_one_revision_per_family(self):
        self.selection["entries"].append({"id": "review:prism-pulse:r2", "name": "Old Prism", "category": "Pads"})
        with mock.patch.object(fp, "acceptance", return_value={"retained": fp.acceptance()["retained"] + ["review:prism-pulse:r2"]}):
            with self.assertRaisesRegex(ValueError, "one preferred revision"):
                self.check_changed()

    def test_duplicate_casefold_name_is_rejected(self):
        self.selection["entries"][1]["name"] = self.selection["entries"][0]["name"].upper()
        with self.assertRaisesRegex(ValueError, "Duplicate release name"):
            self.check_changed()

    def test_category_and_filename_paths_are_not_arbitrary(self):
        for name in ("..", ".", "", " Hidden", "a/b", "a\\b", "a:b", "a\0b", "a\nb"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                fp.component_name(name)
        self.selection["entries"][0]["category"] = "Not a category"
        with self.assertRaisesRegex(ValueError, "Unknown release category"):
            self.check_changed()

    def test_pilot_must_be_unique_and_selected(self):
        self.selection["pilot"].append(self.selection["pilot"][0])
        with self.assertRaisesRegex(ValueError, "Pilot must select unique"):
            self.check_changed()

    def test_pending_and_selected_lists_cannot_overlap(self):
        self.selection["pending_feedback"].append(self.selection["entries"][0]["id"])
        with self.assertRaisesRegex(ValueError, "Pending voices"):
            self.check_changed()

    def test_generated_metadata_keeps_sparse_historical_programs(self):
        generated = catalog.header()
        self.assertIn("std::array<FactoryProgram, 53>", generated)
        self.assertIn("std::array<const char*, 66>", generated)
        self.assertIn('{39, "review:cutoff-alloy:r2", "Tine Keys", "Keys & Organs"}', generated)
        self.assertIn('{59, "review:classic-koto:r1"', generated)
        self.assertNotIn('{37, "review:cutoff-alloy:r1"', generated)
        self.assertEqual(generated, catalog.header())

    def test_wrapper_adapter_refuses_unknown_vendor_source(self):
        with self.assertRaisesRegex(ValueError, "differs from pinned"):
            catalog.adapt_wrapper(b"an unreviewed JUCE source file")

    def test_no_advertised_factory_menu_without_removing_legacy_selection(self):
        # Minimal wrapper shape for transformation only; production generation
        # still checks the real pinned source hash, and native tests query the AU.
        source = b'''#include <set>
    ComponentResult GetPresets (CFArrayRef* outData) const override { return noErr; }
    OSStatus NewFactoryPresetSet (const AUPreset& inNewFactoryPreset) override
    {
        if (chosenPresetNumber >= numPrograms) return kAudioUnitErr_InvalidProperty;
        chosenPreset.presetName = juceFilter->getProgramName (chosenPresetNumber).toCFString();
        juceFilter->setCurrentProgram (chosenPresetNumber);
    }
    const String currentProgramName = juceFilter->getProgramName (currentProgramNumber);
'''
        with mock.patch.object(catalog, "WRAPPER_SHA256", hashlib.sha256(source).hexdigest()):
            adapted = catalog.adapt_wrapper(source)
        getter = adapted[adapted.index("ComponentResult GetPresets"):adapted.index("OSStatus NewFactoryPresetSet")]
        self.assertIn("*outData = nullptr", getter)
        self.assertIn("return kAudioUnitErr_InvalidProperty;", getter)
        self.assertNotIn("CFArrayCreate", getter)
        self.assertIn("juceFilter->setCurrentProgram (chosenPresetNumber);", adapted)
        self.assertIn("chosenPresetNumber < 0 || chosenPresetNumber >= numPrograms", adapted)

    def test_source_archive_metadata_build_does_not_require_personal_journal(self):
        with mock.patch.object(fp.ev, "read_events", return_value=[]):
            self.assertIn("std::array<FactoryProgram, 53>", catalog.header())
            self.assertEqual(len(fp.validate_selection()[1]), 53)
        with mock.patch.object(fp, "acceptance", return_value={"retained": []}):
            with self.assertRaisesRegex(ValueError, "retained-direction feedback"): fp.validate_selection()

    def test_generated_vendor_copy_preserves_crlf_bytes_and_timestamp(self):
        path = self.root / "generated.mm"
        content = "vendor line\r\nsecond line\r\nnew adapter line\n"
        catalog.write_changed(path, content)
        before = path.stat().st_mtime_ns
        catalog.write_changed(path, content)
        self.assertEqual(path.read_bytes(), content.encode())
        self.assertEqual(path.stat().st_mtime_ns, before)

    def test_host_confirmation_requires_actual_user_feedback_evidence(self):
        note = {"id": "note", "type": "note", "author": "assistant", "payload": {
            "host_category_confirmation": {"status": "confirmed-by-user", "feedback_event": "heard"}}}
        with self.assertRaisesRegex(ValueError, "confirmed by the user"):
            fp.host_confirmation([note])
        feedback = {"id": "heard", "type": "feedback", "author": "assistant", "payload": {"scope": "preset-finalization-native-host-pilot"}}
        with self.assertRaises(ValueError): fp.host_confirmation([feedback, note])
        feedback["author"] = "user"
        self.assertEqual(fp.host_confirmation([feedback, note]), "heard")


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="nitride-preset-package-test-")
        self.root = Path(self.temporary.name).resolve()
        self.package = self.root / "package"; self.package.mkdir()
        self.destination = self.root / "installed-preview"
        selection, entries = fp.validate_selection()
        entries = [e for e in entries if e["id"] in selection["pilot"]]
        self.manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": "pilot",
                         "status": "awaiting-logic-category-confirmation", "installed_build": "unit-test-only",
                         "selection": "unit-test-only",
                         "entries": [], "files": {}}
        for entry in entries:
            source = json.loads((fp.ROOT / entry["source"]).read_text())
            relative = entry["category"] + "/" + entry["name"] + ".aupreset"
            path = self.package / relative; path.parent.mkdir(exist_ok=True)
            path.write_bytes(plistlib.dumps(synthetic_native(source, entry)))
            frozen = "sources/" + entry["family"] + ".nitridepreset"
            target = self.package / frozen; target.parent.mkdir(exist_ok=True)
            target.write_bytes((fp.ROOT / entry["source"]).read_bytes())
            self.manifest["entries"].append({**entry, "file": relative, "frozen_source": frozen,
                "native_verification": {"full_document_exact": True, "audio_exact": True, "parameters": 89}})
            for name in (relative, frozen): self.manifest["files"][name] = ev.digest(self.package / name)
        self.save_manifest()

    def tearDown(self):
        self.temporary.cleanup()

    def save_manifest(self):
        (self.package / "manifest.json").write_text(json.dumps(self.manifest))

    def entry(self):
        return self.manifest["entries"][0]

    def native(self):
        return plistlib.loads((self.package / self.entry()["file"]).read_bytes())

    def save_native(self, native):
        path = self.package / self.entry()["file"]; path.write_bytes(plistlib.dumps(native))
        self.manifest["files"][self.entry()["file"]] = ev.digest(path); self.save_manifest()

    def test_exact_source_identity_and_empty_history(self):
        manifest = fp.verify_package(self.package)
        self.assertEqual(len(manifest["entries"]), 3)
        self.assertEqual(len({e["category"] for e in manifest["entries"]}), 2)
        document = fp.document_from_native(self.native())
        self.assertEqual(document["kept"], [])
        self.assertEqual(document["undo"], [])
        self.assertEqual(document["redo"], [])

    def test_personal_sounds_and_history_are_rejected(self):
        for field in ("undo", "redo", "kept"):
            with self.subTest(field=field):
                native = self.native(); document = fp.document_from_native(native); document[field] = [{"personal": "not for distribution"}]
                source = json.loads((self.package / self.entry()["frozen_source"]).read_text())
                self.save_native(synthetic_native(source, self.entry(), document))
                with self.assertRaisesRegex(ValueError, "personal sounds or edit history"):
                    fp.verify_package(self.package)
                self.save_native(synthetic_native(source, self.entry()))

    def test_changed_sound_conditions_and_compare_reference_are_rejected(self):
        source = json.loads((self.package / self.entry()["frozen_source"]).read_text())
        for field in ("patch", "reference"):
            for condition in ("values", "mono", "chord"):
                with self.subTest(field=field, condition=condition):
                    document = fp.document_from_native(synthetic_native(source, self.entry()))
                    if condition == "values": document["current"][field][condition]["fm_ratio"] = 7.99
                    else: document["current"][field][condition] = not document["current"][field][condition]
                    self.save_native(synthetic_native(source, self.entry(), document))
                    with self.assertRaisesRegex(ValueError, "exact source conditions"):
                        fp.verify_package(self.package)
        self.save_native(synthetic_native(source, self.entry()))

    def test_wrong_component_and_truncated_xml_rejected(self):
        native = self.native(); native["subtype"] = 0
        with self.assertRaisesRegex(ValueError, "Not a Nitride"):
            fp.document_from_native(native)
        native = self.native(); native["jucePluginState"] = native["jucePluginState"][:-1]
        with self.assertRaisesRegex(ValueError, "XML state envelope"):
            fp.document_from_native(native)

    def test_checksum_tampering_stops_install_before_destination_is_created(self):
        with (self.package / self.entry()["file"]).open("ab") as stream: stream.write(b"tampered")
        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            fp.install_preview(self.package, self.destination)
        self.assertFalse(self.destination.exists())

    def test_install_is_non_overwriting_and_receipted(self):
        receipt = fp.install_preview(self.package, self.destination)
        for relative, expected in receipt["files"].items(): self.assertEqual(ev.digest(self.destination / relative), expected)
        before = (self.destination / fp.RECEIPT).read_bytes()
        with self.assertRaisesRegex(ValueError, "already exists"):
            fp.install_preview(self.package, self.destination)
        self.assertEqual((self.destination / fp.RECEIPT).read_bytes(), before)

    def test_existing_unowned_folder_is_never_claimed(self):
        self.destination.mkdir(); existing = self.destination / "User.aupreset"; existing.write_bytes(b"personal")
        with self.assertRaisesRegex(ValueError, "already exists"):
            fp.install_preview(self.package, self.destination)
        self.assertEqual(existing.read_bytes(), b"personal")
        self.assertFalse((self.destination / fp.RECEIPT).exists())

    def test_candidate_cannot_be_installed_before_logic_checkpoint(self):
        self.manifest["scope"] = "candidate"; self.save_manifest()
        with self.assertRaisesRegex(ValueError, "Only the three-preset"):
            fp.install_preview(self.package, self.destination)
        self.assertFalse(self.destination.exists())

    def test_package_paths_and_symlinks_are_rejected(self):
        for path in ("../outside.aupreset", "/outside.aupreset"):
            with self.subTest(path=path), self.assertRaisesRegex(ValueError, "Unsafe package path"):
                fp.package_file(self.package, path)
        link = self.package / "alias.aupreset"; link.symlink_to(self.package / self.entry()["file"])
        with self.assertRaisesRegex(ValueError, "not links"):
            fp.package_file(self.package, "alias.aupreset")

    def test_linked_install_destination_is_rejected(self):
        target = self.root / "user-directory"; target.mkdir()
        link = self.root / "alias"; link.symlink_to(target, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "symbolic links"):
            fp.install_preview(self.package, link / "preview")
        self.assertEqual(list(target.iterdir()), [])

    def test_uninstall_preserves_user_additions(self):
        fp.install_preview(self.package, self.destination)
        user = self.destination / self.entry()["category"] / "My edit.aupreset"; user.write_bytes(b"my edit")
        fp.uninstall_preview(self.destination)
        self.assertEqual(user.read_bytes(), b"my edit")
        self.assertFalse((self.destination / fp.RECEIPT).exists())
        self.assertTrue(all(not (self.destination / e["file"]).exists() for e in self.manifest["entries"]))

    def test_modified_owned_file_prevents_any_uninstall(self):
        receipt = fp.install_preview(self.package, self.destination)
        modified = self.destination / self.entry()["file"]; modified.write_bytes(b"edited by user")
        with self.assertRaisesRegex(ValueError, "modified"):
            fp.uninstall_preview(self.destination)
        self.assertTrue((self.destination / fp.RECEIPT).exists())
        self.assertTrue(all((self.destination / p).exists() for p in receipt["files"]))
        self.assertEqual(modified.read_bytes(), b"edited by user")

    def test_uninstall_without_ownership_receipt_does_not_delete_anything(self):
        self.destination.mkdir(); user = self.destination / "User.aupreset"; user.write_bytes(b"mine")
        with self.assertRaises(ValueError): fp.uninstall_preview(self.destination)
        self.assertEqual(user.read_bytes(), b"mine")

    def prepare_candidate(self):
        self.manifest["scope"] = "candidate"; self.save_manifest()

    def prepare_extension(self, studies=False, study_id=None):
        self.prepare_candidate()
        full = list(self.manifest["entries"])
        self.manifest["entries"] = full[:1]; self.save_manifest()
        fp.install_factory(self.package, self.destination)
        self.manifest["entries"] = full; self.save_manifest()
        if studies or study_id:
            files = {}
            for entry in full:
                relative = entry["category"] + "/Study - " + entry["name"] + ".aupreset"
                path = self.destination / relative; path.parent.mkdir(exist_ok=True)
                native = plistlib.loads((self.package / entry["file"]).read_bytes()); native["name"] = "Study - " + entry["name"]
                path.write_bytes(plistlib.dumps(native)); files[relative] = ev.digest(path)
            name = ".nitride-study-" + study_id + ".json" if study_id else ".nitride-fs1r-study-001.json"
            (self.destination / name).write_text(json.dumps({"format": "nitride-study-preset-install" if study_id else "nitride-fs1r-study-install",
                "version": 1, "files": files, "study": study_id, "status": "awaiting-feedback-not-factory-selected"}))
        return (self.destination / fp.FACTORY_RECEIPT).read_bytes()

    def test_named_study_promotion_archives_only_explicit_owned_duplicates(self):
        before = self.prepare_extension(study_id="pop-001")
        user = self.destination / "Pads/Study - My own setting.aupreset"; user.write_bytes(b"user")
        archive = self.root / "archive"
        receipt = fp.extend_factory(self.package, archive, self.destination, study_ids=["pop-001"])
        self.assertEqual((archive / fp.FACTORY_RECEIPT).read_bytes(), before)
        self.assertTrue((archive / ".nitride-study-pop-001.json").is_file())
        self.assertFalse((self.destination / ".nitride-study-pop-001.json").exists())
        self.assertEqual(user.read_bytes(), b"user")
        self.assertEqual(len(list(archive.rglob("*.aupreset"))), 3)
        for relative, expected in receipt["files"].items(): self.assertEqual(ev.digest(self.destination / relative), expected)

    def test_named_study_edits_identity_and_replacement_are_checked(self):
        before = self.prepare_extension(study_id="pop-001")
        path = next(self.destination.rglob("Study - *.aupreset")); original = path.read_bytes(); path.write_bytes(b"user edit")
        with self.assertRaisesRegex(ValueError, "Study file edited"):
            fp.extend_factory(self.package, self.root / "archive", self.destination, study_ids=["pop-001"])
        path.write_bytes(original)
        receipt = self.destination / ".nitride-study-pop-001.json"; data = json.loads(receipt.read_text()); data["study"] = "other"
        receipt.write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, "identity/status"):
            fp.extend_factory(self.package, self.root / "archive", self.destination, study_ids=["pop-001"])
        self.assertEqual((self.destination / fp.FACTORY_RECEIPT).read_bytes(), before)

    def test_named_study_receipt_failure_restores_all_old_files(self):
        before = self.prepare_extension(study_id="pop-001")
        studies = {p: p.read_bytes() for p in self.destination.rglob("Study - *.aupreset")}
        with mock.patch.object(fp.os, "replace", side_effect=OSError("injected receipt failure")):
            with self.assertRaisesRegex(OSError, "injected"):
                fp.extend_factory(self.package, self.root / "archive", self.destination, study_ids=["pop-001"])
        self.assertEqual((self.destination / fp.FACTORY_RECEIPT).read_bytes(), before)
        self.assertTrue((self.destination / ".nitride-study-pop-001.json").exists())
        for path, expected in studies.items(): self.assertEqual(path.read_bytes(), expected)
        self.assertTrue(all(not (self.destination / e["file"]).exists() for e in self.manifest["entries"][1:]))

    def test_named_study_requires_an_exact_sound_replacement_even_with_a_matching_receipt(self):
        self.prepare_extension(study_id="pop-001")
        receipt_path = self.destination / ".nitride-study-pop-001.json"; receipt = json.loads(receipt_path.read_text())
        relative = next(iter(receipt["files"])); path = self.destination / relative
        entry = self.manifest["entries"][0]; source = json.loads((self.package / entry["frozen_source"]).read_text())
        source["values"]["coupling"] += .01
        path.write_bytes(plistlib.dumps(synthetic_native(source, entry)))
        receipt["files"][relative] = ev.digest(path); receipt_path.write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "no exact clean-label replacement"):
            fp.extend_factory(self.package, self.root / "archive", self.destination, study_ids=["pop-001"])
        self.assertFalse((self.root / "archive").exists())

    def test_extension_is_additive_and_archives_exact_study_duplicates(self):
        before = self.prepare_extension(studies=True)
        user = self.destination / "User.aupreset"; user.write_bytes(b"user")
        archive = self.root / "archive"
        result = fp.extend_factory(self.package, archive, self.destination, retire_studies=True)
        self.assertEqual(len(result["files"]), 3)
        self.assertEqual((archive / fp.FACTORY_RECEIPT).read_bytes(), before)
        self.assertEqual(user.read_bytes(), b"user")
        self.assertEqual(len(list(archive.rglob("Study - *.aupreset"))), 3)
        self.assertFalse(list(self.destination.rglob("Study - *.aupreset")))
        for relative, expected in result["files"].items(): self.assertEqual(ev.digest(self.destination / relative), expected)

    def test_extension_rejects_managed_edits_and_unowned_collisions(self):
        self.prepare_extension()
        path = self.destination / self.manifest["entries"][1]["file"]
        path.parent.mkdir(exist_ok=True); path.write_bytes(b"user")
        with self.assertRaisesRegex(ValueError, "collision"): fp.extend_factory(self.package, self.root / "archive", self.destination)
        self.assertEqual(path.read_bytes(), b"user"); path.unlink()
        managed = self.destination / self.manifest["entries"][0]["file"]; managed.write_bytes(b"edited")
        with self.assertRaisesRegex(ValueError, "existing managed sound"): fp.extend_factory(self.package, self.root / "archive", self.destination)
        self.assertEqual(managed.read_bytes(), b"edited")

    def test_extension_receipt_failure_restores_retired_studies_and_old_receipt(self):
        before = self.prepare_extension(studies=True)
        studies = {p: p.read_bytes() for p in self.destination.rglob("Study - *.aupreset")}
        with mock.patch.object(fp.os, "replace", side_effect=OSError("injected receipt failure")):
            with self.assertRaisesRegex(OSError, "injected"): fp.extend_factory(self.package, self.root / "archive", self.destination, retire_studies=True)
        self.assertEqual((self.destination / fp.FACTORY_RECEIPT).read_bytes(), before)
        for path, data in studies.items(): self.assertEqual(path.read_bytes(), data)
        self.assertTrue((self.destination / ".nitride-fs1r-study-001.json").exists())
        self.assertTrue(all(not (self.destination / e["file"]).exists() for e in self.manifest["entries"][1:]))

    def test_extension_publication_failure_removes_only_new_files(self):
        before = self.prepare_extension()
        real = fp.os.link; calls = 0
        def fail_second(source, target):
            nonlocal calls
            calls += 1
            if calls == 2: raise OSError("injected publication failure")
            real(source, target)
        with mock.patch.object(fp.os, "link", side_effect=fail_second):
            with self.assertRaisesRegex(OSError, "injected"): fp.extend_factory(self.package, self.root / "archive", self.destination)
        self.assertEqual((self.destination / fp.FACTORY_RECEIPT).read_bytes(), before)
        self.assertTrue(all(not (self.destination / e["file"]).exists() for e in self.manifest["entries"][1:]))

    def test_extension_refuses_edited_study_before_any_publication(self):
        before = self.prepare_extension(studies=True)
        edited = next(self.destination.rglob("Study - *.aupreset")); edited.write_bytes(b"user edit")
        with self.assertRaisesRegex(ValueError, "Study file edited"):
            fp.extend_factory(self.package, self.root / "archive", self.destination, retire_studies=True)
        self.assertEqual(edited.read_bytes(), b"user edit")
        self.assertEqual((self.destination / fp.FACTORY_RECEIPT).read_bytes(), before)
        self.assertTrue(all(not (self.destination / e["file"]).exists() for e in self.manifest["entries"][1:]))

    def test_extension_partial_retirement_failure_restores_from_archive(self):
        before = self.prepare_extension(studies=True)
        studies = {p: p.read_bytes() for p in self.destination.rglob("Study - *.aupreset")}
        real = fp.os.rename; calls = 0
        def fail_second(source, target):
            nonlocal calls
            calls += 1
            if calls == 2: raise OSError("injected retirement failure")
            real(source, target)
        with mock.patch.object(fp.os, "rename", side_effect=fail_second):
            with self.assertRaisesRegex(OSError, "injected"):
                fp.extend_factory(self.package, self.root / "archive", self.destination, retire_studies=True)
        self.assertEqual((self.destination / fp.FACTORY_RECEIPT).read_bytes(), before)
        for path, data in studies.items(): self.assertEqual(path.read_bytes(), data)
        self.assertTrue(all(not (self.destination / e["file"]).exists() for e in self.manifest["entries"][1:]))

    def test_root_categories_require_host_confirmation(self):
        self.prepare_candidate()
        with mock.patch.object(fp, "host_confirmation", side_effect=ValueError("No host confirmation")):
            with self.assertRaisesRegex(ValueError, "No host confirmation"):
                fp.install_factory(self.package, self.destination)
        self.assertFalse(self.destination.exists())

    def test_root_install_has_no_preview_wrapper_and_is_idempotent(self):
        self.prepare_candidate()
        with mock.patch.object(fp, "host_confirmation", return_value="user-confirmed-test"):
            first = fp.install_factory(self.package, self.destination)
            second = fp.install_factory(self.package, self.destination)
        self.assertEqual(first, second)
        self.assertFalse((self.destination / "Nitride Preview").exists())
        self.assertTrue((self.destination / fp.FACTORY_RECEIPT).is_file())
        for relative, expected in first["files"].items():
            self.assertEqual(ev.digest(self.destination / relative), expected)

    def test_root_install_never_overwrites_or_claims_an_existing_user_file(self):
        self.prepare_candidate()
        existing = self.destination / self.entry()["file"]; existing.parent.mkdir(parents=True)
        existing.write_bytes(b"user owned")
        with mock.patch.object(fp, "host_confirmation", return_value="user-confirmed-test"):
            with self.assertRaisesRegex(ValueError, "no user preset will be overwritten"):
                fp.install_factory(self.package, self.destination)
        self.assertEqual(existing.read_bytes(), b"user owned")
        self.assertFalse((self.destination / fp.FACTORY_RECEIPT).exists())

    def test_root_reinstall_preserves_edits_in_managed_factory_files(self):
        self.prepare_candidate()
        with mock.patch.object(fp, "host_confirmation", return_value="user-confirmed-test"):
            fp.install_factory(self.package, self.destination)
            edited = self.destination / self.entry()["file"]; edited.write_bytes(b"user edited factory")
            with self.assertRaisesRegex(ValueError, "already exist"):
                fp.install_factory(self.package, self.destination)
        self.assertEqual(edited.read_bytes(), b"user edited factory")

    def test_root_install_publication_failure_rolls_back_only_new_files(self):
        self.prepare_candidate()
        user = self.destination / self.entry()["category"] / "User.aupreset"
        user.parent.mkdir(parents=True); user.write_bytes(b"keep me")
        real_link = fp.os.link
        calls = 0
        def fail_second(source, target):
            nonlocal calls
            calls += 1
            if calls == 2: raise OSError("Injected publication failure")
            real_link(source, target)
        with mock.patch.object(fp, "host_confirmation", return_value="user-confirmed-test"), mock.patch.object(fp.os, "link", side_effect=fail_second):
            with self.assertRaisesRegex(OSError, "Injected"):
                fp.install_factory(self.package, self.destination)
        self.assertEqual(user.read_bytes(), b"keep me")
        self.assertFalse((self.destination / fp.FACTORY_RECEIPT).exists())
        self.assertTrue(all(not (self.destination / e["file"]).exists() for e in self.manifest["entries"]))
        self.assertFalse(list(self.destination.glob(".nitride-factory-stage-*")))


class SourceInstallTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="nitride-source-install-test-")
        self.root = Path(self.temporary.name).resolve()
        self.destination = self.root / "presets"
        self.component = self.root / "nitride.component"
        self.binary = self.component / "Contents/MacOS/nitride"
        self.binary.parent.mkdir(parents=True)
        self.binary.write_bytes(b"unit-test-only, not a real AU")
        (self.component / "Contents/Info.plist").write_bytes(plistlib.dumps({
            "CFBundleIdentifier": "com.nitride.audio.nitride", "CFBundleExecutable": "nitride",
            "CFBundleShortVersionString": "unit-test"}))
        self.tool = self.root / "nitride_au_presets"
        self.tool.write_bytes(b"unit-test-only native tool")
        self.selection, self.entries = fp.validate_selection()
        self.by_program = {e["program"]: e for e in self.entries}
        self.package = self.root / "package"
        self.journal = mock.patch.object(ev, "read_events", side_effect=AssertionError("Private journal accessed"))
        self.journal.start()
        self.native = mock.patch.object(fp, "native", side_effect=self.native_fixture)
        self.native.start()

    def tearDown(self):
        self.native.stop()
        self.journal.stop()
        self.temporary.cleanup()

    def native_fixture(self, command, tool, component=None):
        self.assertEqual(Path(component), self.component)
        self.assertEqual(Path(tool), self.tool)
        if command[0] == "export":
            entry = self.by_program[int(command[1])]
            source = json.loads((fp.ROOT / entry["source"]).read_text())
            self.assertEqual(command[2:4], [source["name"], entry["name"]])
            Path(command[4]).write_bytes(plistlib.dumps(synthetic_native(source, entry)))
            return "Unit test fixture only"
        self.assertEqual(command[0], "verify")
        return json.dumps({"full_document_exact": True, "parameters": 89, "audio_exact": True})

    def export(self):
        # Quiet only the per-file progress; validation remains exercised for all53sounds.
        with mock.patch("builtins.print"):
            return fp.export_package(self.package, "candidate", self.tool, component=self.component)

    def test_explicit_bundle_export_and_deploy_need_no_private_journal(self):
        with mock.patch("builtins.print"):
            result = fp.deploy(self.component, self.tool, self.destination)
        self.assertEqual(len(result["installed"]), 53)
        self.assertEqual(result["preserved"], [])
        self.assertEqual(len(list(self.destination.rglob("*.aupreset"))), 53)
        for entry in self.entries:
            path = self.destination / entry["category"] / (entry["name"] + ".aupreset")
            source = json.loads((fp.ROOT / entry["source"]).read_text())
            fp.validate_native(path, entry, source)

    def test_fresh_export_on_reinstall_preserves_user_edits_saves_and_receipts(self):
        with mock.patch("builtins.print"):
            fp.deploy(self.component, self.tool, self.destination)
        entry = self.entries[0]
        edited = self.destination / entry["category"] / (entry["name"] + ".aupreset")
        edited.write_bytes(b"my edited factory sound")
        user = self.destination / "My own.aupreset"; user.write_bytes(b"my save")
        receipt = self.destination / fp.FACTORY_RECEIPT; receipt.write_bytes(b"old ownership receipt")
        with mock.patch("builtins.print"):
            result = fp.deploy(self.component, self.tool, self.destination)
        self.assertEqual(result["installed"], [])
        self.assertEqual(len(result["preserved"]), 53)
        self.assertEqual(edited.read_bytes(), b"my edited factory sound")
        self.assertEqual(user.read_bytes(), b"my save")
        self.assertEqual(receipt.read_bytes(), b"old ownership receipt")

    def test_missing_files_are_repaired_without_claiming_existing_paths(self):
        self.export()
        result = fp.install_missing(self.package, self.destination)
        absent = self.destination / result["installed"][0]; absent.unlink()
        result = fp.install_missing(self.package, self.destination)
        self.assertEqual(result["installed"], [str(absent.relative_to(self.destination))])
        self.assertEqual(len(result["preserved"]), 52)
        self.assertFalse((self.destination / fp.FACTORY_RECEIPT).exists())

    def test_partial_or_relabelled_catalog_is_refused(self):
        manifest = self.export()
        for entries in (manifest["entries"][:-1], [{**e, "program": 0} for e in manifest["entries"]]):
            changed = {**manifest, "entries": entries}
            (self.package / "manifest.json").write_text(json.dumps(changed))
            with self.assertRaises(ValueError): fp.install_missing(self.package, self.destination)
            self.assertFalse(self.destination.exists())

    def test_linked_category_is_refused_without_writing_through_it(self):
        self.export()
        outside = self.root / "outside"; outside.mkdir()
        self.destination.mkdir()
        (self.destination / self.entries[0]["category"]).symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "Unsafe category"):
            fp.install_missing(self.package, self.destination)
        self.assertEqual(list(outside.iterdir()), [])

    def test_concurrent_save_wins_exclusive_publication(self):
        self.export()
        real_link = fp.os.link
        saved = []
        def concurrent_save(source, target):
            if not saved:
                Path(target).write_bytes(b"concurrent user save")
                saved.append(Path(target))
            real_link(source, target)
        with mock.patch.object(fp.os, "link", side_effect=concurrent_save):
            result = fp.install_missing(self.package, self.destination)
        self.assertEqual(len(result["installed"]), 52)
        self.assertEqual(result["preserved"], [str(saved[0].relative_to(self.destination))])
        self.assertEqual(saved[0].read_bytes(), b"concurrent user save")

    def test_publication_failure_rolls_back_only_new_unchanged_files(self):
        self.export()
        self.destination.mkdir()
        user = self.destination / "User.aupreset"; user.write_bytes(b"user sound")
        real_link = fp.os.link
        calls = 0
        def fail_second(source, target):
            nonlocal calls
            calls += 1
            if calls == 2: raise OSError("injected source install failure")
            real_link(source, target)
        with mock.patch.object(fp.os, "link", side_effect=fail_second):
            with self.assertRaisesRegex(OSError, "injected"):
                fp.install_missing(self.package, self.destination)
        self.assertEqual(list(self.destination.iterdir()), [user])
        self.assertEqual(user.read_bytes(), b"user sound")

    def test_bundle_identity_and_mid_export_changes_are_checked(self):
        plist = self.component / "Contents/Info.plist"
        original = plist.read_bytes()
        plist.write_bytes(plistlib.dumps({"CFBundleIdentifier": "another.synth"}))
        with self.assertRaisesRegex(ValueError, "Not a Nitride"):
            fp.component_build(self.component)
        plist.write_bytes(original)
        def changing_bundle(command, tool, component=None):
            result = self.native_fixture(command, tool, component)
            if command[0] == "verify": self.binary.write_bytes(b"changed during export")
            return result
        with mock.patch.object(fp, "native", side_effect=changing_bundle):
            with mock.patch("builtins.print"), self.assertRaisesRegex(ValueError, "changed during preset export"):
                fp.deploy(self.component, self.tool, self.destination)
        self.assertFalse(self.destination.exists())


if __name__ == "__main__": unittest.main()
