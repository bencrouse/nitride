# Batch 02 / r1 — leads and sequence elements

Two new category sketches, not variations of the liked pad/bass pair. Existing
factory programs and review revisions are unchanged. All voicing uses the current
coupling/stress network, Response, FM, envelopes, Tone and Space; no DSP rewrite,
new controls, internal sequencer or automatic quality reduction is introduced.

## Glass Current / Leads

Intent: a singing, upper-register mono voice with a glassy edge and expressive
phrasing, rather than simply turning the bass cutoff up.

- 1.5:1 triangle-shaped FM with an independent attack/decay and network-driven
  depth; amplitude envelope also influences FM shape.
- Gentle 4.8 Hz / 0.08-semitone vibrato and slow coupling/stress/FM movement.
- 85 ms classic glide on overlapping notes only; legato envelopes enabled.
- No automatic attack pitch bend. Filter key tracking and velocity preserve
  contrast across the keyboard and between quieter/louder notes.
- Small Space setting and a 380 ms amplitude release keep it distinct from a pad.

The fixed 100 BPM MIDI demonstration contains separated attacks, overlapping
phrases, velocity contrasts, a held note with modest MIDI pitch-wheel motion,
upper-register notes, and a final lower held note. Last note-off is at 19.2 s;
the 24 s export includes the release/Space tail.

Listen for: an appealing sustained voice, comfortable legato transitions, and
whether the glassy edge or constant gentle vibrato is too conspicuous. Playing
different velocities and separated versus overlapping notes should be useful.
These are musical questions for the user, not conclusions from the renderer.

## Pulse Lattice / Sequence elements

Intent: short, articulate metallic plucks that stay interesting in a repeated
MIDI pattern without turning into a lead or a wash of effects.

- 3:1 triangle-shaped FM, short amplitude/FM envelopes, velocity-sensitive filter
  bite, and filter-envelope/network routes into FM shape/depth.
- Eighth-note cutoff movement, one-bar stress, two-bar coupling and four-bar FM
  cycles are tempo-synced. Pitch motion and glide are off.
- Polyphonic, chord helper off; modest Space leaves gaps between events.
- **MIDI supplies the notes. This patch is not an internal arpeggiator/sequencer.**

The 120 BPM demo is eight bars of eighth-note steps, intentional rests, velocity
accents, an upper transposition in the second half and occasional octave doubles.
The 20 s export leaves more than four seconds after the final note-off for the tail.
For Logic evaluation, start with repeated eighth/sixteenth notes or this MIDI
file while transport plays. Holding one note does not reproduce the demo pattern.

Listen for: useful articulation, accents that feel musical, non-fatiguing metallic
texture, and enough variation over a loop without overwhelming the arrangement.

## Frozen performances

```sh
python3 presets/review/batch-02/r1/make_midi.py --verify
```

The standard-library generator reproduces the frozen SMF bytes, uses one fixed
tempo and 4/4 per file, and refuses to overwrite existing MIDI. Future voicing
revisions should use these same performances for comparable renders. The MIDI
also exercises the production processor's actual note grouping/legato/transport.

## Evidence / eval-003, 0.7.3

48 kHz production-processor renders, stereo 24-bit WAVs, fixed MIDI/host transport.
Two 64-frame passes reproduce exact raw hashes. Export/reimport preserves all
preset conditions/identity and MIDI bytes. No DSP faults or master-ceiling hits;
amplitude releases finish and the last-second RMS is at the −100 dBFS reporting
floor. Preview gain is not stored in the preset; previews use −24 dBFS gated
active RMS, not LUFS matching.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash | 64/257 max sample difference |
| --- | ---: | ---: | ---: | ---: | --- | ---: |
| Glass Current r1 | −15.90 | −24.04 | +0.04 | 1 | `c551b737ebc33fbf` | 0 |
| Pulse Lattice r1 | −10.45 | −24.89 | +0.89 | 3 | `939b9642e11ab58a` | 1.49e−8 |

The sequence's block-size difference is less than one 24-bit PCM step, not zero.
Both repeat exactly at the same block size.

On the Apple M4 Max, two complete passes at 48 kHz / 64 frames measured lead
mean occupancy **3.39%**, p99 **69.33 µs**; sequence **3.04%**, p99 **111.46 µs**.
There were zero measured overdue callbacks against the **1,333.33 µs** deadline.
These exact demos include rests/tails; they are not worst-case sustained-polyphony
qualification or Logic/device-xrun measurements. The open F1 96 kHz limits remain.

Initial raw levels were adjusted before revision registration (lead output
−6 → −4.5 dB; sequence −6 → −2 dB) to bring the live presets closer to the
existing review pair's active levels. The pre-registration development capture
is separate diagnostic evidence, not a published revision or listening result.

## Installed trial

Programs 8 and 9 (zero-based) append to the built-in review catalog:

- `Review / Leads / Glass Current r1`
- `Review / Sequence elements / Pulse Lattice r1`

Use `evaluation/CURRENT.md` for the actual installed build, active trial and next
action. Source/MIDI, raw/preview WAVs, reports, build snapshots and feedback remain
on disk outside disposable `build/`. Neither patch is musically approved yet.
