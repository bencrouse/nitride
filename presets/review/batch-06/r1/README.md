# Batch 06 / r1 — contrasting materials and experimental behavior

Reed r2 is now kept following explicit improvement feedback. This pair completes
the initial breadth pass: twelve distinct families, two per category, not final
library approval. Aim for different playing response and texture, not near-variants
of the familiar grit. All ten kept directions remain unchanged.

## Skin Knock / Physical materials

Contrast Hollow Bronze's ringing, inharmonic strike with short, dry, pitch-relaxing
impacts suggesting a stretched skin or damped membrane:

- Poly/retriggered, no chord helper or cyclic motion. 1 ms attack, 240 ms decay,
  zero sustain, 90 ms release; Space .025 instead of a ringing halo.
- Every hit starts with a +7-semitone bend returning to the played pitch over
  26 ms via the existing cubic Autobend decay. Carrier/modulators move together;
  phrase-only is off so rolls retrigger the relaxation.
- Nominal .5:1 triangle FM with a brief 22 ms index strike, fast network response,
  and a velocity-sensitive short Tone opening. This is a material **impression**,
  not a new physical-model algorithm or a sampled drum.

Try low/mid-register taps with varied velocities, then a roll and actual dyads.
The 100 BPM frozen demo includes a velocity ladder, roll, dyad and longer-held
strike; last note-off is 11.7 s in an 18 s render. Listen for membrane-like tension
and damping versus simply a duller bronze sound or an ordinary bass pluck.

## Orbit Shards / Experimental

Contrast Fault Bloom's stressed mono, opposed-glide, continuous deformation with
low-stress polyphonic inharmonic glints and stepped pitch motion:

- Poly/retriggered, no glide, no Autobend, modest coupling .16 and stress .015.
  Sine FM at nominal 3.71:1 and .32 direct operator mix exposes unequal partials.
- A shared free-running sample-and-hold pitch lane at .41 Hz, depth 9 semitones,
  moves held notes/dyads together. Its deterministic offsets are continuous-valued,
  **not scale-quantized** or independently randomized per voice. Existing slew
  smooths transitions; no MIDI notes are generated.
- Slower .13 Hz FM motion shifts index and ratio; the filter envelope moves
  operator mix and network activity contributes modest index motion. Space .42
  with size .77 leaves diffused traces after the dry voice has changed pitch.
- Hold sparse notes/dyads long enough for pitch steps; transport need not play
  because both active LFOs are free-running. Live phase depends on prior playback;
  the fixed renderer starts from the same prepared state for revision comparisons.

The 96 BPM demo alternates isolated notes, held notes and overlaps/dyads. Last
note-off is 20 s in a 28 s render. Judge the drifting/fragmented behavior and its
contrast with Fault Bloom, not a conventional good/bad score. A cleaner or prettier
tone alone is not proof of a distinct experimental identity.

## Frozen and shared-note performances

`make_midi.py --verify` reproduces the SMF bytes and refuses overwrites. The files
use the existing standard-library encoder, fixed tempos and 4/4. Later revisions
should preserve these performances for voicing comparisons.

Separate shared-note diagnostics use identical impact/roll/dyad MIDI for Hollow
Bronze/Skin Knock and identical held-note/dyad MIDI for Fault Bloom/Orbit Shards.
Original definitions/IDs stay unchanged; old frozen performances are not replaced.
Bronze accumulates up to six tail voices versus two short impacts; Fault stays mono
versus three Orbit voices. These are intended differences, not proof of sufficient
musical range. Preview level normalization is separate from each preset's output.

## Evidence / eval-011, 0.7.11

Stereo 48 kHz / 24-bit production-processor exports with preset export/reimport
preserving identity and all conditions. Both same-block repeats are exact and
64/257-frame maximum sample differences are **zero**. No numerical faults or
master-ceiling hits, completed amplitude releases and final-second RMS at the
−100 dBFS reporting floor. Soft coloration still exists despite zero hard-clamp hits.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Skin Knock r1 | −10.81 | −24.00 | −.001 | 2 | `11e0b22d2cd80e3f` |
| Orbit Shards r1 | −9.64 | −24.08 | +.083 | 3 | `df4c5e2671796234` |

Previews target −24 dBFS gated active RMS, not LUFS; preview gain is excluded from
preset values. Before r1 freezing, only output was calibrated (Skin −2 → −3 dB;
Orbit −5 → −.75 dB). Gating windows can shift with absolute level, so measured
active-RMS changes need not exactly equal the output delta. The uninstalled
eval-010 development capture is archived separately, not a published revision or
a musical improvement verdict.

Apple M4 Max, two full demo passes at 48 kHz / 64: Skin mean occupancy **1.60%**,
p99 **101.08 µs**; Orbit **5.44%**, p99 **165.29 µs**. Zero measured callbacks beyond
the **1,333.33 µs** deadline. These demos include rests/releases and only two/three
voices; not max-polyphony or Logic/device-xrun qualification. Builds ran concurrently
with diagnostic work, so timing is observational, not an isolated performance claim.
Open F1 96 kHz limits remain unchanged.

Programs 22/23 append as `Review / Physical materials / Skin Knock r1` and
`Review / Experimental / Orbit Shards r1`. No DSP architecture, signal flow,
quality/oversampling policy, host contract or global voice cap changed. Read
`evaluation/CURRENT.md` for actual installed build/trial and next action.
