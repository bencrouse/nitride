#!/usr/bin/env python3
"""Qualification gates never turn missing/manual/over-budget evidence into a pass."""
import json
from pathlib import Path
import tempfile
import unittest

import qualification as q


class QualificationTests(unittest.TestCase):
    def case(self, **changes):
        return {"key": "lead/44100/128", "budgetMicros": 1000, "p99Micros": 750,
                "medianPassLoadPercent": 60, "minimumVoices": 1, "maximumVoices": 1, "overBudgetBlocks": 0, **changes}

    def test_headroom_and_zero_overdue_are_both_required(self):
        self.assertTrue(q.assess(self.case())["passed_offline_gate"])
        self.assertFalse(q.assess(self.case(p99Micros=801))["passed_offline_gate"])
        self.assertFalse(q.assess(self.case(overBudgetBlocks=1))["passed_offline_gate"])
        with self.assertRaises(ValueError): q.assess(self.case(p99Micros=float("nan")))
        with self.assertRaises(ValueError): q.assess(self.case(budgetMicros=0))

    def test_fixed_unique_matrix_and_no_false_missing_or_manual_pass(self):
        self.assertEqual(len(q.matrix()), 29)
        with tempfile.TemporaryDirectory() as folder:
            summary = q.summarize(folder)
            self.assertTrue(all(c["status"] == "missing-not-qualified" for c in summary["cases"]))
            self.assertEqual(summary["logic_project_reopen"], "manual-pending")
            self.assertEqual(summary["logic_soak"], "manual-pending")

    def test_wrong_measurement_settings_are_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            report = {"format": "nitride-performance", "settings": {"passes": 1, "seconds": .25, "warmup": .5},
                      "machine": {"configuration": "Release"}, "cases": [self.case(sampleRate=44100, blockSize=128, workload="lead")]}
            path = Path(folder) / "ordinary-44100-128-lead.json"; path.write_text(json.dumps(report))
            with self.assertRaisesRegex(ValueError, "duration/warmup/passes"): q.summarize(folder)

    def test_truncated_returned_matrix_cannot_hide_heavy_cases(self):
        with tempfile.TemporaryDirectory() as folder:
            (Path(folder) / "matrix.tsv").write_text("ordinary\t44100\t128\tlead\n")
            with self.assertRaisesRegex(ValueError, "complete pinned matrix"): q.summarize(folder)


if __name__ == "__main__": unittest.main()
