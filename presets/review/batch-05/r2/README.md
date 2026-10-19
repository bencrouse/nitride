# Reed Snap / r2 — alternate harmonic/transient profile

Feedback likes Reed's voice but finds the cool distortion character repeated
across patches. Undertow is liked and its contrast affirmed. This is collection-
wide diversity feedback, not a directive to clean every sound or merely reduce
drive. All kept sounds and Reed r1 remain unchanged/selectable.

## Investigation and limits

Code inspection shows several shared coloration stages: the cubic FM modulator
shape, stress-dependent network feedback/folding, and a common post-engine tanh
shaper. The plugin master hard clamp is a separate safety stage. **Zero master
ceiling hits do not imply absence of soft saturation.** No dominant perceptual
cause of the user's repeated signature has been established.

Uninstalled eval-009 diagnostics used identical Reed MIDI: baseline, triangle at
the same .6 amount, transient-shaped triangle, a 2:1 variant, zero-stress control,
and unchanged Tension/Fault definitions on common notes. Inputs/raw/preview audio,
reports and the analytical coefficient table are preserved as diagnostic evidence,
not new published revisions or replacements for old frozen reference performances.

At the same .6 amount, the cubic/saturated modulator has a positive third harmonic
and no fifth/seventh/ninth term; the finite triangle has a negative third and
alternating higher odd terms. This demonstrates a different **geometry**, not
simply a lower amount. The full network output is not equivalent to this primary-
modulator coefficient table, and the common output shaper is retained.

## Focused r2 recipe

| Condition | r1 | r2 |
| --- | --- | --- |
| FM shape | Soft saturated | Triangle |
| Base shape amount | .6 | .35 |
| Route 2 source | Off | Filter envelope |
| Route 2 target | Depth | Shape |
| Route 2 amount | 0 | .3 |
| Output (dB) | −2.5 | −2.65 |

R2 makes the nominal shape amount vary .35–.65 with the existing fast filter
envelope: fuller odd-harmonic attack, a different body profile after the strike.
The filter envelope is the existing shared Tone source, not a newly invented
per-voice envelope. Coupling .52, stress .12, Response .035, FM depth .55, ratio 1,
index envelope/amplitude route, resonant Tone, velocity response, no-LFO condition,
poly/retriggered articulation, Space and the frozen MIDI remain identical.

This is an alternate source/time-behavior **hypothesis**, not proof of a distinct
distortion family or a global fix. No engine curves, signal flow, host contract,
quality/oversampling policy or global voice limit were changed. If the shared
signature remains, revisit the source/voicing or discuss explicitly scoped DSP
options rather than pretending this preset edit resolved the common mechanism.

The untrimmed transient probe raised active RMS about .148 dB. A −.15 dB output
trim avoids a louder new version: raw r1/r2 levels **−24.279 / −24.281 dBFS** are
within **.002 dB**. Preview gain remains separate, targets −24 dBFS gated active
RMS, and is not LUFS matching. No darker filter or drive reduction is substituted
for a different profile.

## Evidence / eval-010, 0.7.10

20 s stereo 48 kHz / 24-bit production-processor exports with byte-identical
112 BPM Reed MIDI. Two 64-frame renders repeat exactly; the 257-frame comparison
has **zero** maximum sample difference. Export/reimport preserves identity and
conditions. Maximum two voices, no numerical faults/master-ceiling hits, completed
releases and final-second RMS at the −100 dBFS reporting floor.

- Raw float hash: `2f7dd79ed1f75bd8`.
- Raw peak **−9.66 dBFS**, active RMS **−24.28 dBFS**, preview gain **+.28 dB**.
- Apple M4 Max, two complete passes at 48 kHz / 64: mean occupancy **2.41%**,
  p99 **115.04 µs**, zero measured callbacks beyond **1,333.33 µs**. Rests/releases
  are included; not maximum-polyphony or Logic/device-xrun qualification. Open F1
  96 kHz headroom limits remain unchanged.

Program 21 appends as `Review / Leads / Reed Snap r2`. Read `evaluation/CURRENT.md`
for actual installed build/trial and next action. A/B the same runs/stabs/dyads,
notes/velocities/effects. Does attack-to-body character actually feel different
while preserving the liked lead role, or does it still have the same familiar
grit? Either answer is useful; no musical verdict is inferred from these checks.
