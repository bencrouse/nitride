# Ripple filter

[eval-049 fixes the core gain staging](RIPPLE_GAIN.md) while retaining the existing
preset files/settings. The fixed-6-dB description below records the original
eval-047/048 implementation, not the current corrected output law.

The first auditions were too restrained for the user. The
[preset-only r2 revision](RIPPLE_R2.md) exposes the phase range and wet/dry balance
without changing the core or the original r1 sounds.

Independent Pigments-inspired all-pass family, not an Arturia clone or a claim of
sonic equivalence. No approved Classic/Rage voicing or factory selection changes.

## Public reference and independent choices

Reference: [Pigments 7 manual, §14.3.6, printed page 164](https://dl.arturia.net/products/pigments/manual/pigments_Manual_7_0_1_EN.pdf#page=170).
The documented model is a cascade of same-frequency all-pass filters. Subtle,
Medium and Hard add stages; Cutoff centers phase delay, Resonance controls its
transition bandwidth, Dry adds input, and Mode selects waveshaping or signed
feedback. Drive shapes the waveshaper branch. Exact stage counts, coefficient
mapping, distortion and feedback topology are not documented.

Nitride's independent choices:

- Voice-local normalized lossless scattering sections, processed at the existing
  full **4×** rate before each voice's amplitude envelope. Subtle/Medium/Hard use
  **2/4/8 second-order sections**. No voice cap, solver-precision reduction or
  quality switch; the existing twelve-voice engine is retained.
- `Q = .5 + 15.5 × (Resonance / .85)²`. Higher Resonance narrows the transition.
  Moving normalized states preserve energy in the linear cascade; this is not a
  swept direct-form all-pass with unnormalized delay states.
- Waveshaping follows the cascade. With `d = Drive`, `G = 1 + 15d²`,
  `shaped = wet + d × (tanh(G × wet) / sqrt(G) − wet)`. Drive zero is exactly
  linear and the positive-Drive limit is continuous at zero.
- Feedback mode uses `g = .94 × Feedback` and solves the affine same-sample loop
  `u = (1 − |g|) × input + g × cascade(u)` algebraically, without an added delay.
  The explicit input attenuation bounds the static wet gain; it is not AGC or an
  output limiter. Drive is dormant in Feedback mode and Feedback is dormant in
  Waveshaper mode.
- Output is `.5 × (wet + Dry × input)`: Dry is **addition**, not a dry/wet
  crossfade. The fixed 6 dB summing headroom is not automatic normalization.
  The instrument's existing downstream decimator/master path remains present.
- Type/mode banks crossfade over 5 ms. Family switching uses the voice-local
  Rage/Ripple fade and existing Classic/voice-route fade. Inactive bank work is
  skipped; no render-thread memory allocation or lazy table construction.

## Automation and persistence

The existing `filter_family` ID/range stays **0–1**. The UI offers Classic, Rage
and Ripple; Ripple enables an appended override while preserving the dormant
legacy family. Selecting Classic/Rage clears the override and writes the old
family in one edit transaction.

| Host index | Appended ID | Range/default |
| --- | --- | --- |
| 92 | `ripple_enabled` | 0–1 / 0 |
| 93 | `ripple_type` | 0–2 / 0 |
| 94 | `ripple_mode` | 0–1 / 0 |
| 95 | `ripple_dry` | 0–1 / .5 |
| 96 | `ripple_drive` | 0–1 / 0 |
| 97 | `ripple_feedback` | −1–1 / 0 |

The six controls use host version hint 8. Old IDs, ordering, ranges and hints stay
unchanged. Untouched Ripple defaults retain preset v6/v7 and session/XML v7/v8
wire formats. Active **or dormant nondefault** Ripple settings use preset v8 and
session/XML v9, including Compare, undo/redo and kept sounds. Legacy restores
initialize appended controls to their defaults, never stale prior Ripple values.
Only known native tables are trimmed to their historical 89/92 rows; v9 remains
complete at 98 rows. Unknown layouts/nondefault appended data are not trimmed.

## Original auditions

Programs **132–137** append after the frozen historical table. The original
recipes/MIDI are in `presets/review/batch-23/r1/`; `--verify` checks exact bytes,
and creation refuses existing revisions. None is factory-selected.

| Study | Type / mode | Native category |
| --- | --- | --- |
| Phase Silk | Subtle / Waveshaper | Pads |
| Negative Reed | Subtle / negative Feedback | Leads & Brass |
| Ripple Glass | Medium / Waveshaper | Keys & Organs |
| Positive Loom | Medium / positive Feedback | Sequences |
| Folded Prism | Hard / Waveshaper | Drums & Percussion |
| Phase Vault | Hard / negative Feedback | Textures |

Native publication uses additive `Study - …` filenames; existing factory/user
presets and ownership receipts are not overwritten. Factory promotion and
commit/push require separate authorization.

**Installed: 0.7.47 / eval-047, experimental audition only.** Restart Logic/create
a fresh instance and find the six `Study - …` names in the categories above, under
`~/Library/Audio/Presets/nitride/nitride/`. Actual registered-version AU validation
and 138-program / 98-control checks pass. Final registered native exports retain
77 current factory and 87 historical export bytes exactly; all 164 corresponding
audio/document recalls and six new Study recalls pass. Every pre-existing native
file/receipt remains unchanged. The eval-046 component and final source/binary
archives are preserved privately; no factory promotion, commit or push occurred.

## Reproducible checks and boundaries

- `nitride_ripple_dsp_tests`: unity magnitude/energy, analytical complex transfer,
  additive Dry, both feedback signs, moving-state energy, zero-Drive continuity,
  rate/control extremes, rapid switching, numerical recovery reporting and tails.
- Production integration tests: per-voice envelopes, overlap/release/stealing/
  panic, deterministic rendering/block partitioning, state/history/dormant
  settings, metadata, stale gestures and exact live UI/host switching audio.
- Allocation audit: cold rendering and rapid all-family/type/mode switching at
  48/96 kHz with twelve voices; concurrent host automation and state recall.
- `nitride_perf --ripple`: 108 Release cases, three types × waveshaping/negative/
  positive feedback × 1/4/12 voices × 48/96 kHz × 64/256 frames. Three two-second
  measured passes after .5-second warmup. Headroom requires both p99 ≤80% of the
  deadline **and zero overdue blocks**; failures are retained.
- `nitride_ripple_spectral_tests`: 54 coherent-tone/modulated 4× versus 32×
  reference-rate probes. Residual includes phase, discretization, decimation and
  trajectories, not just aliasing. It is not a cleanliness or audibility verdict.
- `nitride_perf --ripple --case ripple-hard-positive4 --soak --passes 1
  --seconds 1800 --rate 48000 --block 256`: continuous offline audio/control/
  state-reopen test, **not** a real-time Logic/device soak.
- `scripts/ripple_qualification.py` stages full-MIDI Classic/linear Ripple/voiced
  Ripple comparisons and assesses complete evidence without inferring external
  passes. Listening copies use a common active-RMS target, not LUFS. Raw audio,
  levels, deadline failures and metrics remain private under `evaluation/`.

Actual M2 execution, manual Logic project reopen and a 30–60-minute real-time
audio-device soak remain unverified. Existing Rage 96 kHz/twelve-voice overloads
and unexplained historical offline stalls are **not resolved by adding Ripple**.
No Logic projects/settings, telemetry or off-device transfer are needed here.

### eval-047 automated results

On **Apple M4 Max**, all 25 Release / 25 Debug suites, 180 Python tests and four
suites per sanitizer pass. The 212 Classic goldens, all 64 prior Rage benchmark
audio hashes, sixteen approved complete raw/preview performances, 77 current
factory native export bytes and 87 historical native export bytes remain exact.
The new family retains full 4× processing, double-precision states/feedback solve
and twelve voices. Original musical and controlled isolated three-state listening
comparisons remain private under `evaluation/filters/ripple-001/`.

**89/108 CPU cases pass** the strict headroom gate. Every 48 kHz case passes.
All eighteen 96 kHz/twelve-voice cases fail: median loads are 81.7–89.0%, with
116 overdue blocks across those cases. Hard/Waveshaper at four voices, 96 kHz /
256 frames has one overdue block despite a 30.5% median load. These failures are
not waived by the lower cost than Rage, and quality/polyphony are not reduced.

All 54 spectral probes pass their numerical guards (maximum feedback residual
below `5e-16`). None exceeds the diagnostic −20 dB 32× reference-residual marker;
the worst is −21.08 dB. This is **not** proof of alias-free audio, perceptual
quality or Pigments equivalence.

The continuous **1,800-audio-second** test completes with 61 exact in-memory state
round trips and zero numerical faults. It has **10 overdue blocks**, a maximum
block time of **15.092 ms**, and 397.066 seconds elapsed wall time. It passes
offline audio/state continuity, **not the deadline gate or a real-time device
soak**. Raw timings and the separate failing verdicts are retained.
