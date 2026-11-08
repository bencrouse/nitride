"""Selective, append-only factory filter revisions and honest comparison evidence."""
from array import array
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import wave

import evaluation as ev
import factory_presets as fp
import factory_filter_comparisons as comparisons

BATCH = fp.ROOT/"presets/review/batch-22"
spec = importlib.util.spec_from_file_location("factory_filter_recipes", BATCH/"make_factory_filters.py")
recipes = importlib.util.module_from_spec(spec); spec.loader.exec_module(recipes)


class FactoryFilterAuditions(unittest.TestCase):
    def test_original_registry_prefix_and_selection_remain_frozen(self):
        registry = ev.validate_builtins()["entries"]
        self.assertEqual(hashlib.sha256(ev.canonical(registry[:112])).hexdigest(),
                         "cbffd66820772dfe5b67c0b9428c4cd0bee45214883698c1e70f3a7af328fea7")
        self.assertEqual([e["program"] for e in registry[112:]], list(range(116, 132)))
        _, selected = fp.validate_selection(); self.assertEqual(len(selected), 77)
        self.assertTrue({e["id"] for e in registry[112:]} <= {e["id"] for e in selected})
        self.assertFalse({r["parent"]["id"] for r in recipes.recipes()} & {e["id"] for e in selected})

    def test_only_three_filter_controls_change_and_parent_midi_is_exact(self):
        expected = recipes.files(); entries = ev.validate_builtins()["entries"][112:]
        self.assertEqual(len(entries), 16)
        for recipe, entry in zip(recipes.recipes(), entries):
            a, b = recipe["original"], recipe["definition"]
            self.assertEqual(entry["parent"], recipe["parent"]["id"])
            self.assertEqual(entry["family"], recipe["family"])
            self.assertEqual(entry["revision"], recipe["revision"])
            self.assertEqual(a["mono"], b["mono"]); self.assertEqual(a["chord"], b["chord"])
            changed = {k for k in set(a["values"]) | set(b["values"]) if a["values"].get(k) != b["values"].get(k)}
            self.assertEqual(changed, {"filter_family", "rage_character", "rage_amount"})
            self.assertEqual(b["version"], 7); self.assertNotEqual(a["id"], b["id"])
            path = fp.ROOT/"presets/review"/entry["preset"]
            self.assertEqual(path.read_bytes(), expected[recipe["revision"]+"/"+path.name])
            self.assertEqual(path.with_suffix(".mid").read_bytes(), recipe["source"].with_suffix(".mid").read_bytes())
        self.assertEqual({r["sketch"]["character"] for r in recipes.recipes()}, set(recipes.CHARACTERS))

    def test_generator_verifies_but_cannot_overwrite_frozen_auditions(self):
        result = subprocess.run([sys.executable, str(BATCH/"make_factory_filters.py"), "--verify"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr); self.assertEqual(result.stdout.count("Verified "), 34)
        self.assertNotEqual(subprocess.run([sys.executable, str(BATCH/"make_factory_filters.py"), "--create"], capture_output=True).returncode, 0)
        for revision, count in (("r2", 7), ("r3", 9)):
            batch = json.loads((BATCH/revision/"batch.json").read_text())
            self.assertEqual(len(batch["sketches"]), count)
            self.assertEqual(batch["revision"], revision)

    def test_private_three_state_staging_preserves_the_family_control(self):
        for mode in comparisons.MODES:
            files = recipes.files(mode)
            for recipe in recipes.recipes():
                preset = json.loads(files[recipe["revision"]+"/"+recipe["stem"]+".nitridepreset"])
                if mode == "classic": self.assertEqual(preset, recipe["original"])
                else:
                    self.assertEqual(preset["values"]["filter_family"], 1)
                    self.assertEqual(preset["values"]["rage_amount"], 0 if mode == "clean" else recipe["sketch"]["rage_amount"])

    def test_gain_alignment_is_not_loudness_or_listening_approval(self):
        a = array("i", [100, -200, 400, -800]*10)
        self.assertLess(comparisons.residual(a, array("i", (x*2 for x in a))), -100)
        self.assertGreater(comparisons.residual(a, array("i", [100, 200, 400, 800]*10)), -5)
        with self.assertRaises(ValueError): comparisons.residual(a, array("i", [0]*len(a)))
        with tempfile.TemporaryDirectory(prefix="nitride-factory-comparison-") as folder:
            path = Path(folder)/"sequence.wav"
            comparisons.sequence(path, [a]*3, [.5, .5, .5])
            data = path.read_bytes(); self.assertEqual(data[20:22], struct.pack("<H", 3))
            self.assertEqual(len(data), 44+(3*len(a)+96000)*4)
            with self.assertRaises(FileExistsError): comparisons.sequence(path, [a]*3, [1]*3)

    def test_review_wav_reader_rejects_other_layouts_and_preserves_signed_pcm(self):
        with tempfile.TemporaryDirectory(prefix="nitride-factory-comparison-") as folder:
            path = Path(folder)/"pcm.wav"; values = [-8388608, -1, 0, 8388607]
            with wave.open(str(path), "wb") as output:
                output.setnchannels(2); output.setsampwidth(3); output.setframerate(48000)
                output.writeframes(b"".join(v.to_bytes(3, "little", signed=True) for v in values))
            self.assertEqual(list(comparisons.pcm(path)), values)
            with wave.open(str(path), "wb") as output:
                output.setnchannels(1); output.setsampwidth(2); output.setframerate(48000); output.writeframes(b"\0\0")
            with self.assertRaises(ValueError): comparisons.pcm(path)


if __name__ == "__main__": unittest.main()
