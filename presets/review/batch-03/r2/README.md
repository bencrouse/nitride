# Hollow Bronze / r2 — narrower ring, retained strike

Feedback: "hollow bronze is good but sounds slightly buzzy, correct direction
though." R1 remains unchanged/selectable. Fault Bloom is not revised: the user
accepts its weirdness and wants diverse experimental behavior, not conventional
good/bad polish. The other four kept r2 directions are untouched.

## Focused recipe

Only two conditions change:

| Condition | r1 | r2 |
| --- | ---: | ---: |
| FM depth | 0.42 | 0.22 |
| FM envelope amount | 2.2 | 2.4 |

The nominal FM index at envelope peak stays **2.62**; after the same 55 ms FM
decay, the base FM index is lower. Aim: fewer settled sidebands in the ringing
body without dulling the short strike. The peak sum is not a promise of identical
attack samples: envelopes, smoothing and the network still shape the waveform.

2.73:1 ratio, sine modulator, operator mix, network dimensions, filter, velocity
response, zero-sustain amp decay/release, Space, output and articulation are all
unchanged. MIDI is byte-identical to r1's fixed 80 BPM strike/held-decay/dyad demo.
No engine, host controls, quality policy or global polyphony changes are made.

## Investigation, not a confirmed perceptual diagnosis

Three offline diagnostic candidates used the production eval-005 renderer and
the same frozen MIDI:

- Baseline r1 conditions.
- Lower settled FM depth alone (.42 → .22).
- More-damped filter body (cutoff 4900 → 3300 Hz, envelope amount 1.6 → 2.17 to
  retain the initial opening).

The filter probe barely changed the sampled body spectrum. Lower settled FM
produced a clearer contrast, so r2 uses that branch with strike compensation.
Diagnostics are separate uninstalled variants, not published revisions.

For the final compensated r2, Hann-windowed mono power spectra of three 21 ms
attack windows and four 171 ms body windows have these mean descriptors:

| Stage | r1 power centroid | r2 power centroid | Change |
| --- | ---: | ---: | ---: |
| Attacks | 892.44 Hz | 879.43 Hz | −1.5% |
| Bodies | 282.40 Hz | 250.74 Hz | −11.2% |

These coarse descriptors are **not a perceptual buzz measure or proof of a unique
cause**. The waveform may still feel buzzy, or lose desirable complexity. The
standard-library FFT was checked against direct DFT; raw audio, input variants,
analysis script and JSON measurements are preserved in the evaluation workspace.

## Evidence / eval-006, 0.7.6

Stereo 48 kHz / 24-bit production-processor exports. Two 64-frame passes repeat
exactly and the 257-frame comparison has **zero** maximum sample difference.
Preset export/reimport and MIDI preserve identity/conditions. No numerical faults
or master-ceiling hits, max three voices, completed releases and last-second RMS
at the −100 dBFS reporting floor.

- Raw float hash: `1a27b1be2ed87d76`.
- Raw peak: **−10.73 dBFS**; gated active RMS: **−24.31 dBFS**.
- Preview gain: **+0.31 dB**, separate from preset values; previews target −24 dBFS
  gated active RMS, not LUFS.
- R1 active RMS was −24.30 dBFS; the difference is only −0.0071 dB. Output was not
  changed to create a perceived improvement.
- Apple M4 Max, two complete 26 s passes at 48 kHz / 64 frames: mean occupancy
  **3.45%**, p99 **156.50 µs**, zero measured overdue callbacks against the
  **1,333.33 µs** deadline. Rests/releases are included; this is not sustained
  max-polyphony or Logic/device-xrun qualification. F1 96 kHz limits remain open.

Program 14 (zero-based) appends as `Review / Physical materials / Hollow Bronze r2`.
All earlier programs remain available. See `evaluation/CURRENT.md` for the actual
installed build/trial; final musical preference and library approval are pending.

## Listening

A/B r1/r2 with the same MIDI/velocity and channel-strip effects. Does the ring
feel less buzzy while the strike and metallic body remain? Check quiet/loud hits,
registers and the held-key decay. Prefer r1 if the new body feels too plain.
