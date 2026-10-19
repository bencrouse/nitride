# Batch 02 / r2 — gentler lead, stepped harmonic pluck

Feedback: "Glass Current sounds good with a little less stress and coupling.
pulse lattice sounds great, though not very exciting or novel."

The user no longer has the adjusted lead open. Its exact values are unknown;
this is a modest proposed reduction, **not a reconstruction of the user's edit**.
Both r1 revisions remain unchanged/selectable, and r2 uses byte-identical copies
of their fixed MIDI performances. The liked pad/bass r2 directions are untouched.

## Glass Current

Only two base conditions change:

| Condition | r1 | r2 |
| --- | ---: | ---: |
| Coupling | 0.52 | 0.46 |
| Stress | 0.09 | 0.065 |

Response, slow motion depths/rates, FM, vibrato, legato glide, envelopes, filter,
Space and output are identical. Aim: retain the liked voice with less network
intensity. The same 100 BPM demo exercises separated/overlapping notes, velocity,
held-note pitch-wheel motion and release. Listening preference is pending.

## Pulse Lattice

Aim: a recognizable harmonic-color counter-pattern, not simply a louder,
brighter or wetter pluck. The amplitude/FM/filter envelopes, velocity sensitivity,
cutoff motion, coupling/stress, pitch settings, Space and output remain unchanged.

| Condition | r1 | r2 |
| --- | --- | --- |
| FM LFO waveform | Triangle | Sample & Hold |
| FM LFO division | 4 bars | Half-note triplet |
| Route 3 source | Off | FM LFO |
| Route 3 target | Depth | Ratio |
| Route 3 amount | 0 | 0.11 octaves |

The existing 0.12-index FM-depth motion now takes stepped values, and the same
clock makes a small FM-modulator-ratio excursion around 3:1 (bounded roughly
2.78–3.24). In 4/4, three color updates per bar play against the eight eighth-note
attacks/cutoff cycles. The deterministic S&H hash follows musical position, with
the existing slew softening step edges. Returning to a position reproduces its
color values; the waveform still depends on played notes and envelope history.

Carrier note/pitch controls are unchanged; moving FM sidebands can nonetheless
sound more inharmonic. **MIDI still supplies notes; no internal sequencer was
added.** Use a repeating eighth/sixteenth-note loop with transport playing.
The fixed eight-bar 120 BPM demo is identical to r1, so a new MIDI pattern or
velocity arrangement is not responsible for the timbral contrast.

Listen for: a distinctive but usable loop identity, retained articulation, and
whether the color changes become distracting, harsh or apparently out of tune.
Prefer r1 if its simpler texture fits better. Novelty is a listening question,
not a conclusion from an audio hash or a new route.

## Timing defect found by the candidate

The first stepped candidate failed the existing 64/257-frame render tolerance.
A standalone LFO diagnostic isolated a one-sample synced S&H cycle-hash mismatch
at sample 128000 (48 kHz), caused by accumulated PPQ additions. The 20 s test had
a maximum FM-wave difference of 0.00937156. No candidate AU had been installed.

`DSP::LfoBank` now derives synced S&H position from the host block's PPQ anchor
plus elapsed sample count, resolving floating-point-near integer cycle boundaries
with a tolerance capped at 1e−12 cycles. Continuous-wave/free-running interpolation
arithmetic remains unchanged; no network rewrite, quality reduction or voice cap
was introduced. The diagnostic now has **zero** difference. Long 48/96 kHz runs
at 64/257/1024 frames, negative PPQ/3-beat meter, genuine pre-boundary positions
and stopped seeks are covered by regressions. Diagnostic source/before/after
results are retained under `evaluation/diagnostics/`.

## Evidence / eval-004, 0.7.4

Stereo 48 kHz / 24-bit production-processor exports using the same MIDI/tempo and
durations as r1. Both r2 renders repeat exactly and have **zero** maximum sample
difference between 64/257 frames. Preset export/reimport and frozen MIDI preserve
conditions/identity. No DSP faults or master-ceiling hits; releases finish and
the final-second RMS is at the −100 dBFS reporting floor.

| Revision | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Glass Current r2 | −15.66 | −23.95 | −0.05 | 1 | `ea36dbfa748323b6` |
| Pulse Lattice r2 | −10.36 | −25.21 | +1.21 | 3 | `5e7a197de90cf4f4` |

Previews use −24 dBFS gated active RMS, not LUFS. Preview gain is separate from
stored output levels; no r2 output changes were made. Same-build r1 hashes remain
`c551b737ebc33fbf` / `939b9642e11ab58a`; the liked pad/bass r2 hashes remain
`264bbbbede795063` / `db07c509e8f743a1`.

On the Apple M4 Max, two complete 64-frame demo passes measured lead mean
occupancy **3.35%**, p99 **67.79 µs**; sequence **3.04%**, p99 **110.42 µs**.
Zero measured callbacks exceeded the **1,333.33 µs** deadline. These demos include
rests/releases and use one/three voices; they are not sustained maximum-polyphony
or Logic/device-xrun qualification. Open F1 96 kHz limits remain unchanged.

Programs 10/11 (zero-based) append after all earlier programs:

- `Review / Leads / Glass Current r2`
- `Review / Sequence elements / Pulse Lattice r2`

Read `evaluation/CURRENT.md` for the actual installed build/trial and next action.
All revisions, MIDI, reports, audio and build/source evidence persist outside
disposable `build/`; no commits or final collection approval are implied.
