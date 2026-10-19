# Batch13 / r1 — non-pitch-centered transitions, controlled first

Three authorized jobs, derived from dry fixed-source mechanism experiments rather
than finished presets/artist labels. See the
[full evidence/context/listener packet](../../../../evaluation/non-pitch-transitions/pass-001/README.md).

All three use fixed FMratio4/index1.4, zero coupling/stress, no FM envelope/routes,
no LFOs/Autobend/glide/filterFM and no automatic pitch movement. Original MIDI holds
note48 with no pitch-wheel messages; leave the live wheel centered. Fixed controls
are **not** proof of stable perceived pitch, especially the brief flash/reverb tail.
Native tests and source inspection enforce the actual conditions, not just names.

## Reverse Veil / Experimental / program51 (zero-based)

Dark held note develops over about4s through amplitude attack3.4s and Tone
attack3.5s/+4.5octaves. Release it into the next section; dry/.06s amplitude release,
not an automatic bar-length macro or reversed sample. Start MIDI48/velocity96.
Raw probes separately test static dark, amplitude-only and added Tone opening;
final ablation flattens **both** level and Tone contributions, not one parameter.

## Spectral Cut / Experimental / program52

Bright fixed source progressively loses upper harmonics over3.4s, ending muffled
before note-off. −5.7octave Tone amount; dry, .06s amplitude release. Hold MIDI48
about4s. Ablation removes only Tone amount. The negative filter envelope returns
toward bright during release: listen for an unwanted end accent, not a pitch fall.

## Flash Room / Experimental / program53

Brief1.5ms attack/65ms exponential-decay excitation leaves a diffuse room trail.
Zero sustain, .08s amp release, fixed Space .55/size .9. Try MIDI48 with a short
.25s gate at the section boundary, then let it clear. Still a gated synth voice,
not a noise crash, one-shot, pure-wet return or external-input processor. Ablation
removes only Space mix; its mapping also changes dry gain, not pure wet subtraction.

**Output0dB is intentional**, not a promise of high integrated loudness. Own raw
active RMS is−32.88dBFS and preview adds**+8.877dB**; judge stored/live/context levels,
not independently normalized preview apparent strength. This short-duty voice was
not force-matched to−24dBFS at the expense of its shape; level/masking is a listener
question. Compare raw/causal or documented-gain blind pairs.

## Musical handoff and missing evidence

No transport requirement for any of these. Envelopes use seconds, not bars. One
original8-bar120BPM arrangement substitutes for the unsupplied user/reference audio,
with boundary8s: Reverse/Cut start4s and release8s; Flash starts8s. The exact same
unchanged kick/sub/organ stems and common master occur in every comparison.

The packet includes baseline, full/ablated solo/context pairs, RMS/STFT evidence,
causal same-gain versions, anonymous labels/separate key and blank response forms.
Share only its blind ZIP with an independent producer. No reference match/listener
approval/perceived-pitch certification is claimed. Leave responses unfilled until
actually heard; final library selection remains open. All34 earlier families kept.

## Frozen production evidence /0.7.21, eval-021

`make_non_pitch.py --verify` reproduces all three presets/MIDI/manifest and creation
refuses existing files. Self-contained selected values match measured harmonic
probes except output: raw−6dB →−4.65/−5.8/0dB for Reverse/Cut/Flash. Own fixed120BPM
demos are20s: Reverse/Cut two4s holds plus an early .5s gate; Flash two .25s gates
plus a4s hold, all MIDI48/velocity96. Exact export/reimport, exact same-block repeats,
zero64/257 error; no faults/master-ceiling hits, completed amplitude releases and
finalsecond RMS at−100dBFS reporting floor.

| Candidate | Raw peak dBFS | Active RMS dBFS | Preview gain dB | Voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Reverse Veil |−17.07 |−24.21 |+.212 |1 |`d0881a9a73b70195` |
| Spectral Cut |−18.21 |−23.90 |−.097 |1 |`cfa35713b7f1a77d` |
| Flash Room |−14.45 |−32.88 |+8.877 |1 |`e584be3690a7f0f` |

Apple M4 Max, two own-demo passes48kHz/64: mean occupancy1.55/1.58/1.09%, p99
41.58/43.00/40.46µs, zero measured overdue callbacks against1,333.33µs. One voice
plus rests/tails, not max-polyphony, Logic/device-xrun, cross-rate or renewed full
sanitizer qualification. F1 combined96kHz and Socket-on-new120 reference context
remain open. Preview normalization is gated active RMS, not LUFS/perceptual matching.

37 families:34 retained plus these3 pending;50 review revisions plus4 originals.
Original states/indices/evidence remain. No global DSP/host-contract/quality/voice
policy changes, commits or pushes. CURRENT gives actual installed build/trial.
