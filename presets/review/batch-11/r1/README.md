# Batch 11 / r1 — dedicated kicks and tuned toms

The user explicitly requires **kick/tom voices and deliberate FX/transitions** in
the initial release. This batch starts with percussion; FX/transitions remain the
next required exploration, not an optional post-release extra. All twenty-eight
kept family directions and older program indices stay untouched.

Three original Nitride interpretations, not acoustic models, samples, noise layers
or exact drum-machine recipes. The current category is **Physical materials**;
no category/schema change is hidden in these auditions. Sound/role verdicts are
pending. Distinct parameters or diagnostics alone do not prove useful contrast.

## Floor Pulse / program 44 (zero-based)

**Brief:** round low weight, a quick downward front and a nearly bare settled body.
Mono/retriggered, 1 ms attack, .36 s decay, zero sustain and .16 s release. Coupling
.025/zero stress/base FM .015, nominal 1:1 sine FM with a small 12 ms index strike.
Autobend drops +24 semitones to the played pitch over 26 ms, affecting carrier and
modulators. Dry: no Space, direct operator blend, cyclic modulation or glide.

Start at **MIDI note 36**, velocity 90–115, roughly .4–.6 s gates; Pitch −3 puts
the nominal settled carrier at **55 Hz**. The fresh long-gate diagnostic's positive
crossings suggest 55.02 Hz between 80–250 ms, not a universal tuning/spectral claim.
Try notes 33–40, ghost hits and double hits. Is it a useful rounded kick rather than
just a low pluck? Original demo: 120 BPM, last note-off 14 s in a 20 s render.

## Rivet Kick / program 45

**Brief:** a hard compact synthetic front and shorter body, not Floor with more gain.
The taste brief is tactile synthetic percussion alongside the earlier SOPHIE-led
material direction, not an authentic artist recipe/endorsement. Mono/retriggered,
1 ms attack, .17 s decay, zero sustain and 85 ms release. Stronger coupling .34/
stress .065; nominal 1:1 soft-saturated FM with a 14 ms index strike of 2.6 above
base .1. Carrier-only +30-semitone Autobend settles over 12 ms while the modulator
stays at its played pitch. A short Tone contour gives the entrance more bandwidth.
No Space, operator blend, cyclic motion or glide; there is no separate click/noise layer.

Start at **MIDI note 36**, then short syncopated accents/ghosts and fast rolls.
Pitch −1 puts the nominal settled carrier near **61.74 Hz**; fresh-gate positive
crossings suggest 61.76 Hz in the same limited window. Does it cut in a genuinely
different way from Floor without recreating the collection's shared grit? Original
demo: 128 BPM, last note-off about 12.66 s in a 20 s render. Common-note comparisons
swap both kick performances so the arrangement does not decide the result.

## Bowl Toms / program 46

**Brief:** overlapping low/rack tom fills, a lightly inharmonic hollow body and a
gentle relaxing entrance rather than Skin Knock's short downward knock. Poly,
1.5 ms attack, .62 s decay, zero sustain and .28 s release; nominal ratio 1.48,
modest triangle shaping/base FM .22 and a smaller 60 ms index strike. +4.5-semitone
carrier/modulator Autobend over 65 ms, modest coupling/low stress and .035 small
Space. No cyclic motion, direct operator blend, glide or legato envelopes.

Try **MIDI notes 41–57**, descending fills with some overlap, velocities 65–118;
start with gates around .5–.8 s for the body. Pitch follows MIDI normally, not a
General MIDI drum map. Shared Tone follows note ownership, so the new accent can
reshape older ringing voices. Does it read as tuned toms with useful body, and earn
a place beside Skin? Compare both on the tom demo and original Skin MIDI.
Original demo: 108 BPM, last note-off about 15.83 s in a 22 s render.

## Important playing behavior: not one-shots

These presets do not bypass note-off or reset oscillator phase through a hidden
drum mode. Zero sustain lets a held hit decay naturally, but **short note gates
enter release early**. No new DSP, host controls or voice policy is added. Notes
36 etc. are MIDI numbers because octave labels differ between hosts.

Each own demo begins with three isolated velocity-varied hits, then a pattern/fill,
register tests and rolls, and finishes with a held hit versus a very short tap and
a longer gate. Separate fresh-instance diagnostics compare 25 ms and 1 s gates
at identical note/velocity/output: first **1,200 frames match exactly**, then the
short gate loses post-note-off energy. The 25–800 ms window is **−5.28 / −4.70 /
−4.80 dB** versus long gate for Floor/Rivet/Toms. This quantifies gate dependence,
not perceived loudness, bad/good articulation or one-shot equivalence. Compare
**raw** tails; independently normalized previews can obscure the envelope difference.

160 BPM sixteenth-roll diagnostics exercise velocity/retrigger and overlapping
release tails (max voices 1/1/4). Raw roll peaks are −14.46 / −12.43 / −9.70 dBFS.
Both dry kicks are exactly dual-mono in these roll captures; tom fold-down energy
differs by about −.00026 dB. That is limited evidence for these captures, not a
universal stereo/mix qualification. Long rests and all amplitude releases finish.

## Frozen inputs and diagnostic comparisons

`make_drums.py --verify` reproduces three complete presets, three original MIDI
files and the manifest. All 88 condition keys are explicit/checked; inherited
neutral settings and MIDI encoder come from the earlier frozen generator. Creation
refuses existing targets; registered sources/MIDI cannot be overwritten.

`evaluation/diagnostics/batch-11-percussion/` preserves preparation/analysis and
three uninstalled packets:

- **Controls (7 captures):** study versus only Autobend enable removed for each;
  Rivet additionally versus only index-envelope amount removed. Same MIDI/output.
- **References (10):** unchanged Skin on each study's notes; both kicks on each
  other's performance; Toms/Skin also on Skin's original MIDI. Old definitions/IDs,
  stored levels and frozen original performances remain intact.
- **Playing (9):** fresh short/long-gate pair plus a fast-roll capture for each.

All 26 captures repeat exactly with **zero 64/257-frame sample difference**, no
numerical faults/master-ceiling hits and completed amplitude releases. They do not
prove new musical territory, final approval, device performance or repair the
separate known Socket-on-new-120-BPM qualification.

## Production evidence / 0.7.18, eval-018

48 kHz stereo 24-bit exports through the production processor; export/reimport
identity/conditions preserved. Two 64-frame raw renders repeat exactly; 257-frame
maximum difference is **zero** for all three own demos. No faults/master-ceiling
hits; final-second RMS at the −100 dBFS reporting floor.

| Sketch | Raw peak dBFS | Active RMS dBFS | Preview gain dB | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Floor Pulse | −14.33 | −24.08 | +.082 | 1 | `e7f4b1b843047c1` |
| Rivet Kick | −12.95 | −24.13 | +.130 | 1 | `48bb5d77179b08dd` |
| Bowl Toms | −11.66 | −24.00 | +.001 | 3 | `5d0ec1947e6a7039` |

Pre-registration eval-017 development captures preserve the initial −6 dB output
for each. Only output was calibrated before freezing: −3.55 / −1.45 / −4.03 dB
respectively. Gated-window changes mean measured RMS deltas need not equal gain
deltas. Previews target −24 dBFS gated active RMS, not LUFS/perceptual matching;
preview gain does not alter stored output. Percussive peak/headroom and arrangement
still need musical comparison, not just this level calibration.

Apple M4 Max, two own-demo passes at 48 kHz/64: mean occupancy **1.81 / 1.63 /
2.86%**, p99 **47.08 / 57.00 / 144.21 µs**, zero measured overdue callbacks against
**1,333.33 µs**. Full demos include rests and at most one/one/three voices, with
concurrent local validation possible. This is not sustained max-polyphony,
Logic/device-xrun, cross-rate or renewed sanitizer qualification. Open F1 combined
96 kHz workload and Socket-at-new-120 reference clock limits remain unchanged.

Read `evaluation/CURRENT.md` for installed build/trial. There are now 31 study
families: 28 kept directions plus these three auditions, not final factory approval;
43 review revisions and four original factory programs. Deliberate FX/transitions
remain required next. No commits or pushes.
