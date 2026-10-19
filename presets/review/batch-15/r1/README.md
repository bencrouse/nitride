# Batch15 / r1 — requested DX7 VOICE1, ORCH-CHIME, KOTO

Original Nitride approximations of exactly the user's one-based ROM1A slots
**30/25/23**. [Raw source/operator/modulation audit and A/B/C packet](../../../../evaluation/dx7-classics/pass-002/README.md)
reuse the frozen original bank without modifying earlier recipes/evidence.

| Source slot | New voice | Nitride host program0-based | Kept anchor |
| ---: | --- | ---: | --- |
|30 VOICE1 |Classic Voice / Pads |57 |Stillwater |
|25 ORCH-CHIME |Orch Chime / Physical materials |58 |Hollow Bronze r2 |
|23 KOTO |Classic Koto / Physical materials |59 |Rosewood Tap |

**Classic Voice:** MIDI60 held notes/chords/velocities. Soft poly near-unison FM/tap,
shared Tone vowel color, triangle vibrato/index flutter. No dedicated formant bank,
sampled voice, exact multi-branch/detuned-operator/pitch-EG copy or original delayed
operator AMS. The software A reference is **PARTIAL for operator AMS**, explicitly
flagged in its filename and guarded by a required tool opt-in; original data intact.
Pitch LFO/delay/EG ARE rendered. Do not call A an exact/full-feature hardware target.

**Orch Chime:** MIDI60 strikes then3–4s holds, sparse55/67/dyads. Fast6.28ratio chime
plus later index/body contour, not separately layered orchestral strings. **Pitch−12**
preserves the source low.5carrier mapping; source/native input unchanged, only the
old Hollow comparator's MIDI−12 aligns sounding root. Its upper octave layers remain
absent. New notes can retrigger the shared filter-envelope-to-index body contour.

**Classic Koto:** MIDI60 velocity-varied plucks/runs/dyads and held57.4:1 nail/string
front into damped body; a short shape strike approximates extra harmonic density,
not the original serial3/4/1stack/feedback7/operator touch. Native reduced immediate
.05st sine vibrato substitutes for unavailable delayed/reset .1575st source motion.
Source AMD15 has all operator AMS0: no global tremolo is invented from that number.

All3are dry/poly/free-running modulation, **no transport required**. No glide/
Autobend/legato/global Stress/granular/noise/new algorithms or host controls added.
Native shared free LFOs lack source note-triggered delay/reset/phase behavior;
these compromises are explicit. Short gates still enter release, not one-shot mode.
No model code/raw factory bank is linked/embedded into the AU. Earlier sounds stay.

## Frozen evidence /0.7.23, eval-023

`make_classics.py --verify` reproduces3presets/3original MIDI/manifest; creation
refuses existing paths. Voice100BPM/34s, Orch100BPM/32s, Koto110BPM/26s. Original
raw data/selectedVMEM/global shared pitch-LFO model/F64 repeats/actual stdout logs/
modulation guard/current-anchor comparisons are frozen separately. Native48kHz/
reference44100Hz retained, no cross-engine null/likeness score or hardware accuracy
claim. Previews target−24dBFS gated100ms active RMS, not perceptual/LUFS matching.

Own production processor preset export/reimport identity/conditions, exact same-
block repeats and zero64/257 maximum differences; no faults/master-ceiling hits,
finished amplitude releases and finalsecond at−100dBFS reporting floor. All6anchor
captures also pass; source-model repeat is a separate qualified test.

| Voice | Raw peak dBFS | Active RMS dBFS | Preview gain dB | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
|Classic Voice |−12.52 |−24.00 |+.002 |3 |`4a699d553a010a65` |
|Orch Chime |−11.24 |−24.05 |+.049 |2 |`e36595f3deb77795` |
|Classic Koto |−10.49 |−24.20 |+.201 |2 |`913b2383a7f7ffe5` |

Only output calibrated pre-freeze:−6→−7.15/−3.35/−2.45dB; eval-022 development
captures retained separately. Whole-demo Apple M4 Max48kHz/64, two passes: mean
occupancy3.73/2.29/2.30%, p99139.21/94.96/95.92µs, zero measured overdue callbacks
against1,333.33µs. Rests/tails/at most3/2/2voices, not maximum-polyphony/device-xrun/
cross-rate or renewed full sanitizer qualification. F1/Socket-new120 remain open.

37kept families unchanged; prior Classic Tines/Bass/Tubes still pending, not approved
from acknowledgement. These3also await likeness/utility/overlap preference.43families,
56review revisions+4original programs. No commits/pushes. CURRENT names active trial.
