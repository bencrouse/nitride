# LFO and envelope presets — batch 18

All eight original Nitride studies are **finalized unchanged in 0.7.32 / eval-032**.
That explicit promotion brought the catalog to 61, preserving the prior 53 sounds;
the later [analog r2 promotion](ANALOG_STUDIES.md) brought it to 69, and the
[reverse r2 promotion](REVERSE_STUDIES.md) brings the current total to 77.
This does not infer per-sound commentary or extra host/release
qualification. These are parameter recipes using the existing synth, not new DSP,
per-voice LFOs, an arpeggiator or a
modulation-matrix expansion. Existing programs 0–65 remain unchanged; the studies
append at zero-based programs 66–73.

| Preset | Logic category | What to play and listen for |
| --- | --- | --- |
| Late Glass | Keys & Organs | Hold a key for two seconds: loudness arrives before the slow FM brightness. Compare a short tap with a sustained chord. |
| Rubber Relay | Bass | Short detached roots give the filter bite; hold one root for several beats to hear eighth-note FM chewing without a pitch LFO. |
| Gate Garden | Sequences | Hold a chord with transport playing: straight-eighth amplitude chops against dotted-eighth cutoff motion. The shared filter attack gradually reveals the pattern. |
| Paper Motor | Textures | Alternate short taps and held low/mid notes. Sample-and-hold Stress changes the material; slower Coupling and shallow tremolo keep the motor moving. |
| Closing Argument | Leads & Brass | Play detached lead notes, then sustain one: the negative filter envelope closes the initial bright vowel while amplitude stays up. |
| Undertint | Pads | Hold a chord for several seconds, then release. The FM brightness grows slowly but releases earlier than loudness, leaving a darker tail. |
| Loose Change | Drums & Percussion | Repeat the same note/velocity over several seconds. A slow free sample-and-hold FM lane varies the short metallic knock across the phrase. A held key dies away. |
| Carousel Wire | Plucks & Mallets | Repeat identical short notes with transport playing. The FM LFO's direct depth is zero, but its ratio/mix routes vary the pluck's partials around a consistent envelope. |

These descriptions explain the design intentions. The user explicitly promoted all
eight; that selection decision is separate from the technical checks below and does
not invent detailed listening observations. The r1 recipes and MIDI are untouched.

## Install in Logic

The normal source-install command now includes all 77 finalized sounds, including
these eight with clean names:

```sh
make reload-au CONFIG=Release
```

This works from a clean clone without a private evaluation journal or separate
preset download. Native export verifies the exact document, 89 host controls and
four-second stereo audio recall before installing missing clean-name files in the
categories above. Existing preset files, edits and user saves are not overwritten.
The source installer also leaves old Study files alone; the guarded local promotion
archives only receipt-owned, unchanged duplicates with exact clean replacements.
See [host installation and managed migration](../presets/factory/HOST_PRESETS.md).

In Logic, restart after updating and use the instrument's host **Settings** menu
above the interface. Standalone includes all 77 sounds in its normal category browser.
The original internal `Review / … r1` names and programs 66–73 remain available for
saved-session recall; clean display labels do not rewrite sound documents.

The original explicit-study export workflow remains supported for unselected future
studies, but it correctly refuses to republish this now-finalized batch as unapproved.

## MIDI, renders and checks

Frozen v6 recipes, original MIDI and the review manifest are in
`presets/review/batch-18/r1/`. Each performance includes repeated/detached notes and
held notes or chords, with time for amplitude release. Gate Garden and Carousel Wire
use 120 BPM; several free-motion/envelope studies use 100 BPM. Importing MIDI can
change host tempo, so explicitly check the actual project tempo when comparing.

```sh
python3 presets/review/batch-18/r1/make_motion_studies.py --verify
make preset-review REVIEW_BATCH=presets/review/batch-18/r1 \
  REVIEW_OUTPUT="$PWD/build/preset-review/batch-18/a-new-output"
```

The renderer refuses existing output, exports raw and level-matched 24-bit stereo
WAVs, and checks repeatability, 64/257-frame agreement at the unchanged 1e-5 guard,
finite audio, output headroom and completed amplitude release. Preview gain is
reported separately and does not alter the preset's Output setting. These are
offline serialization/timing checks, not a musical verdict or real-time host/xrun
qualification. Use moderate monitoring level; start practical host tests at
48 kHz/256 frames rather than inferring 96 kHz maximum-polyphony support.

### Recorded technical checks

The 0.7.32/eval-032 promotion passed 12 Debug and 12 Release suites, including seven
motion/promotion tests, plus the eight optional local FS1R tests. All eight new render
hashes exactly match their eval-031 audition hashes. Native AU validation and all
74-program/89-control state/MIDI checks passed; all 61 clean native files passed exact
document, parameter and audio recall. The prior 53 files stayed byte-identical.
Only the eight unchanged receipt-owned Study duplicates and their receipt were
archived, after checking exact clean replacements. User saves were preserved.

A staged source tree without `evaluation/` or git metadata also exported, verified
and installed all 61 real native presets, then reinstalled without changing an edited
factory file, user save or existing receipt. These are local technical checks, not
M2 host playback, project-reopen, soak or universal real-time qualification.

On the development M4 Max, the 0.7.31/eval-031 Release build passed all 12 core/tool
suites, including five new motion-study tests. All eight complete performances
rendered deterministically, finished their amplitude releases and reported zero
numerical faults. Maximum 64/257-frame sample error was 7.450580596923828e-9, below
the unchanged 1e-5 guard. The demos exercise one to four voices, not maximum polyphony.

Only the new studies' Output values were trimmed against these fixed performances:
raw active RMS is approximately −24.00 to −24.08 dBFS and the highest raw peak is
−9.96 dBFS. This is a simple −50 dBFS-gated RMS measurement, not LUFS, a musical
loudness verdict or a promise of headroom for arbitrary MIDI. Existing factory
outputs were not changed.

Separate uninstalled control batches retain the original MIDI, Output and amplitude
ADSR while disabling either all LFO lanes or the filter/FM envelope amounts. Both
change every study's audio. Sampled left-channel difference measurements show Late
Glass is predominantly envelope-led (about 0.99% relative RMS difference with its
LFO off, versus 76% with its contours off); the other LFO-off differences range
approximately 7–88%. These measurements establish modulation contributions, not
audibility thresholds or taste judgments. Controls, raw renders and full reports
remain local under `evaluation/motion-studies/pass-001/`.

## Timing and architecture boundaries

- All six LFO lanes are global and are **not restarted by a note**. Free-running
  movement is seconds-based; synced lanes follow host transport or fallback tempo.
  A stopped host with valid position freezes synced phase. Let transport run to
  hear Gate Garden and Carousel Wire as intended.
- Sample-and-hold is deterministic by cycle, not a new random draw for every note.
- The filter envelope is shared. New notes can reshape the whole chord; it is not
  independent per-note filter articulation.
- FM envelopes are per voice, but the amplitude envelope owns voice lifetime.
  Undertint deliberately gives amplitude a longer release than FM.
- Amplitude LFO attenuation occurs before Space. Small spatial tails can remain
  during a chop; neither preset promises a hard mute of the effect return.

The prior 53 factory recipes/native files and all old identities stay intact.
Audition feedback, technical measurements, MIDI and build identities are separate;
the explicit promotion does not extend the measured hardware/performance support envelope.
