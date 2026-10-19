# Batch 05 / r1 — contrasting lead and sequence families

Priority: range and diversity, not near-variants of the kept sounds. These are
new families with explicit same-category `contrast_with` relationships. No DSP
architecture, host contract, quality/oversampling policy or global voice cap is
changed. The accepted pad is separately renamed Velvet Pulse without sound changes.

## Reed Snap / Leads

Compared with Glass Current's singing mono/gliding/vibrato voice, Reed Snap aims
for dry, resonant, reedy attacks and detached playing:

- Polyphonic, chord helper off; each note retriggers. No legato/glide, automatic
  pitch bend, vibrato or other LFO motion.
- 4 ms amplitude attack, 220 ms decay, .36 sustain and 180 ms release; a fast
  velocity-sensitive filter opening provides bite instead of a slow singing bloom.
- Nominal 1:1 soft-saturated FM, brief independent FM strike and amplitude-to-depth
  route; higher resonance focuses body. Space is only .02.
- A melodic voice for runs, stabs and actual dyads, not a sampled/modeled reed.

The 112 BPM demo uses detached runs, repeated velocity accents, longer notes and
dyads. Last note-off is about 13.39 s; the 20 s render includes the tail. Judge
responsive articulation and useful distinct identity, not just greater harshness
or a new name. Musical preference is pending.

## Undertow Grid / Sequence elements

Compared with Pulse Lattice's short metallic note-driven poly plucks, Undertow
aims for a lower, sustained mono rhythmic bed:

- Held-note envelope (.82 sustain), legato with a small 30 ms overlapping-note
  glide, no automatic pitch bend. MIDI supplies pitch changes, not every pulse.
- Sixteenth-note triangle amplitude movement (depth .85) and dotted-eighth
  sample-and-hold cutoff color, with slower two-bar FM-index/mix motion.
- Nominal 1:1 shaped FM, lower cutoff and gentler network stress; network activity
  influences FM shape. The character is not a 3:1 metallic pluck variant.
- **Play transport and hold notes** to hear the rhythm. It modulates audio/timbre;
  no internal MIDI sequencer/arpeggiator has been added.

The 116 BPM demo has held roots, deliberate overlap and a separated restart.
Last note-off is about 19.66 s; the 26 s render includes the tail. Listen for a
distinctive/useful flowing rhythmic role versus annoying tremolo or a muddy tone.
Do not infer success solely from a different pulse subdivision or parameter count.

## Frozen and shared-note performances

`make_midi.py --verify` reproduces the frozen SMF bytes, uses the existing
standard-library encoder, and refuses overwrites. Each file has one fixed tempo
and 4/4; later revisions should keep these performances for comparable voicing.

Separate shared-note comparisons render Glass Current/Reed Snap with identical
Reed MIDI, and Pulse Lattice/Undertow with identical held-note MIDI. Mono-versus-
poly response and short-versus-held behavior are intentional differences. Original
sound definitions/identity stay intact; these diagnostic performances do not
replace the older families' original MIDI or official revision evidence.

## Evidence / eval-009, 0.7.9

Stereo 48 kHz / 24-bit production-processor exports, repeated 64-frame passes and
257-frame comparison. Both repeat exactly at the same block size, export/reimport
with unchanged conditions/identity and finish releases. No numerical faults or
master-ceiling hits; final-second RMS is at the −100 dBFS reporting floor.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash | 64/257 max difference |
| --- | ---: | ---: | ---: | ---: | --- | ---: |
| Reed Snap r1 | −9.48 | −24.28 | +0.28 | 2 | `47fc4f775d95e6cb` | 0 |
| Undertow Grid r1 | −14.18 | −24.12 | +0.12 | 1 | `7ab5fb7b029b2301` | 1.49e−8 |

The sequence difference is below one 24-bit PCM step, not zero. Previews target
−24 dBFS gated active RMS, not LUFS; preview gain is excluded from preset values.
Before freezing r1, only output was calibrated (Reed −5 → −2.5 dB; Undertow
−4 → −.75 dB). Development capture is preserved separately, not a published or
user-tested revision, and level adjustment is not a musical-improvement verdict.

Apple M4 Max, two full demo passes at 48 kHz / 64: Reed mean occupancy **2.32%**,
p99 **107.83 µs**; Undertow **3.01%**, p99 **57.83 µs**. Zero measured callbacks
beyond the **1,333.33 µs** deadline. Demos include silence/releases and two/one
voices, not sustained max-polyphony or Logic/device-xrun qualification. Open F1
96 kHz limits remain unchanged.

Programs 19/20 append as `Review / Leads / Reed Snap r1` and
`Review / Sequence elements / Undertow Grid r1`. Read `evaluation/CURRENT.md`
for actual installed build/trial and next action. Ten distinct families now
exist; contrasting physical-material/experimental families remain toward roughly
twelve directions, not final approval. Their breadth should not be collapsed
into conventional good/bad polish or minor versions of Fault Bloom.
