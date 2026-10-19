from pathlib import Path
import json
import sys
import tempfile
import threading
import subprocess
import unittest
from unittest.mock import patch

import evaluation as ev


class EvaluationTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="nitride-evaluation-")
        self.root = Path(self.directory.name) / "evaluation"
        self.ledger = ev.Ledger(self.root)

    def tearDown(self):
        self.directory.cleanup()

    def test_rebuild_and_verbatim_feedback(self):
        with self.ledger.locked():
            self.ledger.append("feedback", {"text": "Good body.\nToo bright!", "interpretation": "Reduce edge, not motion."}, "user")
            self.ledger.append("note", {"text": "Pending Logic trial", "next": "Listen in Logic"}, "assistant")
            state = self.ledger.rebuild()
            (self.root / "catalog.json").unlink()
            (self.root / "CURRENT.md").unlink()
            self.assertEqual(state, self.ledger.rebuild())
        self.assertEqual(self.ledger.events()[0]["payload"]["text"], "Good body.\nToo bright!")
        self.assertIn("Listen in Logic", (self.root / "CURRENT.md").read_text())

    def test_superseding_preserves_original(self):
        with self.ledger.locked():
            original = self.ledger.append("feedback", {"text": "Too dark"}, "user")
            self.ledger.append("feedback", {"text": "Correction: too bright", "supersedes": original["id"]}, "user")
        self.assertEqual(len(self.ledger.events()), 2)
        self.assertEqual(self.ledger.rebuild()["feedback"][0]["text"], "Correction: too bright")
        with self.assertRaises(ValueError):
            self.ledger.append("note", {"text": "Wrong type", "supersedes": original["id"]})

    def test_corruption_is_not_silently_accepted(self):
        self.ledger.append("note", {"text": "Original"})
        path = self.root / "events.jsonl"
        path.write_bytes(path.read_bytes().replace(b"Original", b"Modified"))
        with self.assertRaises(ValueError):
            self.ledger.rebuild()

    def test_torn_tail_requires_explicit_recovery(self):
        self.ledger.append("note", {"text": "Complete event"})
        path = self.root / "events.jsonl"
        with path.open("ab") as stream:
            stream.write(b'{"partial":')
        original = path.read_bytes()
        with self.assertRaises(ValueError):
            self.ledger.append("note", {"text": "Must not append"})
        with patch.object(sys, "argv", ["evaluation.py", "--root", str(self.root), "recover-tail"]):
            ev.main()
        self.assertEqual(len(self.ledger.events()), 2)
        saved = list((self.root / "diagnostics").glob("*-original-journal.bin"))
        self.assertEqual(saved[0].read_bytes(), original)

    def test_registered_assets_are_frozen(self):
        entry = ev.validate_builtins()["entries"][0]
        renders = Path(self.directory.name) / "renders"
        renders.mkdir()
        stem = Path(entry["preset"]).stem
        for name in (stem + ".wav", stem + "-raw.wav"):
            (renders / name).write_bytes(b"test evidence")
        preset = json.loads((ev.PROJECT / "presets/review" / entry["preset"]).read_text())
        ev.write_json(renders / "report.json", [{"name": preset["name"], "evaluationBuild": "eval-test", "synthVersion": "0.7.2"}])
        with self.ledger.locked():
            self.ledger.register(entry, renders)
            self.ledger.register(entry, renders)
            self.assertEqual(len(self.ledger.events()), 1)
            self.assertEqual(self.ledger.events()[0]["payload"]["render_provenance"]["evaluation_build"], "eval-test")
            self.ledger.verify()
        frozen = self.root / "revisions" / entry["family"] / entry["revision"] / "preset.nitridepreset"
        frozen.write_text("mutated")
        with self.assertRaises(ValueError):
            self.ledger.verify()

    def test_backup_is_external_and_never_overwrites(self):
        self.ledger.append("note", {"text": "Persistent baseline"})
        output = Path(self.directory.name) / "backup.zip"
        self.assertEqual(len(self.ledger.backup(output)), 64)
        with self.assertRaises(ValueError):
            self.ledger.backup(output)
        with self.assertRaises(ValueError):
            self.ledger.backup(self.root / "backup.zip")

    def render_fixture(self, entry):
        renders = Path(self.directory.name) / (entry["family"] + "-" + entry["revision"])
        renders.mkdir()
        source = ev.PROJECT / "presets/review" / entry["preset"]
        preset = json.loads(source.read_text())
        stem = source.stem
        for suffix in (".wav", "-raw.wav"):
            (renders / (stem + suffix)).write_bytes(b"test evidence")
        (renders / (stem + ".nitridepreset")).write_text(json.dumps(preset, indent=4))
        (renders / (stem + ".mid")).write_bytes(source.with_suffix(".mid").read_bytes())
        ev.write_json(renders / "report.json", [{"name": preset["name"], "evaluationBuild": "eval-test", "synthVersion": "0.7.2"}])
        return renders

    def test_revision_lineage_and_isolated_bass_change(self):
        entries = ev.validate_builtins()["entries"]
        with self.ledger.locked():
            for entry in (entries[1], entries[3]):
                self.ledger.register(entry, self.render_fixture(entry))
            child = self.ledger.events()[-1]["payload"]
            self.assertEqual(child["parent"], "review:tension-core:r1")
            self.assertEqual(child["parameter_changes"], {"autobend_enabled": {"before": 1.0, "after": 0.0}})
            self.ledger.verify()

    def test_render_formatting_is_not_sound_identity(self):
        entry = ev.validate_builtins()["entries"][0]
        renders = self.render_fixture(entry)
        with self.ledger.locked():
            self.ledger.register(entry, renders)
            self.ledger.append("build", {"id": "eval-test", "version": "0.7.2", "files": {}})
            self.ledger.record_render("first", entry["id"], "eval-test", renders)
            exported = renders / (Path(entry["preset"]).stem + ".nitridepreset")
            preset = json.loads(exported.read_text())
            preset["values"]["fm_depth"] += .1
            ev.write_json(exported, preset)
            with self.assertRaises(ValueError):
                self.ledger.record_render("changed", entry["id"], "eval-test", renders)
            self.assertFalse((self.root / "artifacts" / entry["family"] / entry["revision"] / "changed").exists())
            self.ledger.verify()

    def test_assistant_cannot_promote_to_approved(self):
        self.ledger.append("revision", {"id": "review:sample:r1", "family": "sample", "revision": "r1", "category": "Pads", "program": 4})
        args = ["evaluation.py", "--root", str(self.root), "decision", "--revision", "review:sample:r1", "--status", "approved", "--reason", "Assumed", "--author", "assistant"]
        with patch.object(sys, "argv", args), self.assertRaises(ValueError):
            ev.main()
        self.assertEqual(len(self.ledger.events()), 1)

    def test_concurrent_writers_keep_a_single_valid_chain(self):
        failures = []
        def write(worker):
            try:
                ledger = ev.Ledger(self.root)
                for i in range(4):
                    with ledger.locked():
                        ledger.append("note", {"text": f"worker {worker} / {i}"})
            except Exception as error:
                failures.append(error)
        threads = [threading.Thread(target=write, args=(i,)) for i in range(3)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join(timeout=10)
        self.assertTrue(all(not t.is_alive() for t in threads))
        self.assertFalse(failures)
        self.assertEqual(len(self.ledger.events()), 12)
        self.ledger.verify()

    def test_batch_02_midi_matches_its_frozen_generator(self):
        generator = ev.PROJECT / "presets/review/batch-02/r1/make_midi.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Verified glass-current-r1.mid", result.stdout)
        self.assertIn("Verified pulse-lattice-r1.mid", result.stdout)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen performances", result.stderr)

    def test_batch_02_r2_lineage_and_focused_changes(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        expected = {
            "glass-current": {"coupling": {"before": .52, "after": .46}, "stress": {"before": .09, "after": .065}},
            "pulse-lattice": {
                "lfo_fm_wave": {"before": 1.0, "after": 4.0},
                "lfo_fm_division": {"before": 0.0, "after": 8.0},
                "fm_route_3_source": {"before": 0.0, "after": 1.0},
                "fm_route_3_target": {"before": 0.0, "after": 1.0},
                "fm_route_3_amount": {"before": 0.0, "after": .11}
            }
        }
        with self.ledger.locked():
            for family, changes in expected.items():
                for revision in ("r1", "r2"):
                    entry = entries[f"review:{family}:{revision}"]
                    self.ledger.register(entry, self.render_fixture(entry))
                child = self.ledger.events()[-1]["payload"]
                self.assertEqual(child["parent"], f"review:{family}:r1")
                self.assertEqual(child["parameter_changes"], changes)
                before = self.root / "revisions" / family / "r1/performance.mid"
                after = self.root / "revisions" / family / "r2/performance.mid"
                self.assertEqual(before.read_bytes(), after.read_bytes())
            self.ledger.verify()

    def test_batch_03_midi_and_six_category_coverage(self):
        entries = ev.validate_builtins()["entries"]
        self.assertEqual({e["category"] for e in entries}, set(ev.CATEGORIES))
        generator = ev.PROJECT / "presets/review/batch-03/r1/make_midi.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Verified hollow-bronze-r1.mid", result.stdout)
        self.assertIn("Verified fault-bloom-r1.mid", result.stdout)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen performances", result.stderr)

    def test_category_coverage_does_not_imply_approval(self):
        with self.ledger.locked():
            for i, category in enumerate(ev.CATEGORIES):
                self.ledger.append("revision", {"id": f"review:category-{i}:r1", "category": category, "program": i + 4})
            state = self.ledger.rebuild()
        self.assertTrue(all(r["status"] == "awaiting-trial" for r in state["revisions"].values()))
        self.assertIn("None — all six categories have sketches; trial feedback and final library approval remain separate.", (self.root / "CURRENT.md").read_text())

    def test_material_r2_preserves_strike_and_performance(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for revision in ("r1", "r2"):
                entry = entries[f"review:hollow-bronze:{revision}"]
                self.ledger.register(entry, self.render_fixture(entry))
            child = self.ledger.events()[-1]["payload"]
            self.assertEqual(child["parent"], "review:hollow-bronze:r1")
            self.assertEqual(child["parameter_changes"], {"fm_depth": {"before": .42, "after": .22}, "fm_envelope_amount": {"before": 2.2, "after": 2.4}})
            self.assertEqual((self.root / "revisions/hollow-bronze/r1/performance.mid").read_bytes(), (self.root / "revisions/hollow-bronze/r2/performance.mid").read_bytes())
            self.ledger.verify()

    def test_batch_04_contrasts_are_new_families(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        for identity, contrast in (("review:prism-pulse:r1", "review:soft-alloy:r2"), ("review:felt-weight:r1", "review:tension-core:r2")):
            entry = entries[identity]
            self.assertEqual(entry["contrast_with"], contrast)
            self.assertNotIn("parent", entry)
            self.assertNotEqual(entry["family"], entries[contrast]["family"])
        generator = ev.PROJECT / "presets/review/batch-04/r1/make_midi.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Verified prism-pulse-r1.mid", result.stdout)
        self.assertIn("Verified felt-weight-r1.mid", result.stdout)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen performances", result.stderr)

    def test_collection_guidance_is_visible_on_resume(self):
        with self.ledger.locked():
            event = self.ledger.append("guidance", {"text": "Showcase range and diversity.", "interpretation": "Prioritize distinct behavior, not near-variants."}, "user")
            state = self.ledger.rebuild()
        self.assertEqual(state["guidance"][0]["event"], event["id"])
        brief = (self.root / "CURRENT.md").read_text()
        self.assertIn("## Collection guidance", brief)
        self.assertIn("user: Showcase range and diversity.", brief)
        self.assertIn("Assistant interpretation: Prioritize distinct behavior, not near-variants.", brief)

    def test_contrast_registration_preserves_relationship(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for identity in ("review:soft-alloy:r1", "review:soft-alloy:r2"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            entry = entries["review:prism-pulse:r1"]
            renders = self.render_fixture(entry)
            with self.assertRaises(ValueError):
                self.ledger.register({**entry, "contrast_with": "unknown"}, renders)
            self.assertFalse((self.root / "revisions/prism-pulse/r1").exists())
            self.ledger.register(entry, renders)
            candidate = self.ledger.events()[-1]["payload"]
            self.assertIsNone(candidate["parent"])
            self.assertEqual(candidate["contrast_with"], "review:soft-alloy:r2")
            self.ledger.verify()

    def test_prism_r2_keeps_pulse_and_contrast_lineage(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for identity in ("review:soft-alloy:r1", "review:soft-alloy:r2", "review:prism-pulse:r1", "review:prism-pulse:r2"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            child = self.ledger.events()[-1]["payload"]
            self.assertEqual(child["parent"], "review:prism-pulse:r1")
            self.assertEqual(child["contrast_with"], "review:soft-alloy:r2")
            self.assertEqual(child["parameter_changes"], {
                "fm_ratio": {"before": 2.5, "after": 2.0}, "fm_route_1_amount": {"before": .035, "after": 0.0},
                "fm_shape_amount": {"before": .45, "after": .18}, "fm_operator_mix": {"before": .09, "after": .04},
                "output": {"before": -4.0, "after": -5.8}})
            self.assertEqual((self.root / "revisions/prism-pulse/r1/performance.mid").read_bytes(), (self.root / "revisions/prism-pulse/r2/performance.mid").read_bytes())
            self.ledger.verify()

    def test_name_only_revision_preserves_sound_and_midi(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for identity in ("review:soft-alloy:r1", "review:soft-alloy:r2", "review:prism-pulse:r1", "review:prism-pulse:r2"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            entry = entries["review:prism-pulse:r3"]
            renders = self.render_fixture(entry)
            with self.assertRaises(ValueError):
                self.ledger.register({**entry, "parent": "review:prism-pulse:r1"}, renders)
            self.assertFalse((self.root / "revisions/prism-pulse/r3").exists())
            self.ledger.register(entry, renders)
            renamed = self.ledger.events()[-1]["payload"]
            self.assertEqual(renamed["parent"], "review:prism-pulse:r2")
            self.assertEqual(renamed["revision_kind"], "rename-only")
            self.assertEqual(renamed["display_family"], "Velvet Pulse")
            self.assertEqual(renamed["parameter_changes"], {})
            self.assertEqual((self.root / "revisions/prism-pulse/r2/performance.mid").read_bytes(), (self.root / "revisions/prism-pulse/r3/performance.mid").read_bytes())
            self.ledger.verify()

    def test_batch_05_contrasts_and_frozen_performances(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        self.assertEqual(entries["review:reed-snap:r1"]["contrast_with"], "review:glass-current:r2")
        self.assertEqual(entries["review:undertow-grid:r1"]["contrast_with"], "review:pulse-lattice:r2")
        generator = ev.PROJECT / "presets/review/batch-05/r1/make_midi.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Verified reed-snap-r1.mid", result.stdout)
        self.assertIn("Verified undertow-grid-r1.mid", result.stdout)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen performances", result.stderr)

    def test_reed_r2_preserves_drive_and_articulation(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for identity in ("review:glass-current:r1", "review:glass-current:r2", "review:reed-snap:r1", "review:reed-snap:r2"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            child = self.ledger.events()[-1]["payload"]
            self.assertEqual(child["parent"], "review:reed-snap:r1")
            self.assertEqual(child["contrast_with"], "review:glass-current:r2")
            self.assertEqual(child["parameter_changes"], {
                "fm_shape": {"before": 2.0, "after": 1.0}, "fm_shape_amount": {"before": .6, "after": .35},
                "fm_route_2_source": {"before": 0.0, "after": 3.0}, "fm_route_2_target": {"before": 0.0, "after": 2.0},
                "fm_route_2_amount": {"before": 0.0, "after": .3}, "output": {"before": -2.5, "after": -2.65}})
            self.assertEqual((self.root / "revisions/reed-snap/r1/performance.mid").read_bytes(), (self.root / "revisions/reed-snap/r2/performance.mid").read_bytes())
            self.ledger.verify()

    def test_batch_06_contrasts_and_frozen_performances(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for identity in ("review:hollow-bronze:r1", "review:hollow-bronze:r2", "review:fault-bloom:r1", "review:skin-knock:r1", "review:orbit-shards:r1"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for identity, contrast in (("review:skin-knock:r1", "review:hollow-bronze:r2"), ("review:orbit-shards:r1", "review:fault-bloom:r1")):
                candidate = state["revisions"][identity]
                self.assertIsNone(candidate["parent"])
                self.assertEqual(candidate["contrast_with"], contrast)
                self.assertEqual(candidate["status"], "awaiting-trial")
        generator = ev.PROJECT / "presets/review/batch-06/r1/make_midi.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Verified skin-knock-r1.mid", result.stdout)
        self.assertIn("Verified orbit-shards-r1.mid", result.stdout)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen performances", result.stderr)

    def test_artist_lens_registration_and_frozen_performances(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        studies = (("review:taffy-coil:r1", "SOPHIE", "review:skin-knock:r1"),
                   ("review:stillwater:r1", "Brian Eno", "review:soft-alloy:r2"),
                   ("review:crooked-velvet:r1", "Flying Lotus", "review:felt-weight:r1"))
        with self.ledger.locked():
            for identity in ("review:hollow-bronze:r1", "review:hollow-bronze:r2", "review:skin-knock:r1", "review:soft-alloy:r1", "review:soft-alloy:r2", "review:tension-core:r1", "review:tension-core:r2", "review:felt-weight:r1"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            for identity, lens, contrast in studies:
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
                candidate = self.ledger.events()[-1]["payload"]
                self.assertEqual(candidate["design_lens"], lens)
                self.assertEqual(candidate["contrast_with"], contrast)
                self.assertIsNone(candidate["parent"])
                self.assertEqual(self.ledger.rebuild()["revisions"][identity]["status"], "awaiting-trial")
            self.ledger.verify()
        generator = ev.PROJECT / "presets/review/batch-07/r1/make_midi.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        for family in ("taffy-coil", "stillwater", "crooked-velvet"):
            self.assertIn("Verified " + family + "-r1.mid", result.stdout)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen performances", result.stderr)

    def test_crooked_r2_preserves_concept_and_frozen_midi(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for identity in ("review:tension-core:r1", "review:tension-core:r2", "review:felt-weight:r1", "review:crooked-velvet:r1", "review:crooked-velvet:r2"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            child = self.ledger.events()[-1]["payload"]
            self.assertEqual(child["parent"], "review:crooked-velvet:r1")
            self.assertEqual(child["design_lens"], "Flying Lotus")
            self.assertEqual(child["contrast_with"], "review:felt-weight:r1")
            self.assertEqual(child["parameter_changes"], {
                "fm_depth": {"before": .8, "after": .4}, "fm_ratio": {"before": 1.25, "after": 1.0},
                "fm_operator_mix": {"before": .11, "after": .035}, "fm_route_2_amount": {"before": .13, "after": .04},
                "fm_route_3_target": {"before": 1.0, "after": 0.0}, "output": {"before": 0.0, "after": -1.675}})
            self.assertEqual((self.root / "revisions/crooked-velvet/r1/performance.mid").read_bytes(), (self.root / "revisions/crooked-velvet/r2/performance.mid").read_bytes())
            self.ledger.verify()

    def test_batch_08_artist_lenses_and_frozen_performances(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        studies = (("review:replica-voice:r1", "Oneohtrix Point Never", "review:glass-current:r2"),
                   ("review:tendon-drift:r1", "Arca", "review:fault-bloom:r1"),
                   ("review:socket-dance:r1", "Body Meat", "review:undertow-grid:r1"))
        with self.ledger.locked():
            for identity in ("review:glass-current:r1", "review:glass-current:r2", "review:fault-bloom:r1", "review:pulse-lattice:r1", "review:pulse-lattice:r2", "review:undertow-grid:r1"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            for identity, lens, contrast in studies:
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
                candidate = self.ledger.events()[-1]["payload"]
                self.assertEqual(candidate["design_lens"], lens)
                self.assertEqual(candidate["contrast_with"], contrast)
                self.assertIsNone(candidate["parent"])
                self.assertEqual(self.ledger.rebuild()["revisions"][identity]["status"], "awaiting-trial")
            self.ledger.verify()
        generator = ev.PROJECT / "presets/review/batch-08/r1/make_midi.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        for family in ("replica-voice", "tendon-drift", "socket-dance"):
            self.assertIn("Verified " + family + "-r1.mid", result.stdout)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen performances", result.stderr)

    def test_batch_08_tail_only_r2_preserves_voice_and_midi(self):
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        with self.ledger.locked():
            for identity in ("review:glass-current:r1", "review:glass-current:r2", "review:pulse-lattice:r1", "review:pulse-lattice:r2", "review:undertow-grid:r1"):
                entry = entries[identity]
                self.ledger.register(entry, self.render_fixture(entry))
            for family, before, after, lens in (("replica-voice", .9, .4, "Oneohtrix Point Never"), ("socket-dance", .1, .045, "Body Meat")):
                for revision in ("r1", "r2"):
                    entry = entries[f"review:{family}:{revision}"]
                    self.ledger.register(entry, self.render_fixture(entry))
                child = self.ledger.events()[-1]["payload"]
                self.assertEqual(child["parent"], f"review:{family}:r1")
                self.assertEqual(child["design_lens"], lens)
                self.assertEqual(child["parameter_changes"], {"amp_release": {"before": before, "after": after}})
                self.assertEqual((self.root / "revisions" / family / "r1/performance.mid").read_bytes(), (self.root / "revisions" / family / "r2/performance.mid").read_bytes())
            self.ledger.verify()

    @unittest.skipUnless((ev.PROJECT / "evaluation/collection-review/pass-001/prepare.py").is_file(), "Private local collection study is not distributed")
    def test_collection_packet_preserves_sources_and_provisional_status(self):
        script = ev.PROJECT / "evaluation/collection-review/pass-001/prepare.py"
        output = self.root / "collection-packet"
        result = subprocess.run([sys.executable, str(script), "--output", str(output)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        manifest = json.loads((output / "manifest.json").read_text())
        self.assertEqual(manifest["status"], "prepared-not-listened")
        self.assertEqual(manifest["role_author"], "assistant-provisional")
        self.assertEqual(manifest["approval"], "none")
        self.assertEqual(len(manifest["groups"]), 6)
        entries = {e["id"]: e for e in ev.validate_builtins()["entries"]}
        selected = set()
        for group in manifest["groups"]:
            self.assertEqual(len(group["candidates"]), 3)
            for candidate in group["candidates"]:
                entry = entries[candidate["revision"]]
                selected.add(entry["family"])
                source = ev.PROJECT / "presets/review" / entry["preset"]
                folder = output / group["folder"]
                self.assertEqual(ev.digest(folder / source.name), entry["sha256"])
                self.assertEqual(ev.digest(folder / (source.stem + ".mid")), group["common_midi_sha256"])
                self.assertEqual(candidate["program"], entry["program"])
                self.assertIn(candidate["provisional_role"], ("anchor", "character", "edge-case"))
        self.assertEqual(len(selected), 18)
        before = ev.digest(output / "manifest.json")
        result = subprocess.run([sys.executable, str(script), "--output", str(output)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never overwrite evidence", result.stderr)
        self.assertEqual(before, ev.digest(output / "manifest.json"))

    def test_frontier_studies_preserve_axis_and_frozen_generators(self):
        entries = ev.validate_builtins()["entries"]
        studies = [e for e in entries if e["preset"].startswith("batch-09/r1/")]
        self.assertEqual(len(studies), 6)
        self.assertEqual(len({e["study_axis"] for e in studies}), 6)
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for entry in studies:
                candidate = state["revisions"][entry["id"]]
                self.assertEqual(candidate["study_axis"], entry["study_axis"])
                self.assertEqual(candidate["contrast_with"], entry["contrast_with"])
                self.assertIsNone(candidate["parent"])
                self.assertEqual(candidate["status"], "awaiting-trial")
        generator = ev.PROJECT / "presets/review/batch-09/r1/make_studies.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 13)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen presets/performances", result.stderr)

    def test_tine_keys_rename_preserves_sound_midi_and_lineage(self):
        entries = ev.validate_builtins()["entries"]
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            child = self.ledger.rebuild()["revisions"]["review:cutoff-alloy:r2"]
            self.assertEqual(child["parent"], "review:cutoff-alloy:r1")
            self.assertEqual(child["revision_kind"], "rename-only")
            self.assertEqual(child["display_family"], "Tine Keys")
            self.assertEqual(child["parameter_changes"], {})
            self.assertEqual((self.root / "revisions/cutoff-alloy/r1/performance.mid").read_bytes(), (self.root / "revisions/cutoff-alloy/r2/performance.mid").read_bytes())
            self.ledger.verify()
        generator = ev.PROJECT / "presets/review/batch-09/r2/make_rename.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 3)

    def test_archetype_axes_and_frozen_generator(self):
        entries = ev.validate_builtins()["entries"]
        archetypes = [e for e in entries if e["preset"].startswith("batch-10/r1/")]
        self.assertEqual(len(archetypes), 4)
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for entry in archetypes:
                child = state["revisions"][entry["id"]]
                self.assertIsNone(child["parent"])
                self.assertEqual(child["study_axis"], entry["study_axis"])
                self.assertEqual(child["status"], "awaiting-trial")
        generator = ev.PROJECT / "presets/review/batch-10/r1/make_archetypes.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 9)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen presets/performances", result.stderr)


    def test_percussion_axes_and_frozen_generator(self):
        entries = ev.validate_builtins()["entries"]
        drums = [e for e in entries if e["preset"].startswith("batch-11/r1/")]
        self.assertEqual([e["program"] for e in drums], [44, 45, 46])
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for entry in drums:
                child = state["revisions"][entry["id"]]
                self.assertIsNone(child["parent"])
                self.assertEqual(child["study_axis"], entry["study_axis"])
                self.assertEqual(child["status"], "awaiting-trial")
                self.assertEqual(child["category"], "Physical materials")
                preset = json.loads((ev.PROJECT / child["source"]).read_text())
                values = preset["values"]
                self.assertEqual(values["amp_sustain"], 0)
                self.assertEqual(values["autobend_enabled"], 1)
                self.assertEqual(values["autobend_phrase"], 0)
                self.assertEqual(values["legato"], 0)
                self.assertEqual(values["glide_enabled"], 0)
                self.assertEqual(values["fm_operator_mix"], 0)
                self.assertFalse(preset["chord"])
        generator = ev.PROJECT / "presets/review/batch-11/r1/make_drums.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 7)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen presets/performances", result.stderr)


    def test_tom_r2_preserves_midi_and_rebalances_attack_body(self):
        import runpy
        generator = ev.PROJECT / "presets/review/batch-11/r2/make_refinement.py"
        recipe = runpy.run_path(str(generator))
        changes = recipe["CHANGES"]
        entries = ev.validate_builtins()["entries"]
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            parent = state["revisions"]["review:bowl-toms:r1"]
            child = state["revisions"]["review:bowl-toms:r2"]
            self.assertEqual(child["parent"], parent["id"])
            self.assertEqual(child["program"], 47)
            self.assertEqual(child["status"], "awaiting-trial")
            self.assertEqual(set(child["parameter_changes"]), set(changes))
            for key, value in changes.items():
                self.assertEqual(child["parameter_changes"][key]["after"], value)
            self.assertEqual((self.root / "revisions/bowl-toms/r1/performance.mid").read_bytes(), (self.root / "revisions/bowl-toms/r2/performance.mid").read_bytes())
            old = json.loads((ev.PROJECT / parent["source"]).read_text())
            new = json.loads((ev.PROJECT / child["source"]).read_text())
            self.assertNotEqual(old["id"], new["id"])
            for key in ("mono", "chord"):
                self.assertEqual(old[key], new[key])
            for key in ("output", "pitch", "amp_sustain", "glide_enabled", "legato", "autobend_enabled", "autobend_target", "autobend_phrase", "fm_operator_mix"):
                self.assertEqual(old["values"][key], new["values"][key])
            self.assertLess(new["values"]["amp_decay"], old["values"]["amp_decay"])
            self.assertLess(new["values"]["fm_depth"], old["values"]["fm_depth"])
            self.assertGreater(new["values"]["fm_envelope_amount"], old["values"]["fm_envelope_amount"])
            self.assertEqual(new["values"]["space_mix"], 0)
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 3)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen revisions/performances", result.stderr)


    def test_transitions_preserve_roles_and_frozen_generator(self):
        entries = ev.validate_builtins()["entries"]
        transitions = [e for e in entries if e["preset"].startswith("batch-12/r1/")]
        self.assertEqual([e["program"] for e in transitions], [48, 49, 50])
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for entry in transitions:
                child = state["revisions"][entry["id"]]
                self.assertIsNone(child["parent"])
                self.assertEqual(child["status"], "awaiting-trial")
                self.assertEqual(child["design_lens"], entry["design_lens"])
                self.assertEqual(child["study_axis"], entry["study_axis"])
        generator = ev.PROJECT / "presets/review/batch-12/r1/make_transitions.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 7)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen presets/performances", result.stderr)


    @unittest.skipUnless((ev.PROJECT / "evaluation/non-pitch-transitions/pass-001/harmonic-probes").is_dir(), "Private local mechanism evidence is not distributed")
    def test_non_pitch_candidates_match_measured_sources_and_frozen_generator(self):
        import runpy
        generator = ev.PROJECT / "presets/review/batch-13/r1/make_non_pitch.py"
        recipe = runpy.run_path(str(generator))
        sketches = {s["family"]: s for s in recipe["sketches"]()}
        entries = ev.validate_builtins()["entries"]
        candidates = [e for e in entries if e["preset"].startswith("batch-13/r1/")]
        self.assertEqual([e["program"] for e in candidates], [51, 52, 53])
        for entry in candidates:
            preset = json.loads((ev.PROJECT / "presets/review" / entry["preset"]).read_text())
            sketch = sketches[entry["family"]]
            probe = json.loads((ev.PROJECT / "evaluation/non-pitch-transitions/pass-001/harmonic-probes" / (sketch["probe"] + ".nitridepreset")).read_text())
            self.assertEqual({k: v for k, v in preset["values"].items() if k != "output"}, {k: v for k, v in probe["values"].items() if k != "output"})
            recipe["fixed_paths"](preset["values"])
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for entry in candidates:
                self.assertIsNone(state["revisions"][entry["id"]]["parent"])
                self.assertEqual(state["revisions"][entry["id"]]["status"], "awaiting-trial")
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 7)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen presets/performances", result.stderr)

    @unittest.skipUnless((ev.PROJECT / "evaluation/non-pitch-transitions/pass-001/context-design.json").is_file(), "Private local context study is not distributed")
    def test_non_pitch_context_and_causal_ablations(self):
        import runpy
        folder = ev.PROJECT / "evaluation/non-pitch-transitions/pass-001"
        recipe = runpy.run_path(str(folder / "prepare_context.py"))
        design = json.loads((folder / "context-design.json").read_text())
        self.assertEqual(design["bars"], 8)
        self.assertEqual(design["boundary_seconds"], 8)
        self.assertEqual(design["listener_status"], "prepared-not-listened; independent producer not recruited")
        for family, changes in recipe["ABLATIONS"].items():
            full = json.loads((folder / "context-inputs/fx" / (family + "-full.nitridepreset")).read_text())
            ablated = json.loads((folder / "context-inputs/fx" / (family + "-ablated.nitridepreset")).read_text())
            actual = {k: v for k, v in ablated["values"].items() if v != full["values"][k]}
            self.assertEqual(actual, changes)
            self.assertEqual(full["values"]["output"], ablated["values"]["output"])
            self.assertEqual((folder / "context-inputs/fx" / (family + "-full.mid")).read_bytes(), (folder / "context-inputs/fx" / (family + "-ablated.mid")).read_bytes())
        for stem, definition in design["stems"].items():
            self.assertEqual((folder / "context-inputs/stems" / (stem + ".nitridepreset")).read_bytes(), (ev.PROJECT / definition["source"]).read_bytes())

    @unittest.skipUnless((ev.PROJECT / "evaluation/non-pitch-transitions/pass-001/audio_evidence.py").is_file(), "Private local spectral study is not distributed")
    def test_non_pitch_fft_and_raw_probe_change_sets(self):
        import runpy
        folder = ev.PROJECT / "evaluation/non-pitch-transitions/pass-001"
        analysis = runpy.run_path(str(folder / "audio_evidence.py"))
        analysis["self_test"]()
        recipe = runpy.run_path(str(folder / "make_probes.py"))
        for variant in ("original", "harmonic-bed"):
            cases = recipe["cases"](variant)
            values = {name: v for name, v, _, _, _ in cases}
            for name, v, parent, expected, _ in cases:
                if parent:
                    self.assertEqual({k: x for k, x in v.items() if values[parent][k] != x}, expected)


    @unittest.skipUnless((ev.PROJECT / "evaluation/dx7-classics/pass-001/sources/rom1a.syx").is_file(), "Yamaha ROM reference data is not distributed")
    def test_factory_rom_and_reference_pitch_mapping(self):
        import runpy
        folder = ev.PROJECT / "evaluation/dx7-classics/pass-001"
        data = (folder / "sources/rom1a.syx").read_bytes()
        self.assertEqual(len(data), 4104)
        self.assertEqual((sum(data[6:-2]) + data[-2]) & 127, 0)
        factory = json.loads((folder / "factory-analysis.json").read_text())
        decoder = runpy.run_path(str(folder / "decode_factory.py"))["decode"]
        for voice in factory["voices"]:
            parsed = decoder((folder / "selected-factory" / (voice["file"] + ".vmem")).read_bytes())
            self.assertEqual(parsed["operators"], voice["operators"])
            self.assertEqual(parsed["algorithm_display"], voice["algorithm_display"])
            self.assertEqual(parsed["lfo_pitch_depth"], 0)
            self.assertEqual(parsed["lfo_amp_depth"], 0)
            self.assertTrue(all(op["amplitude_mod_sensitivity"] == 0 for op in parsed["operators"]))
        bass = next(v for v in factory["voices"] if v["file"] == "bass1")
        carrier = next(op for op in bass["operators"] if op["operator"] == 1)
        self.assertEqual(bass["transpose_semitones"], -12)
        self.assertEqual(carrier["nominal_ratio_or_hz"], .5)
        recipe = runpy.run_path(str(ev.PROJECT / "presets/review/batch-14/r1/make_classics.py"))
        sketch = next(s for s in recipe["sketches"]() if s["family"] == "classic-bass")
        self.assertEqual(sketch["reference_note_offset"] + bass["transpose_semitones"] - 12, 0)

    def test_classic_frozen_generator_and_append_only_roles(self):
        entries = ev.validate_builtins()["entries"]
        classics = [e for e in entries if e["preset"].startswith("batch-14/r1/")]
        self.assertEqual([e["program"] for e in classics], [54, 55, 56])
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for entry in classics:
                child = state["revisions"][entry["id"]]
                self.assertEqual(child["study_axis"], entry["study_axis"])
                self.assertIsNone(child["parent"])
                self.assertEqual(child["status"], "awaiting-trial")
                preset = json.loads((ev.PROJECT / child["source"]).read_text())
                for key in ("space_mix", "stress", "glide_enabled", "autobend_enabled", "filter_amount"):
                    self.assertEqual(preset["values"][key], 0)
        generator = ev.PROJECT / "presets/review/batch-14/r1/make_classics.py"
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 7)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen presets/performances", result.stderr)
        self.assertNotIn("msfa", (ev.PROJECT / "CMakeLists.txt").read_text().lower())


    @unittest.skipUnless((ev.PROJECT / "evaluation/dx7-classics/pass-002/factory-analysis.json").is_file(), "Yamaha reference study is not distributed")
    def test_requested_rom_slots_and_modulation_qualifications(self):
        folder = ev.PROJECT / "evaluation/dx7-classics/pass-002"
        voices = json.loads((folder / "factory-analysis.json").read_text())["voices"]
        self.assertEqual([v["factory_slot_one_based"] for v in voices], [30, 25, 23])
        self.assertEqual([v["canonical_name"] for v in voices], ["VOICE 1", "ORCH-CHIME", "KOTO"])
        self.assertEqual([v["algorithm_display"] for v in voices], [7, 5, 2])
        self.assertTrue(voices[0]["modulation_audit"]["source_amp_mod_active"])
        self.assertFalse(voices[2]["modulation_audit"]["source_amp_mod_active"])
        self.assertEqual(voices[2]["lfo_amp_depth"], 15)
        self.assertTrue(all(v["lfo_pitch_depth"] > 0 for v in voices))
        reference = json.loads((folder / "reference-audio/report.json").read_text())
        self.assertEqual([r["partial_operator_ams"] for r in reference], [True, False, False])
        for item in reference:
            self.assertTrue(item["deterministic"])
            self.assertTrue(all(m["shared_lfo"] and m["pitch_lfo_rendered"] for m in item["harness_metrics"]))
        guard = json.loads((folder / "partial-reference-guard.json").read_text())
        self.assertNotEqual(guard["returncode"], 0)
        self.assertFalse(guard["output_created"])

    @unittest.skipUnless((ev.PROJECT / "evaluation/dx7-classics/pass-002/comparison-inputs").is_dir(), "Private source/anchor comparisons are not distributed; generators are tested separately")
    def test_requested_classics_frozen_generator_and_root_alignment(self):
        import runpy
        generator = ev.PROJECT / "presets/review/batch-15/r1/make_classics.py"
        recipe = runpy.run_path(str(generator))
        sketches = recipe["sketches"]()
        entries = ev.validate_builtins()["entries"]
        requested = [e for e in entries if e["preset"].startswith("batch-15/r1/")]
        self.assertEqual([e["program"] for e in requested], [57, 58, 59])
        with self.ledger.locked():
            for entry in entries:
                self.ledger.register(entry, self.render_fixture(entry))
            state = self.ledger.verify()
            for entry in requested:
                item = state["revisions"][entry["id"]]
                self.assertIsNone(item["parent"])
                self.assertEqual(item["status"], "awaiting-trial")
                self.assertEqual(item["study_axis"], entry["study_axis"])
        folder = ev.PROJECT / "evaluation/dx7-classics/pass-002/comparison-inputs"
        for sketch in sketches:
            family = sketch["family"]
            full = json.loads((folder / (family + "-study.nitridepreset")).read_text())
            self.assertEqual(full["values"]["lfo_pitch_enabled"], 1)
            self.assertEqual(full["values"]["lfo_pitch_sync"], 0)
            if family == "orch-chime":
                self.assertEqual(full["values"]["pitch"], -12)
                self.assertEqual(sketch["anchor_note_offset"], -12)
                self.assertAlmostEqual(full["values"]["fm_ratio"], 3.14 / .5)
                notes = [(b, d, n - 12, v) for b, d, n, v in sketch["notes"]]
                expected = recipe["base"].encoder.performance("Orch Chime anchor / explicit MIDI-12 sounding-root alignment", sketch["bpm"], notes)
                self.assertEqual((folder / (family + "-anchor.mid")).read_bytes(), expected)
            else:
                self.assertEqual((folder / (family + "-study.mid")).read_bytes(), (folder / (family + "-anchor.mid")).read_bytes())
        result = subprocess.run([sys.executable, str(generator), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count("Verified "), 7)
        result = subprocess.run([sys.executable, str(generator)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("never replace frozen presets/performances", result.stderr)


if __name__ == "__main__":
    unittest.main()
