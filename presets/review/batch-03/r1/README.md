# Batch 03 / r1 — physical materials and experimental

The last breadth-first category pair. Four liked r2 directions remain untouched:
Soft Alloy, Tension Core, Glass Current and Pulse Lattice. These two new candidates
bring coverage to six categories, **not final approval or a finished 12-preset
collection**. Existing factory/review program indices and saved conditions remain.

No DSP architecture changes are made for this batch. Both sounds use Nitride's
coupling/stress network, Response, FM, envelopes, Tone, pitch motion and Space.

## Hollow Bronze / Physical materials

Intent: a struck, hollow metallic body with a brief attack and damping ring,
rather than a sustained bell-pad or a rhythmically modulated sequence patch.

- 2.73:1 sine FM and a 55 ms FM-envelope decay provide inharmonic strike color.
- A small amplitude-envelope-to-FM-ratio route changes partial relationships as
  the strike decays; network activity also influences FM depth.
- 1.6 ms amplitude attack, 1.4 s decay, **zero sustain**, 1.5 s release; holding a
  key should still die away, and short gates leave a ring.
- Velocity-sensitive filter bite, key tracking and modest Space. All LFO lanes,
  glide, automatic pitch bend and the chord helper are off; polyphonic playing
  permits simultaneous strikes.

This is **material-like network/FM voicing**, not a claim that Nitride now contains
a new modal/waveguide physical model or sampled bronze instrument.

The fixed 80 BPM demonstration plays A3 at three velocities, lower/upper strikes,
a held C4 to expose the zero-sustain decay, and two dyads. Last note-off is 16.95 s;
the 26 s export includes release/Space tails. Compare light versus firm strikes,
short gates versus a held key, and how convincing/useful the body feels. Musical
believability is pending user listening, not established by numeric validation.

## Fault Bloom / Experimental

Intent: an elastic, playable mono network voice that changes character while held
and pulls carrier/modulator motion apart during overlapping note transitions.

- Higher coupling/stress with a slower Response and independent free-running
  coupling/stress/FM/cutoff motion, rather than a beat-clocked random sequence.
- Sub-unity 0.75:1 shaped FM with a 1.4 s bloom; network activity influences FM
  ratio, the FM LFO influences shape, and the filter envelope influences operator
  mix. Mild audio-rate filter FM contributes texture through the existing path.
- **550 ms opposed glide on overlapping notes**, legato envelopes, no automatic
  attack bend. Separated notes restart the bloom; overlapping intervals expose
  the deliberately unusual carrier/modulator pitch relationship.
- No internal sequencer and no separate pitch LFO. Mono articulation is a patch
  choice, not a global engine polyphony cap or quality reduction.

The fixed 90 BPM performance contains held lower/upper notes, overlapping fifth,
tritone and wider transitions, a separated quieter restart and a final held note.
Last note-off is about 23.33 s; the 30 s export includes release/Space tails.
Hold a note for several seconds, then overlap another note before releasing the
first. Listen for a distinctive, usable instability versus an unpleasant or
unfocused tone. Neither excitement nor playability is inferred from the renderer.

## Frozen performances

```sh
python3 presets/review/batch-03/r1/make_midi.py --verify
```

The generator reuses batch-02's frozen standard-library SMF encoder, supplies one
fixed tempo/4/4 per file, reproduces bytes exactly and refuses overwrites. Later
voicing revisions should retain these same MIDI performances. These are demos,
not captures of private Logic projects or user playing.

## Evidence / eval-005, 0.7.5

Stereo 48 kHz / 24-bit production-processor exports, two repeated 64-frame passes
and a 257-frame comparison. Both new sounds repeat exactly and have **zero** max
sample difference across block sizes. Export/reimport preserves preset conditions
and identity. No numerical faults or master-ceiling hits; releases finish and
last-second RMS is at the −100 dBFS reporting floor.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Hollow Bronze r1 | −10.74 | −24.30 | +0.30 | 3 | `2f74846ed0d256f1` |
| Fault Bloom r1 | −13.04 | −24.48 | +0.48 | 1 | `81cc944aff0f76c1` |

Previews use −24 dBFS gated active RMS, not LUFS. Preview gain is excluded from
preset values. Before freezing r1, only output was calibrated from initial
development levels (material −5 → −2.5 dB; experimental −5.5 → −0.5 dB) for more
comparable live levels; that development capture is separate diagnostic evidence,
not a published/user-tested revision. Final source hashes refer to the final levels.

On the Apple M4 Max, two complete passes at 48 kHz / 64 frames measured material
mean occupancy **3.35%**, p99 **152.96 µs**; experimental **3.38%**, p99 **65.25 µs**.
Zero measured callbacks exceeded the **1,333.33 µs** deadline. These exact demos
include silence/releases and use three/one voices: not sustained maximum-polyphony
or real Logic/device-xrun qualification. Open F1 96 kHz limits remain unchanged.

## Trial

Programs 12/13 (zero-based) append after all earlier programs:

- `Review / Physical materials / Hollow Bronze r1`
- `Review / Experimental / Fault Bloom r1`

Use `evaluation/CURRENT.md` for the actual installed build/trial and next action.
Source/MIDI, raw/preview audio, reports, build/source snapshots, feedback and
decisions persist outside `build/`. No commit or final library approval is implied.
After listening/refinement of this pair, develop contrasting alternatives toward
two candidates per category, then collection-wide voicing/performance review.
