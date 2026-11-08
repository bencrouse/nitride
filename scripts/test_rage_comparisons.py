import math
from array import array
from pathlib import Path
import tempfile
import unittest

import rage_comparisons as rc


class RageComparisons(unittest.TestCase):
    def test_constant_gain_is_not_timbre(self):
        source = [math.sin(i*.1) for i in range(500)]
        self.assertLess(rc.residual(source, [x*4 for x in source]), -250)
        self.assertGreater(rc.residual(source, [x+.2*math.sin(i*.3) for i, x in enumerate(source)]), -20)

    def test_float_wav_roundtrip_and_non_overwrite(self):
        with tempfile.TemporaryDirectory(prefix="nitride-rage-comparison-") as folder:
            path = Path(folder)/"probe.wav"; samples = array("f", [.25]) * (192000*2)
            rc.write_probe(path, samples, .5); self.assertEqual(rc.read_probe(path), array("f", [.125])*(192000*2))
            with self.assertRaises(FileExistsError): rc.write_probe(path, samples, 1)
            invalid = Path(folder)/"bad.wav"; invalid.write_bytes(b"not a WAV")
            with self.assertRaises(ValueError): rc.read_probe(invalid)

    def test_range_gate_rejects_gain_only_and_collapsed_characters(self):
        good = {"cleanMaximumError": 0, "amountComparisons": [{"build": "candidate", "halfResidualDb": -10, "fullResidualDb": -5}],
                "fullCharacterPairs": [{"residualDb": -20}]}
        rc.require_strong_range(good)
        good["fullCharacterPairs"][0]["residualDb"] = -100
        with self.assertRaisesRegex(ValueError, "collapsed"): rc.require_strong_range(good)
        good["fullCharacterPairs"][0]["residualDb"] = -20; good["amountComparisons"][0]["fullResidualDb"] = -100
        with self.assertRaisesRegex(ValueError, "range"): rc.require_strong_range(good)

    def test_build_labels_reject_paths_and_collisions(self):
        for baseline, candidate in (("../escape", "new"), ("old", "new/name"), ("candidate", "new"), ("same", "same")):
            with self.assertRaises(ValueError): rc.compare("missing", "missing", "missing", baseline, candidate)

    def test_study_gate_preserves_diode_and_requires_live_and_quiet_character_range(self):
        report = {"diodeMaximumError": 0, "zeroRageMaximumError": 0, "preSwitchMaximumError": 0,
                  "settledSwitchMaximumError": 1e-7, "characters": [
                      {"character": 0, "oldFullResidualDb": -22, "fullResidualDb": -22, "halfResidualDb": -30, "fullLevelDeltaDb": -5},
                      {"character": 1, "oldFullResidualDb": -23, "fullResidualDb": -15, "halfResidualDb": -24, "fullLevelDeltaDb": 1}]}
        rc.require_study_range(report)
        report["diodeMaximumError"] = 1e-9
        with self.assertRaisesRegex(ValueError, "Diode"): rc.require_study_range(report)
        report["diodeMaximumError"] = 0; report["settledSwitchMaximumError"] = 1e-3
        with self.assertRaisesRegex(ValueError, "switching"): rc.require_study_range(report)
        report["settledSwitchMaximumError"] = 0; report["characters"][1]["fullResidualDb"] = -22
        with self.assertRaisesRegex(ValueError, "weak"): rc.require_study_range(report)
        report["characters"][1]["fullResidualDb"] = -15; report["characters"][1]["fullLevelDeltaDb"] = -10
        with self.assertRaisesRegex(ValueError, "energy"): rc.require_study_range(report)

    def test_retention_mode_does_not_demand_continually_increasing_drive(self):
        report = {"diodeMaximumError": 0, "zeroRageMaximumError": 0, "preSwitchMaximumError": 0,
                  "settledSwitchMaximumError": 0, "characters": [
                      {"character": 1, "oldFullResidualDb": -16, "fullResidualDb": -17, "halfResidualDb": -24, "fullLevelDeltaDb": 0}]}
        rc.require_study_range(report, require_improvement=False)
        with self.assertRaisesRegex(ValueError, "weak"): rc.require_study_range(report)
        report["characters"][0]["fullResidualDb"] = -30
        with self.assertRaisesRegex(ValueError, "weak"): rc.require_study_range(report, require_improvement=False)

    def test_fft_matches_known_impulse_and_constant(self):
        self.assertEqual(rc._fft([1, 0, 0, 0]), [1+0j]*4)
        self.assertEqual(rc._fft([1]*4), [4+0j, 0j, 0j, 0j])
        with self.assertRaises(ValueError): rc._fft([1]*3)

    def test_magnitude_separation_ignores_constant_gain_and_phase(self):
        a = [math.sin(2*math.pi*n/64) for n in range(64)]
        b = [3*math.sin(2*math.pi*n/64+.7) for n in range(64)]
        c = [x+.2*math.sin(6*math.pi*n/64) for n, x in enumerate(a)]
        magnitudes = lambda x: [abs(y) for y in rc._fft(x)]
        self.assertLess(rc.residual(magnitudes(a), magnitudes(b)), -250)
        self.assertGreater(rc.residual(magnitudes(a), magnitudes(c)), -20)

    def test_contrast_gate_rejects_unchanged_or_collapsed_spectra(self):
        report = {"revisedMeanSeparationIncreaseDb": 3, "revisedMinimumSpectralResidualDb": -20}
        rc.require_character_contrast(report)
        report["revisedMeanSeparationIncreaseDb"] = 0
        with self.assertRaises(ValueError): rc.require_character_contrast(report)
        report["revisedMeanSeparationIncreaseDb"] = 3; report["revisedMinimumSpectralResidualDb"] = -40
        with self.assertRaises(ValueError): rc.require_character_contrast(report)


if __name__ == "__main__": unittest.main()
