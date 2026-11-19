# Ripple gain staging / eval-049

**Installed and verified: 0.7.49 / eval-049.** Actual registered-version AU
validation and 144-program / 98-control checks pass. The twelve existing installed
Ripple r1/r2 files recall exact current-engine audio and documents; no preset or
native file/receipt was rewritten. All 77 factory selections, 176 historical
native export bytes, sixteen approved full render pairs, 212 Classic goldens and
64 prior Rage benchmark audio hashes remain preserved.

All 25 Release / 25 Debug suites, 188 Python tests and four suites per sanitizer
pass on the recorded retries. Earlier sanitizer timeout logs remain retained.
The final 1,800-audio-second offline continuity run completes with 61 exact state
round trips, zero numerical faults and zero overdue blocks (4.176 ms maximum
block; 388.558 seconds elapsed wall time). This is an offline pass, not manual
Logic/device-soak or M2 qualification; the failed CPU matrix described below is
not waived. Quit and reopen Logic, confirm the version and use the existing r2
names. Start quietly. No factory promotion, commit or push occurred.

This fixes Ripple's excessive level loss in the core, without rewriting presets
or changing Classic/Rage. Use the existing `Study - … r2` names. Frozen parameters,
MIDI, native files/receipts, program numbers, automation and state schemas stay
unchanged. Ripple r1/r2 audio is deliberately louder on this build; eval-048 and
its exact old renders remain archived privately for comparison/rollback.

## Control-only post-sum makeup

The all-pass sections, Q mapping, waveshaper, signed feedback loop and relative
wet/Dry mix are unchanged. The old fixed `.5` output multiplier is replaced with
one calibrated scalar multiplying the **complete** `wet + Dry × input` sum.

- Waveshaper calibration measures the existing transfer's RMS response to a
  unit-peak sine at 65 Drive positions using 8,192-phase offline quadrature. The
  baked double-precision table is interpolated only for the gain control; the
  audio transfer itself is not approximated or reduced in quality.
- For Feedback, `g = .94 × Feedback`, and the phase-averaged wet power of the
  unchanged loop is `(1 − |g|) / (1 + |g|)`.
- With calibrated wet power `P`, the common post-sum gain is
  `1 / sqrt(max(1/16, P + Dry²))`. The maximum is **4× post-sum gain**; this bounds
  the calibration, not the audio waveform. Wet-only linear all-pass gain is unity.
- Both branches receive the same scalar: no Dry increase, notch filling or wet
  rebalancing. Complete gain-staged banks crossfade over the existing 5 ms period.
  Coefficients are cached; there is no allocation or live RMS analysis.

There is **no envelope follower, AGC, automatic loudness locking, new limiter,
drive reduction or voice cap**. The calibration does not depend on the signal,
pitch, Cutoff, Resonance, stage count or note envelope. Notches and resonant peaks
still change level naturally. The existing shared tanh bus ceiling, JUCE Space
path and final output clamp remain unchanged; dense chords/extreme settings may
still need ordinary master-output trim. Measured audition headroom is not a
guarantee for every note, gain setting or twelve-voice chord.

## Checks

The archived eval-048 core comparison covers **3,538,944 samples** across all six
type/mode combinations, four input levels, modulation and tails. The corrected
output matches the old output times its predicted fixed gain to within `2.3e-16`;
feedback trajectories and diagnostics are identical. This establishes a gain
change, not a revised waveshaper, feedback topology or phase response.

Core tests cover unity all-pass energy, the revised complex transfer, bounded and
sign-symmetric makeup, actual RMS calibration between table knots, no-AGC level
scaling, control/rate extremes, switching, invalid-input reporting and tails.

The full-performance audit retains exact source conditions and MIDI. It reverses
the **existing** master tanh ceiling and JUCE's existing 2× dry gain at Space=0 to
compare signals before that ceiling. Contact with the final hard clamp or a
non-invertible ceiling invalidates this audit rather than being silently omitted.
Downstream spectral/crest changes from driving the unchanged ceiling harder are
reported separately; raw levels and matched-level listening copies are private.

### Frozen r2 performance results

| Preset | Active-RMS increase | Relative to same-output unity reference | Peak headroom |
| --- | ---: | ---: | ---: |
| Phase Silk r2 | +9.90 dB | +0.59 dB | 9.45 dB |
| Negative Reed r2 | +16.34 dB | −4.32 dB | 5.13 dB |
| Ripple Glass r2 | +12.45 dB | −0.24 dB | 10.52 dB |
| Positive Loom r2 | +14.88 dB | +1.05 dB | 4.00 dB |
| Folded Prism r2 | +12.33 dB | +0.84 dB | 16.05 dB |
| Phase Vault r2 | +16.32 dB | +0.37 dB | 5.05 dB |

All six pass the explicit ±6 dB reference-level and ≥3 dB peak-headroom goals for
these performances. The pre-ceiling fixed-gain residual is below −103 dB for r2;
all twelve r1/r2 audits pass. Active RMS is not LUFS, and these are engineering
checks, not listening approval. Start playback quietly: old saved Ripple settings
now receive the corrected gain without a preset update.

Actual M2 execution and manual Logic/project/audio-device-soak checks remain
unverified. CPU/headroom failures and historical unexplained timing stalls are
not resolved by this gain correction. Private raw evidence is in
`evaluation/filters/ripple-003/`. No factory promotion or commit/push is implied.

The first complete eval-049 timing matrix passes **75/108** strict headroom gates.
It includes very large wall-clock stalls (including low-voice-count cases), not
just the existing 96 kHz/twelve-voice headroom failures. All raw observations are
retained; interrupted ASan/TSan runs also have retained timeout logs, with later
successful retries. The cause of the stalls is unproven. Neither corrected gain,
successful numerical tests nor a later retry makes those timing failures a pass.
No existing final-output clamp contact occurs in this matrix (maximum peak .871),
but that does not establish universal real-time or peak-headroom qualification.
