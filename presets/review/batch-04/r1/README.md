# Batch 04 / r1 — contrasting pad and bass families

Collection priority: **showcase range and diversity early**, rather than making
near-variants or polishing all sounds toward the same character. These are new
families, not r3 replacements for Soft Alloy or Tension Core. Both kept reference
families and all other established directions remain unchanged.

## Intended contrast

| Dimension | Soft Alloy r2 | Prism Pulse r1 |
| --- | --- | --- |
| Temporal identity | Slow, continuous bloom | Quicker held-chord voice with quarter-note amplitude breathing |
| Timbre | Warm, low-cutoff shaped FM | Brighter 2.5:1 triangle-shaped FM and operator color |
| Movement | Independent slow free-running network motion | Quarter-note amplitude, quarter-triplet cutoff, two-bar FM color |
| Space | Larger/wetter bed | Smaller/drier rhythmic chord bed |
| Playing | Slow envelope and chord helper | 250 ms attack, actual played/MIDI chords; chord helper off |

Prism Pulse is still a pad: MIDI supplies held chords, while amplitude/Tone/FM
motion animates them. It does not generate an arpeggio or internal note sequence.
The amplitude lane is a smooth triangle pulse (depth .82), not a hard mute gate.
**Play transport** to hear the synced behavior in Logic; holding a chord while
the host reports a stopped, stationary PPQ does not demonstrate the loop.

| Dimension | Tension Core r2 | Felt Weight r1 |
| --- | --- | --- |
| Body | Dirty, stressed coupled voice | Rounded, lower-coupling, zero-stress voice |
| Articulation | Sustained, legato/gliding | Damped, low-sustain, retriggered pluck |
| FM | Shaped 2:1, stronger bite/operator mix | Low-index 1:1 sine FM, no operator mix/routes |
| Movement | Beat-synced FM and legato slides | No LFOs, glide or automatic bend |
| Space | Small amount of Space | Completely dry |

Felt Weight aims for a genuinely different bass function, not merely a quieter
or darker Tension Core. It uses the existing network/FM/Tone architecture—not a
new oscillator, sample or physical model. Mono is a patch articulation choice,
not an engine-wide polyphony reduction. These contrasts are **intent**, not an
automatic verdict that the musical range is sufficient.

## Performances and shared-note checks

- Prism's fixed 104 BPM demo holds four actual triads for six beats each, with
  space between chord changes and release overlap. Last note-off is about 17.31 s;
  the 26 s render includes the tail.
- Felt's 120 BPM demo has separated hits, velocity contrasts, low-octave notes,
  a higher fill and a final held note that exposes the low sustain. Last note-off
  is 9.75 s; the 14 s render includes the tail.
- Frozen SMF bytes are reproduced by `make_midi.py --verify`; the generator refuses
  overwrites and reuses the existing standard-library encoder.

Separate **shared-note comparisons** render Soft Alloy and Prism with the exact
same Prism chord MIDI, and Tension Core and Felt with the exact same Felt bass
MIDI. Original sound conditions/identity stay unchanged, and previews use the
same active-RMS target. These diagnostic performances are not substituted for the
old families' frozen reference demos or relabeled as their official revision
evidence. They help avoid mistaking a new MIDI arrangement for a new sound.

Listen for different musical roles on the same notes—not just louder/brighter
color. If either candidate still feels like a variation of an existing sound,
record that and revisit its behavior rather than counting a new name as diversity.

## Evidence / eval-007, 0.7.7

Stereo 48 kHz / 24-bit production-processor exports, two repeated 64-frame passes
and a 257-frame comparison. Both repeat exactly at the same block size; releases
finish, no numerical faults/master-ceiling hits and last-second RMS is at the
−100 dBFS reporting floor. Export/reimport preserves conditions/identity and MIDI.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash | 64/257 max difference |
| --- | ---: | ---: | ---: | ---: | --- | ---: |
| Prism Pulse r1 | −10.18 | −24.34 | +0.34 | 6 | `390c449462866513` | 2.98e−8 |
| Felt Weight r1 | −13.83 | −24.95 | +0.95 | 1 | `825529835147e105` | 0 |

The pad difference is below one 24-bit PCM step, not zero. Previews target −24
dBFS gated active RMS, not LUFS; preview gain is not stored in the presets.
Prism output was calibrated −6 → −4 dB before revision freezing; Felt remains
−2.5 dB. Development-level capture is separate diagnostic evidence, not a
published/user-tested revision or an improvement claim.

Apple M4 Max, two complete passes at 48 kHz / 64 frames: pad mean occupancy
**9.37%**, p99 **316.00 µs**; bass **1.99%**, p99 **48.21 µs**. Zero measured
callbacks exceeded the **1,333.33 µs** deadline. Demos include rests/releases and
use six/one voices; not max-polyphony or real Logic/device-xrun qualification.
The open F1 96 kHz limits remain unchanged. No DSP architecture, host-control
contract, oversampling/quality policy or global voice limit changed for this batch.

Programs 15/16 (zero-based) append:

- `Review / Pads / Prism Pulse r1`
- `Review / Basses / Felt Weight r1`

Read `evaluation/CURRENT.md` for actual installed build/trial and next action.
Eight distinct families now exist, not a completed/finally approved twelve-preset
collection. Contrasting lead/sequence and material/experimental families remain.
