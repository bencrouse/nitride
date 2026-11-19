# Ripple r2: expose the phase response

The later [eval-049 gain-staging correction](RIPPLE_GAIN.md) applies to these same
r2 files without another preset revision. Statements below about unchanged DSP
describe the original eval-048 preset-only checkpoint; exact old audio remains in
its archive. The r2 source parameters and MIDI themselves remain unchanged.

The user's eval-047 feedback was that the filter hardly seemed to do anything.
That identified a voicing failure which numerical stability and waveform-difference
tests alone did not address. Several r1 presets placed a narrow phase transition
above most of their oscillator's harmonic energy. Also, the shaped wet branch can
be much quieter than the original input: a high Dry setting then masks that branch.

This is a **preset-only revision**, not a new filter algorithm. All DSP sources,
r1 presets/MIDI, approved Classic/Rage sounds and the 77 factory selections stay
unchanged. Programs 138–143 append after 0–137. No automation, schema, precision,
oversampling or voice-count change is involved.

## Changes

| Study r2 | Cutoff | Resonance | Dry | Drive / Feedback |
| --- | ---: | ---: | ---: | --- |
| Phase Silk | 180 Hz | 0 | .45 | Drive .9 |
| Negative Reed | 260 Hz | .15 | .05 | Feedback −.9 |
| Ripple Glass | 350 Hz | 0 | .3 | Drive 1 |
| Positive Loom | 180 Hz | .1 | .1 | Feedback +.9 |
| Folded Prism | 600 Hz | 0 | .25 | Drive 1 |
| Phase Vault | 260 Hz | .18 | .05 | Feedback −.9 |

Lower centers and broader transitions put movement through the actual harmonics.
The cutoff envelopes/LFO ranges are retuned where useful. Dry is balanced against
the **actual wet level**, not assumed to be a conventional dry/wet percentage.
Fixed output trims recover some wet-path attenuation; they are not AGC. The exact
recipes are in `presets/review/batch-23/r2/make_ripple_r2.py`.

Oscillator/FM settings, amplitude envelopes, notes, velocities, timing, type/mode
and mono/chord settings remain identical to r1. Each MIDI file is byte-identical.
The generator verifies frozen bytes and refuses to overwrite an existing revision.

## Contrast evidence, not listening approval

Private comparisons play a near-flat same-route reference, r1, then r2 on the same
MIDI/source. One common active-RMS target retains at least 3 dB peak headroom. This
is not LUFS. Raw levels, timing and files remain available privately.

The new check compares Welch **magnitude** profiles after removing constant gain,
so a pure phase shift or volume change does not qualify as stronger tonal color.
Unit tests explicitly reject those false positives. On the prototype render:

| Study | r1 magnitude residual | r2 magnitude residual |
| --- | ---: | ---: |
| Phase Silk | −36.32 dB | −13.67 dB |
| Negative Reed | −46.40 dB | −5.60 dB |
| Ripple Glass | −26.91 dB | −13.64 dB |
| Positive Loom | −23.92 dB | −7.90 dB |
| Folded Prism | −19.37 dB | −8.00 dB |
| Phase Vault | −28.94 dB | −10.92 dB |

Less-negative residual means more separation from the reference, not better sound.
Every r2 must exceed −16 dB residual and improve separation over r1 by at least
10 dB. These are numerical contrast goals, **not proof of audibility or approval**.
`scripts/ripple_audition_comparisons.py` stages and checks these comparisons.

## Audition and limits

**Installed experimental audition: 0.7.48 / eval-048.** Actual registered-version
AU validation and all 144-program / 98-control native checks pass. All 25 Release,
25 Debug and 184 Python tests pass. Every DSP source, 22 existing full raw/preview
render pairs, 170 historical native exports, 108 prior Ripple hashes and 64 Rage
hashes remain exact. All pre-existing native files/receipts and 77 factory
selections are unchanged; six r2 native recalls pass and their files are additive.
Source/binary evidence and the previous component are archived privately under
`evaluation/filters/ripple-002/` and the evaluation journal.

The repeated CPU matrix passes 90/108 headroom gates; all eighteen 96 kHz /
twelve-voice cases still fail. Timing completion is not broader qualification.

The new native files use **`Study - … r2`** names; the original r1 names/files are
retained rather than overwritten. Start with **Study - Positive Loom r2** and
**Study - Negative Reed r2**. Quit and reopen Logic and confirm **0.7.48 / eval-048**
before loading them: an already-loaded older AU may not know the appended programs.

Ripple Cutoff is a phase-transition center, **not a low-pass edge**. In Waveshaper
mode with Dry=0 and Drive=0, a static linear all-pass deliberately retains unity
magnitude. For phaser-like color, mix input back in and sweep through the source's
harmonics. Higher Resonance narrows the transition; it need not sound more dramatic.

The existing CPU/headroom/deadline failures, actual-M2 qualification and manual
Logic/audio-device-soak checks remain open. No new long-duration soak or sanitizer
run is inferred from this preset-only iteration. It is an experimental audition,
not factory promotion, Arturia equivalence or authorization to commit/push.
