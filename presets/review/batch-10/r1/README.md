# Batch 10 / r1 — Nitride interpretations of FM archetypes

The user approves a recognisable FM-essentials layer, not exact DX7 recipes or
copies of acoustic instruments. Four original auditions complement the character
patches; do not add several variants merely to fill a factory-bank checklist.
Each must earn a musical job, with the dry slap explicitly compared against
Crooked Velvet. All twenty-four retained family directions remain unchanged.
The liked electric-key voice is separately continued as **Tine Keys**, name only.

## Rosewood Tap / Physical materials / program 40

**Brief:** warm pitched wood, quick damping, no falling membrane pitch or lingering
bronze ring. Poly/retriggered, 1.5 ms attack, .38 s decay, zero sustain and .12 s
release. Nominal 3:1 sine FM with a brief 32 ms-decay index strike, no direct operator
mix, small low-stress network and very little Space. No LFO/glide/Autobend.

Try repeated mid-register notes at different velocities, a short run, then actual
dyads. Does attack/body feel usefully wooden without becoming dull, bass-like or
another bell? The original 104 BPM demo ends at about 12.40 s in a 20 s render.
Compare Hollow Bronze on the same notes and Skin Knock on its own lower-register
performance. The impression is not sampled marimba or a physical-model claim.

## Clear Stops / Leads / program 41

**Brief:** a compact sustained organ-like chord voice, not an evolving pad.
Poly, 4 ms attack, sustain 1 and 65 ms release; stable nominal 2:1 carrier/operator
blend with small triangle shaping, low oscillator FM and .08 coupling/zero stress.
No FM envelope amount, Tone envelope amount, cyclic motion or Space. Input
velocity still scales amplitude; there is no velocity-insensitive organ switch.

Try detached runs and held two/three-note chords. Does even dry sustain give a
useful keyboard role beyond Reed Snap's articulated decay? Demo 100 BPM, last
note-off 16.2 s, 24 s render. This is not a drawbar/tonewheel, key-click, rotary or
automatic chord engine. The existing shared Tone and per-voice network remain.

## Copper Lift / Leads / program 42

**Brief:** a harmonic chord stab that can open into a solid held body, rather than
a mono gliding lead. Poly, 28 ms amplitude entrance/.72 sustain/.18 s release;
nominal 1:1 triangle FM with 35 ms index rise and 45 ms Tone-envelope rise. Both
settle after the entrance. Modest low-stress coupling and a small positive network-
to-index route give Nitride color without relying on extra movement or large Space.

Try short chord accents, then hold through the swell. Is the articulation brass-like,
clear and dependable, or too soft/harsh/indistinct? Demo 112 BPM, last note-off about
12.86 s, 24 s render; compare Glass Current on identical notes (mono response is an
intentional contrast). No breath/noise source or acoustic brass model is added.
Tone is shared, so rearticulating notes can reshape the held chord globally.

## Pocket Slap / Basses / program 43

**Brief:** a concise bright index pop over a clearly played bass note, dry and easy
to place rhythmically. Mono/retriggered, 2 ms attack, .22 s decay, .38 sustain and
75 ms release; nominal 1:1 sine FM, base index .16 with a 35 ms-decay strike. Zero
operator mix/Space, no automatic bend, glide, cyclic wobble or other FM routes.

Play low ghost accents, short phrases, overlaps and a few held notes. The original
108 BPM demo ends about 13.61 s in a 20 s render. Compare Crooked on this MIDI and
both on Crooked's original irregular 92 BPM MIDI: is this genuinely a simpler,
useful job, or already adequately covered? Familiarity is a goal, not a reason for
automatic approval; do not claim an exact named classic FM-bass preset recreation.

## Frozen recipes and fair comparisons

`make_archetypes.py --verify` reproduces four complete presets, four original MIDI
files and the manifest. It imports only the earlier frozen generator's explicit
neutral settings/standard MIDI encoder; no earlier voicing is silently inherited.
All 88 condition keys are checked. Creation refuses existing targets; later
revisions must not overwrite r1 or its performances. MIDI has one fixed tempo/4/4.

Four uninstalled contour controls preserve output/MIDI while removing:

- Rosewood's index envelope amount.
- Clear Stops' direct operator blend.
- Copper's index **and** Tone envelope amounts (a paired contour removal, not a
  single-knob isolation).
- Pocket's index envelope amount.

Reference pairs preserve old definitions/IDs and use common notes; reverse-MIDI
checks also compare Pocket/Crooked on Crooked's original performance and Rosewood/
Skin on Skin's original performance. Both register and arrangement can otherwise
masquerade as sound-design differences. Original frozen performances and old preset
levels are never replaced. Normalized previews reduce level bias but are not
perceptual matching or proof that a useful distinct job exists.

All controls and twelve reference captures repeat exactly with zero 64/257 max
sample difference in these contexts, no faults/master-ceiling hits and finished
amplitude releases. No new timing issue appeared here; this does not repair or
supersede the explicitly unqualified Socket-on-new-120-BPM case from batch 09.

## Production evidence / eval-017, 0.7.17

Stereo 48 kHz / 24-bit production exports, exact preset export/reimport identity/
conditions, exact same-block repeats and zero 64/257 differences on all four own
demos. No faults/master-ceiling hits; completed releases and final-second RMS at
the −100 dBFS reporting floor.

| Sketch | Raw peak (dBFS) | Active RMS (dBFS) | Preview gain (dB) | Voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Rosewood Tap | −10.65 | −24.15 | +.147 | 2 | `e34ad578c5675eb7` |
| Clear Stops | −15.35 | −24.01 | +.010 | 3 | `45dc9ae287d5460d` |
| Copper Lift | −11.88 | −24.02 | +.017 | 3 | `8ca4b97c6b6c1f65` |
| Pocket Slap | −14.70 | −24.00 | −.003 | 1 | `afae371823d129d5` |

Only output was calibrated before freezing: Rosewood −5 → −2.8 dB; Stops −9 →
−8.9 dB; Copper −6 → −7.25 dB; Pocket −5 → −2.95 dB. Gated windows can shift with
level, so active-RMS changes need not equal the gain delta. The eval-016 development
capture is archived separately, not relabeled final or treated as musical approval.
Previews target −24 dBFS gated active RMS, not LUFS; their gain is not stored output.

Apple M4 Max, two full demos at 48 kHz / 64: mean occupancy **1.81 / 3.31 / 3.43 /
1.66%**, p99 **105.63 / 125.38 / 157.96 / 51.29 µs** in table order. Zero measured
overdue callbacks against **1,333.33 µs**. Includes rests/releases and only two/
three/three/one voices, not sustained maximum-polyphony or Logic/device-xrun
qualification. F1 96 kHz and the separate reference-clock qualification remain open.

Twenty-eight families now exist: twenty-four retained directions plus four new
archetype auditions. Tine's rename adds a revision, not a family. Forty review
revisions/four original factory sounds; no final library approval. No algorithms,
global curves, host contracts, quality/voice policy or progressive UI changes.
Read `evaluation/CURRENT.md` for actual installed build/trial and next action.
