# Batch 07 / r1 — original producer-lens experiment

The user selected **SOPHIE / Brian Eno / Flying Lotus** for one original patch
each. These are assistant-imagined creative briefs inspired by broad aesthetics,
not actual producer settings, quotations, private intent or endorsement. Artist
names stay in design-lens metadata, not a claim of authentic factory presets.

The twelve-family baseline stays intact. Skin is retained as useful rather than
particularly interesting; Orbit is explicitly praised. This adds three exploratory
families, not three automatically approved replacements or a new DSP architecture.

## Taffy Coil / Physical materials — SOPHIE lens

**Imagined brief:** make an impact behave like an impossible synthetic material,
stretching and snapping back, rather than put the same distortion on a drum.

- Poly/retriggered, 2 ms attack, .55 s decay, .05 sustain, .25 s release; modest
  Space .06. No cyclic motion or glide to distract from the hit's deformation.
- +12-semitone Autobend over .14 s with **opposed** routing: carrier and modulators
  move in opposite directions. Every note retriggers, unlike a phrase-only bend.
- Nominal 1.5:1 triangle FM, envelope-shaped spectrum/operator mix, short index
  strike, resonant Tone with filter-FM and modest network-to-index contribution.
- Contrast Skin Knock's 26 ms common-direction relaxation and shorter, dry damping.
  This is a synthetic material impression, not a physical simulation.

Play detached low/mid-register hits, accents, short rolls and actual dyads. Listen
for elastic attack-to-body behavior, not simply more brightness or recognizable
grit. The original 132 BPM demo ends its last note at about 11.36 s in a 20 s render.

## Stillwater / Pads — Brian Eno lens

**Imagined brief:** design an environment that rewards waiting; let harmonic
relationships change without making every change demand attention.

- 3.2 s amplitude attack, .85 sustain, 6.5 s release, slow filter/index envelopes
  and Space .48 / size .9. Open fifth/add-nine voicings are the demo, not automatic
  chord generation; chord helper is off.
- Low-stress, near-unity sine FM with direct operator blend. A .037 Hz source
  shifts index and nominal ratio; slow independent cutoff/coupling lanes and
  tiny pitch drift avoid a common beat-locked pulse.
- The lanes are shared and free-running, not a new per-voice generative system.
  No amplitude tremolo, synced rhythm, pitch attacks or glide.
- Contrast Soft Alloy's quicker and more active network bloom. Similar pad roles
  may still overlap; only listening establishes whether this lens adds useful range.

Hold quiet open chords for **at least 10–20 seconds**; short taps are not the
intended test. The original 76 BPM demo has two overlapping three-note sets,
last note-off at about 25.26 s and a 44 s render including long releases.

## Crooked Velvet / Basses — Flying Lotus lens

**Imagined brief:** give a bass line an elastic, slightly unstable personality that
responds to ghost accents, rather than rely on a static dirty sustained tone.

- Mono/retriggered envelopes, 7 ms attack, .34 s decay, .28 sustain and .22 s
  release. Short classic **carrier-only** legato glide applies to overlapping notes.
- A −5-semitone **modulator-only** bend relaxes over .16 s on each attack: harmonic
  structure moves without adding the same carrier pitch drop as the material patch.
- Triangle FM, velocity-sensitive Tone and envelope-to-shape/operator-mix routes
  change attack color. Free .28-depth amplitude motion at 1.9 Hz and modest
  sample-and-hold FM/ratio movement at 2.7 Hz make held body motion non-static.
- Contrast Felt Weight's dry, static, damped retriggered pluck. It is not sampling,
  time-stretching, a new groove engine or an internal sequencer.

Play short low-register phrases, ghost notes and a few overlaps/held notes.
The original 92 BPM demo's uneven offbeats/velocity accents are explicitly **MIDI
arrangement**, not swing built into the preset. Last note-off is about 16.63 s in
a 22 s render. Compare on identical MIDI so arrangement does not get credit for
a different synthesis identity. Transport need not play for the free motion.

## Fixed and common-note evidence

`make_midi.py --verify` reproduces all original SMF bytes and refuses overwrites.
The files use the existing standard-library encoder, one fixed tempo and 4/4 each.
Later revisions should retain these performances for comparable voicing changes.

Separate diagnostics compare Skin/Taffy on the same strike MIDI, Soft/Stillwater
on the same held chords and Felt/Crooked on the same irregular bass performance.
Original definitions/IDs and old frozen MIDI are unchanged. Normalized previews
help separate level and arrangement from patch identity; artist labels and changed
parameter counts do not demonstrate stylistic authenticity or meaningful diversity.

## Production renders / eval-012, 0.7.12

Stereo 48 kHz / 24-bit exports with identity/conditions preserved through preset
export/reimport. Same-block repeats and 64/257-frame comparisons are exact for all
three. No numerical faults/master-ceiling hits, completed amplitude releases and
final-second RMS at the −100 dBFS reporting floor. Soft coloration is still present.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Taffy Coil r1 | −9.89 | −24.09 | +.090 | 3 | `d0f3e69fa13d16ba` |
| Stillwater r1 | −11.84 | −24.01 | +.013 | 6 | `6034014af595ff7b` |
| Crooked Velvet r1 | −13.45 | −24.57 | +.567 | 1 | `1e0281f46f0326d2` |

Previews target −24 dBFS gated active RMS, not LUFS, and their gains are separate
from preset values. Only output was calibrated before r1 freezing: Taffy −4 →
−.75 dB, Stillwater −6 → −6.5 dB, Crooked −3 → 0 dB. Crooked stays about .57 dB
below preview target at the existing maximum preset output; no drive or global
gain-range change forces a match. Gate windows can shift with level. The eval-011
development capture is archived separately, not relabeled as final eval-012 audio.

Apple M4 Max, two full demos at 48 kHz / 64: mean occupancy **1.95 / 10.82 / 2.31%**
and p99 **109.46 / 330.63 / 58.29 µs** for Taffy / Stillwater / Crooked respectively.
Zero measured callbacks beyond the **1,333.33 µs** deadline. Demos include silence
and releases with only three/six/one voices; not sustained max-polyphony, real-device
xruns or Logic qualification. Open F1 96 kHz workload limits remain unchanged.

Programs 24/25/26 append under Physical materials / Pads / Basses. Host controls,
signal flow, progressive UI, network/Response/Space, global curves, quality policy
and global voice limits are unchanged. Registry/revision metadata preserves
`design_lens`; `evaluation/CURRENT.md` identifies the actual installed build/trial.
Musical success, the usefulness of each producer lens and approval remain pending.
