# Batch 09 / r1 — six frontier audition studies

The user asks to audition conclusions from the range map with actual patches.
These are original playable mechanism studies with taste briefs, **not proven new
library roles or automatically approved additions**. All eighteen kept directions
and earlier revisions remain intact. No DSP algorithm, signal-flow, host contract,
quality reduction or global voice cap is changed.

## Start here: foundation and keys

### Sub Canvas / Basses / program 33

**Brief:** an unobtrusive floor under character sounds, not another clever bass.
Zero coupling, stress, oscillator FM, routes and Space; mono/retriggered with .85
sustain and .11 s release. Nominal ratio 1, no modulation/glide. Common soft ceiling
and Tone mean this is a near-sine impression, not a mathematically pure sine.

Try low notes, ghost accents and longer holds. Compare Felt Weight's damped pluck
on the same MIDI: does the steady body fill a genuinely useful job? Demo 100 BPM,
last note-off 12.6 s, 18 s render. Judge low-register behavior on appropriate
monitoring; no actual user monitoring settings have been inferred.

### Afterimage Keys / Leads / program 34

**Brief:** a key whose short strike leaves a second spectral image when held.
Poly/retriggered, nominal 2:1 sine FM, modest coupling. Independent FM envelope
gives a brief 90 ms-decay index strike; the separate filter envelope takes .45 s
to rise and routes +.75 to **FM depth**, while its Tone-envelope amount stays zero.
This explores the previously missing configured matrix pair without inventing a
second layer or sampled piano. Small Space, .35 sustain and .25 s release.

Try taps, then hold through the delayed contour and play dyads. Does the second
stage sound like useful articulation rather than a distracting swell? Demo 100
BPM, last note-off 15 s, 22 s render; compare Reed Snap on identical MIDI.

## Then partials and fast motion

### Overtone Lantern / Experimental / program 35

**Brief:** treat the modulator as the main voice, not seasoning on the carrier.
Direct operator mix .86, nominal 5:1 triangle operator with .7 shape amount,
modest oscillator FM/network, .42 sustain/.55 s release and diffused Space.
No pitch-step LFO, glide or automatic bend. The strong upper partial can imply a
higher pitch than the played key; it is not intended to hide a bass fundamental.

Try sparse low/mid notes, holds and dyads, comparing Orbit Shards' pitch-stepped
glints. Is the exposed-partial identity interesting without those steps? Demo
100 BPM, last note-off 15.6 s, 24 s render. Not a new oscillator algorithm.

### Buzz Loom / Sequence elements / program 36

**Brief:** a sustained thread roughened by a fine clock, against a slow reset.
Mono sustained body; **sine unipolar AM at 1/64 triplet**, .95 depth, gives 48
cycles/s at 120 BPM. A separate **free .25 Hz saw** moves Tone over four-second
ramps. Modest oscillator/network complexity and little Space keep the motion clear.
This is AM/sidebands, not bipolar ring modulation, a sampler or MIDI sequencing.
The modulation clock is tempo-relative, not key-tracked.

**Play transport**, preferably 120 BPM for the documented study, and hold different
low/mid notes. Does fine flutter become a useful object rather than faster ordinary
tremolo? Demo last note-off 16 s, 22 s render. The Socket comparison uses identical
**Socket 118 BPM MIDI** for both definitions: Buzz AM is then 47.2 cycles/s.

## Finally filter color and self-restraint

### Cutoff Alloy / Physical materials / program 37

**Brief:** ask the filter, not a heavily driven oscillator, to shape a tuned impact.
Filter FM 2.5, oscillator FM .08, stress .01, resonant Tone .5, zero amplitude
sustain and .3 s release. The audio tap is before Tone/AM/Space; velocity and dyads
affect the shared filter drive. No noise source or alternate filter type is added.

Try velocity-varied strikes and dyads against Hollow Bronze. The mechanism-control
waveform difference is comparatively small here (about −40.3 dB relative RMS),
despite a large depth number. **That is a useful open question, not proof of a new
texture**: does it add meaningful color, or does this carrier/cutoff relationship
need a more revealing study? Demo 100 BPM, last note-off 12 s, 20 s render.

### Brake Bloom / Experimental / program 38

**Brief:** more internal activity need not always mean more spectral complexity.
Network-activity-to-depth amount **−2.4**, base FM 1.35, coupling .62 / stress .22 /
Response .42. The source is the existing magnitude of network signal, not a chaos
detector. The index contribution subtracts as that signal grows, within existing
clamps. This is not a loudness limiter, new feedback topology or guaranteed pulsing.

Mono/legato with .16 s **modulator-only glide**: carrier takes the played note while
overtone relationships smear on overlaps. Nominal ratio 1 and small operator blend
keep a pitched center. Try soft versus firm notes, then overlaps and long holds;
compare Tendon Drift. Does restraint produce personality rather than simply lower
FM? Demo 100 BPM, last note-off 19.2 s, 26 s render. No playing transport required.

## Reproducibility, controls and qualifications

`make_studies.py --verify` reproduces six presets, six original MIDI files and the
manifest from explicit neutral settings/recipes; creation refuses any existing
target. All 88 condition keys are checked. No hidden settings are inherited from
the earlier patch used to check the key schema. Frozen r1 is never replaced.

Uninstalled controls remove only the study axis on identical output/MIDI:
Afterimage's delayed depth route, Lantern's operator mix, Buzz's amplitude depth,
Alloy's filter FM and Brake's negative depth route. Raw and normalized audio is
archived. Same-output waveform differences demonstrate signal change, not
perceptual novelty/preference. Negative FM routing can raise fundamental/RMS rather
than reduce loudness. Separate six reference pairs preserve old definitions/IDs;
their diagnostic MIDI does not replace original frozen performances.

### Timing limits discovered and preserved

Initial eval-015 Buzz development used a **synced** slow saw and showed a 64/257
max sample difference **2.976e−6**. Before registration, switching only the ramp
clock to free-running removes that difference in this study; final own-MIDI error
is zero. The old input/export/audio/report is preserved, not relabeled final.
This choice is a patch workaround, not a global timing fix.

An unchanged **Socket Dance r2 reference on new 120 BPM Buzz MIDI** fails the
renderer threshold: **7.56569e−4** max difference versus **1e−5** acceptance.
A discontinuous synced-wave boundary is suspected, not uniquely diagnosed.
Failed inputs/partial exports and explicit failure metadata are archived; there
is no fabricated aggregate success report or Socket WAV for that failed run.
The corrected pair uses original verified Socket **118 BPM MIDI for both unchanged
definitions**. Socket retains its original hash; Buzz then has max error 1.49e−8.
The Socket-at-120 context remains **unqualified**, separate from open F1 96 kHz
max-polyphony limits. No global DSP repair or old sound change is hidden here.

## Production evidence / eval-016, 0.7.16

Stereo 48 kHz / 24-bit exports through the production processor, preserved identity/
conditions on preset export/reimport, exact same-block repeats and **zero 64/257
max error on all six own performances**. No faults/master-ceiling hits, completed
releases and final-second RMS at the −100 dBFS reporting floor.

| Study | Raw peak (dBFS) | Active RMS (dBFS) | Preview gain (dB) | Voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Sub Canvas | −18.74 | −24.02 | +.019 | 1 | `caeeef303321dac5` |
| Afterimage Keys | −14.27 | −24.02 | +.025 | 2 | `ad63547903e113e6` |
| Overtone Lantern | −11.89 | −24.06 | +.058 | 2 | `73fc030a940f9451` |
| Buzz Loom | −16.15 | −23.98 | −.015 | 1 | `b4c059a5a0caf4ee` |
| Cutoff Alloy | −12.11 | −24.05 | +.051 | 2 | `d4379eb09184ed3f` |
| Brake Bloom | −16.49 | −24.04 | +.042 | 1 | `ce42eb856c1a9e4f` |

Output was calibrated before registration. Previews target −24 dBFS gated active
RMS, not LUFS; gain stays separate from preset values. Gate-window inclusion can
shift with absolute level. Raw controls retain each study's exact output while
normalized previews reduce louder/softer bias; neither establishes a taste verdict.

Apple M4 Max, two full demos at 48 kHz / 64: mean occupancy **1.61 / 2.20 / 2.82 /
2.46 / 1.81 / 2.81%**, p99 **42.00 / 90.67 / 111.04 / 58.13 / 100.67 / 61.13 µs**
in the table order. Zero measured overdue callbacks against **1,333.33 µs**. Includes
rests/releases and only one/two voices, not sustained max-polyphony, Logic/device
xruns or general timing qualification. F1 and the new reference-clock limit remain.

Twenty-four families now exist: eighteen retained directions plus six frontier
studies awaiting musical feedback, thirty-five review revisions and four original
factory sounds. `study_axis` is persisted, not an approval tag or artist identity.
Read `evaluation/CURRENT.md` for installed build, active trial and next action.
