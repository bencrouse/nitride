# Detuned analog-style presets — batch 19

## Finalized r2 / 0.7.35 · eval-035

The user explicitly approved all eight r2 refinements and requested finalization,
commit and push. They now join normal factory browsing unchanged, bringing the
catalog to **69 sounds**. All 90 programs and both r1/r2 source definitions/MIDI are
preserved. R1 remains unselected and recallable; only r2 is factory-selected.

```sh
make reload-au CONFIG=Release
```

This installs missing clean-name native presets from a fresh clone without a private
journal. Restart Logic/create a fresh instance, then browse the host Settings menu.
Standalone includes all 69 in its normal browser. Existing user files/edits are never
overwritten. The guarded local promotion archives only unchanged, receipt-owned r2
duplicates with exact clean replacements; original r1 comparison files are untouched.
The ordinary source installer does not automatically remove old Study files on other
machines. Detailed [managed migration rules](../presets/factory/HOST_PRESETS.md) apply.

The approval does not infer hardware/artist fidelity, per-sound comments, actual
Logic project-reopen, M2/soak or wider real-time/release qualification.

Promotion checks on the development Mac passed all 13 Release and 13 Debug suites,
plus all 124 Python tests. All eight eval-035 render hashes exactly match the
auditioned eval-034 r2 hashes. The actual registered 0.7.35 AU passed `auval` and
90-program/89-control state/MIDI checks. All 69 native presets passed exact full-
document, parameter and audio recall. The prior 61 native files, original r1
comparisons/receipt and all unrelated before-files stayed byte-identical; only the
eight owned r2 Study duplicates and their receipt were archived outside the host tree.
Frozen local evidence is in `evaluation/preset-finalization/pass-007/`.
Actual native deployment from a staged source tree without Git metadata or a private
journal installed all 69; reinstall preserved every path, including a deliberately
edited factory file, user save and existing receipt.

## Refinement history: r2 / 0.7.34 · eval-034

The user liked all eight directions and asked for slightly more detunedness. Each
r2 changes **only the static FM ratio**, moving it about 1.5–2× farther from its
intended harmonic. Tap mix, amplitude/FM/filter envelopes, tone, Output, all drift
and vibrato settings, mono/legato behavior and original MIDI remain unchanged.
More separation means faster beating/stronger partial displacement, not extra vibrato.
Perceived loudness can still change with phase relationships even though Output is
unchanged; level-matched previews and actual-gain raw files remain separate.

| Preset | r1 offset | r2 offset |
| --- | ---: | ---: |
| Low Battery Piano | +5.19 cents | +10.36 cents |
| Slack Drum | +18.94 cents | +29.18 cents |
| Brown Console | −5.20 cents | −10.42 cents |
| Silt Bass | −3.47 cents | −6.94 cents |
| Bent Aerial | +11.22 cents | +17.23 cents |
| Melted Ensemble | +6.05 cents | +12.08 cents |
| Oxide Rim | +11.22 cents | +18.94 cents |
| Reel Clav | +2.59 cents | +5.19 cents |

The last two offsets are relative to 2:1; the others are relative to 1:1. Frozen r1
files, programs 0–81, the finalized 61 and all old native/user files are preserved.
The r2 programs appended at 82–89 (90 total). At this stage both were auditions;
the later explicit finalization above selects r2 only.

During audition, **Study - Low Battery Piano r2** lived beside the original
**Study - Low Battery Piano**, and likewise for the other seven. The revision suffix
prevented replacement or ownership claims on the originals. After promotion, the
clean **Low Battery Piano** is r2. The original r1 export command below remains valid;
the study exporter correctly refuses to republish finalized r2 as unapproved studies.

R2 passed all 13 Release suites, including seven analog/side-by-side publication
tests. All eight r2 performances passed deterministic rendering, complete release,
headroom and 64/257-frame agreement (observed maximum error zero). Re-rendered r1
audio hashes match the original eval-033 batch exactly. Tests verify byte-identical
MIDI and precisely one changed sound parameter per revision. Local A/B previews
and actual-gain WAVs are linked in `evaluation/analog-studies/pass-002/LISTEN.md`.

## Original r1 brief and architecture

Eight original studies were prepared for 0.7.33/eval-033, using **Beak> and Portishead as imagined
mood briefs**, not artist settings, sampled instruments, song recreations or
endorsements. R1 remains unselected; r2 is now finalized alongside the prior 61.
Programs 0–73 and their sounds were unchanged; r1 appended at zero-based 74–81.
There are no DSP, schema, parameter-ID, oversampling or voice-policy changes.

The target is small, exposed, slightly unsteady instruments: a worn key, loose drum,
dark console organ, grounded bass and narrow lead—not eight big chorused pads.

| Study | Logic category | Role and gesture |
| --- | --- | --- |
| **Low Battery Piano** | Keys & Organs | Requested detuned piano-like key: short hammer-like FM edge, decaying body and near-unison ring. Compare soft/hard isolated strikes and small chords. An FM approximation, not an acoustic-piano model. |
| **Slack Drum** | Drums & Percussion | Requested detuned drum: a loose pitched tom-like body with a short attack drop. Hold MIDI 48 or 55 to hear its ring; lower short notes work as dull groove punctuation. |
| **Brown Console** | Keys & Organs | Dark, immediate combo-organ surrogate. Hold a chord for beating, then play repeated motorik figures. Tiny ratio wander and tremolo are not a Leslie model. |
| **Silt Bass** | Bass | Dry, rounded near-unison bass with slow shallow common drift and modest legato glide. Leave space between phrases rather than treating it as a wobble-bass effect. |
| **Bent Aerial** | Leads & Brass | Narrow, slightly fuzzy lead; small relative drift and slower common wow supply tension. Try overlapping phrases and a long high note. |
| **Melted Ensemble** | Pads | Dark, sagging ensemble surrogate with slow entrance, moving beat rates and restrained room. Less bright and direct than Budget Heaven; no new chorus engine. |
| **Oxide Rim** | Drums & Percussion | Dry, papery rim-like edge, contrasting Slack Drum's low body. Slightly inharmonic FM attack, not a snare-wire/noise model or a claim of slow beats in every short hit. |
| **Reel Clav** | Plucks & Mallets | Damped woody electric-key/clav surrogate. Repeat identical short notes to hear a shallow held-cycle pitch offset change across the phrase, not randomly per key. |

These descriptions explain the design intentions. The user approved r2 after the
refinement; that does not invent detailed listening observations about every role.
The MIDI remains original and unchanged between revisions.

## How the detuning works

Nitride has no new dual-VCO/unison model here. Its existing audible primary-modulator
tap can be blended with the carrier. Putting that primary slightly above or below
1:1 gives **real relative frequency separation**, even with the common pitch LFO off.
FM sidebands and network interaction also color the result; it is not simply a pair
of ideal analog oscillators.

Original r1 ratios are retained here for lineage; the finalized values are the r2
offsets in the comparison table above (ratios 1.006, 1.017, 0.994, 0.996, 1.010,
1.007, 2.022 and 2.006 respectively).

| Original r1 study | Base primary ratio | Offset from intended harmonic |
| --- | ---: | ---: |
| Low Battery Piano | 1.003 | +5.19 cents from 1:1 |
| Slack Drum | 1.011 | +18.94 cents from 1:1 |
| Brown Console | 0.997 | −5.20 cents from 1:1 |
| Silt Bass | 0.998 | −3.47 cents from 1:1 |
| Bent Aerial | 1.0065 | +11.22 cents from 1:1 |
| Melted Ensemble | 1.0035 | +6.05 cents from 1:1 |
| Oxide Rim | 2.013 | +11.22 cents from 2:1 |
| Reel Clav | 2.003 | +2.59 cents from 2:1 |

For example, original r1 Low Battery Piano's nominal carrier/primary separation at MIDI 60
(~261.63 Hz) is ~0.785 Hz; Slack Drum's at MIDI 48 (~130.81 Hz) is ~1.439 Hz.
These are derived frequency differences, **not measured acoustic beat-rate claims**.
The two 2:1 studies emphasize detuned partial/strike color rather than primary
unison beating. The drum's five-semitone attack drop is a separate transient
gesture; it is not the persistent detune and it does not need a pitch LFO.

Common pitch motion moves carrier and modulators together; by itself that is wow
or vibrato, **not unison detune**. Brown Console, Bent Aerial and Melted Ensemble
add an independent, tiny FM-LFO-to-ratio route while keeping direct FM-LFO depth
at zero. All movement remains global and free-running, not independent drift per
voice or note-reset random tuning. Reel Clav's sample-and-hold is deterministic by
cycle and changes over a phrase, not on each strike.

Other limits remain deliberate: one shared Tone filter/envelope, no tape process,
noise oscillator, independently tuned piano strings, dedicated snare model, chorus
or stereo VCO spread. These are creative parameter patches, not new emulations.

## Original r1 installation

On the development Mac the batch is installed with explicit **Study - …** names.
Restart Logic after the AU update and create a fresh instance. Use the host's
**Settings** menu above the interface, in the categories listed above. Normal
factory browsing now contains 69 sounds, including the clean finalized r2 versions.

For another source checkout:

```sh
make reload-au CONFIG=Release
python3 scripts/study_presets.py build/analog-mood-001 \
  --batch presets/review/batch-19/r1 --study analog-mood-001 \
  --component "$HOME/Library/Audio/Plug-Ins/Components/nitride.component" \
  --tool build/nitride_au_presets --install
```

The explicit-component path needs no private journal. It verifies exact full state,
89 parameters and four-second stereo audio recall before safe additive installation.
Use a new output directory; existing user files, factory files, study receipts and
edited studies are never overwritten. Standalone exposes these new programs in
**… → Evaluation archive**, or imports their portable `.nitridepreset` files.

## Demos and technical checks

Frozen recipes, original MIDI and the manifest are in `presets/review/batch-19/r1/`.
The performances cover held notes/chords, repeated notes, articulation and release.
Free LFOs are seconds-based; MIDI tempos range from 80 to 108 BPM and do not lock
the drift. Importing MIDI may affect project tempo: check the actual host setting.

```sh
python3 presets/review/batch-19/r1/make_analog_studies.py --verify
make preset-review REVIEW_BATCH=presets/review/batch-19/r1 \
  REVIEW_OUTPUT="$PWD/build/preset-review/batch-19/a-new-output"
```

The renderer checks repeatability, finite audio, completed amplitude release,
headroom and 64/257-frame agreement at the existing 1e-5 guard. It writes raw and
level-matched preview WAVs; preview gain is separate from the saved Output value.
The piano retains extra transient headroom instead of forcing every raw patch to
the same RMS. Slack Drum's Output stops at the existing 0 dB limit—no hidden boost.

Separate local controls disable common pitch/relative-ratio LFO motion while
retaining static detune, then additionally move the ratio to the intended harmonic.
Comparing those controls isolates static detune from common wow without replacing
the original MIDI, amplitude contour, output, transient pitch drop or glide. Real
renderer assertions also check that the first six patches retain different carrier
and primary frequencies with their motion disabled. These are mechanism checks,
not proof of musical success or analog hardware fidelity.

### Recorded local checks

The development M4 Max Release build passed all 13 suites, including five new analog
tests and the real-renderer frequency checks. All 24 original/no-wobble/tuned
performances passed repeatability, release, finite-audio, headroom and block-size
checks. The two drum originals exactly match their no-wobble controls because they
already have no pitch/ratio LFO; their static-ratio controls still differ.
Waveform-difference RMS is phase-sensitive and is not a perceptual detune score.

The original demos use one to four voices. Raw active RMS ranges approximately
−24.0 to −25.2 dBFS, with a highest raw peak of −6.04 dBFS. These are simple gated
RMS measurements, not LUFS or a guarantee for arbitrary MIDI. All 61 old native
presets passed exact document/parameter/audio recall on the new build, all eight
Study exports passed the same native checks, and the actual registered 0.7.33 AU
passed validation plus 82-program/89-control state/MIDI checks. The old 61 files,
user saves, receipts and factory selection remained unchanged. Recipe/native-export
unit tests also passed in a staged source tree without the private journal or git.

Private r1 renders, controls, native packages, build evidence and the listening guide
remain local under `evaluation/analog-studies/pass-001/`; r2 comparisons live under
`evaluation/analog-studies/pass-002/`. They are not uploaded.
Use moderate monitoring level and start practical host tests at 48 kHz/256 frames.
None of this closes the existing maximum-polyphony/96 kHz, M2, host-soak or actual
Logic project-close/reopen qualifications. Factory promotion required the separate
explicit user decision recorded above.
