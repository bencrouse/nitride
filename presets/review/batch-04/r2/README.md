# Prism Pulse / r2 — chord-friendly FM, rhythmic identity retained

Feedback: "prism pulese sounds a bit harsh, particularly in a chord. like in
direction of the pleasant fm pad tones i'm familiar with, but not beatiful enough."
The user likes Felt Weight and explicitly affirms its bass contrast; it is not
revised. All other kept directions and Prism r1 remain unchanged/selectable.

## Focused recipe

| Condition | r1 | r2 |
| --- | ---: | ---: |
| FM ratio | 2.5 | 2.0 |
| FM LFO → ratio amount (octaves) | 0.035 | 0 |
| Triangle shape amount | 0.45 | 0.18 |
| Operator mix | 0.09 | 0.04 |
| Output (dB) | −4 | −5.8 |

Aim: a more chord-friendly primary ratio without intentional ratio wander,
sine-richer modulator shaping and less exposed operator color. The fixed nominal
2:1 relationship is not a claim that the active network produces perfectly
harmonic/static partials, or that the user's harshness has a uniquely diagnosed
cause. Beauty/pleasantness still requires listening.

Quarter-note amplitude breathing, quarter-triplet cutoff, two-bar FM-index motion,
amp/filter/FM envelopes, cutoff/resonance, velocity/key tracking, network/Response,
Space, polyphony and playing articulation all remain unchanged. FM depth itself
stays 1.05: this is not a generic global darkening or convergence on Soft Alloy.
The faster, rhythmic held-chord role is retained. **Play transport** in Logic;
MIDI still supplies actual chords, not an internal note sequencer.

## Probes and comparison level

Uninstalled eval-007 diagnostics used identical frozen chord MIDI and four cases:
baseline, ratio/wander changes alone, shape/mix changes alone, and both combined.
The nominal non-integer ratio and its motion were plausible candidates for clang
in chords, but technical renders do not establish what the user heard.

The combined treatment without level trim raised gated active RMS about 1.81 dB.
R2 therefore trims output by 1.8 dB rather than comparing a louder new version.
Raw r1/r2 active levels are **−24.342 / −24.336 dBFS**, only **0.006 dB** apart.
The strike peak/waveform is not asserted identical. Preview normalization remains
separate from stored output values, targets −24 dBFS gated active RMS, and is not
LUFS matching. Probe scripts/inputs/audio/reports are archived separately.

R2 MIDI is a byte-identical copy of r1's four held-triad 104 BPM performance. No
new chord voicing or velocity arrangement is being used to imply an improvement.

## Evidence / eval-008, 0.7.8

Stereo 48 kHz / 24-bit production-processor exports. Two 64-frame passes repeat
exactly; 64/257 max sample difference **1.49e−8**, below one 24-bit PCM step, not
zero. Preset export/reimport and MIDI preserve identity/conditions. Maximum six
voices, no numerical faults/master-ceiling hits, completed releases, and final
second RMS at the −100 dBFS reporting floor.

- Raw float hash: `cd8fec82bbafb3b2`.
- Raw peak: **−11.71 dBFS**; active RMS: **−24.34 dBFS**.
- Preview gain: **+0.34 dB**, not part of the preset.
- Apple M4 Max, two complete 26 s passes at 48 kHz / 64: mean occupancy **9.37%**,
  p99 **314.42 µs**, zero measured callbacks beyond the **1,333.33 µs** deadline.
  Rests/releases are included; not max-polyphony or Logic/device-xrun qualification.
  F1 96 kHz headroom limits remain open. No DSP architecture, host contract,
  oversampling/quality policy or global voice limit was changed.

Program 17 (zero-based) appends as `Review / Pads / Prism Pulse r2`. Read
`evaluation/CURRENT.md` for actual installed build/trial and next action.

## Listening

A/B r1/r2 on the same chords, notes/velocities and effects with transport playing.
Does r2 reach a more pleasant/beautiful FM-pad tone without losing crystalline
rhythmic character or becoming another Soft Alloy? Test single notes and chords;
record remaining harshness rather than treating parameter choices as a verdict.
