# Batch 12 / r1 — playable FX and transitions

Bowl Toms r2 is now explicitly preferred and direction-kept, with both kicks and
all 31 retained families unchanged. The user asks to move to FX/transitions,
required for the initial release. Three original temporal jobs follow, not pads
renamed FX or a checklist of every unused control. Taste lenses reuse the approved
producer-imagination workflow, **not authentic artist recipes or endorsements**.
Musical usefulness/distinctness is pending, not established by names or measurements.

## Pressure Lift / Experimental / program 48 (zero-based)

**Brief:** a tactile synthetic pressure build, imagined through the earlier SOPHIE
lens. A single note rises from two octaves below while amplitude, FM index and Tone
open, with more modulator shaping as the entrance develops. Release it into the
next section; it does not automatically trigger another sound or drop itself.

Start at **MIDI note 48**, velocity around 96, **hold about four seconds**, then
release. Try 43/55 for different register/weight. Amplitude attack 2.6 s; FM/Tone
attacks 3.2 s; −24-semitone Autobend over 4 s to carrier/modulators. Mono/retriggered,
no glide or cyclic lanes, moderate network, nominal inharmonic ratio 1.414. Short
.12 s amp release with small .12 Space: not guaranteed instantaneous silence.
**No transport requirement.** A short tap deliberately gives only a quiet partial
gesture. Does it create a usable approach to a downbeat, rather than merely a pad?

## Sinkhole / Experimental / program 49

**Brief:** an immediate metallic implosion falling into low weight and room,
imagined through the earlier Arca lens. Different from a bass drum: a much longer
pitch movement, more complex initial spectrum and a spacious decay.

Start at **MIDI note 43**, velocity around 100, with a **one- to two-second gate**;
let the tail clear before retriggering. Try 48 for a lighter landing. 2 ms amp
entrance, zero sustain/.8 s exponential decay/.7 s release; +24-semitone Autobend
over 1.2 s; Pitch −7 gives nominal settled carrier at MIDI36/65.4 Hz for input43.
FM index strike relaxes through .18 s decay at nominal ratio 2.71; moderate network,
some filter FM, .3 Space/.8 size. Mono/no glide or cyclic lanes.
**No transport requirement.** Does the falling impact/tail mark a section boundary,
without merely becoming another kick, bass note or shared gritty material hit?

## Shred Relay / Sequence elements / program 50

**Brief:** a signal breaking into a steady rhythmic grid, imagined through the
earlier Oneohtrix Point Never lens. A rounded held note grows less harmonically
stable as index and ratio movement build while Tone closes. Deep sine-AM supplies
the clock; the phrase should change over time rather than be a static pulse preset.

Start at **MIDI note 48**, velocity around 96, **hold three to four seconds with
transport playing**, then release. Try 43/55. Dry mono/retriggered, .075 s amplitude
release; FM attack 2.4 s, Tone attack 2.8 s; filter envelope also raises the FM ratio.
Only AM is clocked: **fixed 1/16 sine**, depth .99, eight cycles/s at 120 BPM.
This is **not an accelerating stutter**, hard square gate, internal MIDI generator,
note-on-reset clock or independently randomized grain engine.

Stopped host PPQ freezes the synced lane at that position; at some positions the
deep AM can make the held note very quiet. Without host position, it advances at
the fallback tempo (stored 120 BPM). Playing phase follows host position/tempo,
not when the note began. Does the evolving breakup earn a transition role beyond
the unchanged Socket/Buzz rhythmic cells?

## Timing and architectural boundaries

**All envelopes and Autobend use seconds, not bar lengths or tempo-following macros.**
Four seconds equals two bars only at 120 BPM in 4/4. Change trigger/gate timing
yourself at other tempos; the preset does not schedule a next-section handoff.
Shred's clock follows tempo, but its color envelope does not stretch with the bar.
Held gestures continue at their settled state; note-off still controls release.

These are generated instrument sounds, not inserts that process incoming audio.
No dedicated noise generator, sampled/reversed audio, mapped mod-wheel/MPE,
granular/delay effect, one-shot/phase-reset switch, new topology or host parameter
is added. Space remains the existing reverb/diffusion with dry contribution. Try
single notes first; mono replacement and shared Space history affect retriggers.
Octave labels differ by host; the note recommendations above are MIDI numbers.

## Frozen demos and comparisons

`make_transitions.py --verify` reproduces three complete presets, three original
fixed-tempo 4/4 MIDI performances and the manifest; all 88 condition keys checked.
Creation refuses existing paths. The original neutral recipe/SMF encoder comes
from an earlier frozen generator, not a hidden inherited artist patch.

All demos are 120 BPM. Pressure/Shred have two four-second holds in different
registers, a deliberately truncated one-second gesture and another four-second
hold; final note-off at 20 s, rendered 28/26 s respectively. Sink compares 1 s,
2 s and .5 s gates, then a short tap; final off about 19.08 s in a 32 s render.

Uninstalled evidence under `evaluation/diagnostics/batch-12-transitions/`:

- **8 control captures:** study versus only Autobend disabled for Pressure/Sink;
  Sink also dry; Shred versus AM disabled or a static-color control. Static-color
  removes **three** evolving color contributions (FM envelope amount, filter-to-ratio
  route amount, Tone amount), not a single-knob isolation. Outputs/MIDI unchanged.
- **8 references:** Pressure/Tendon and Sink/Brake on new study notes; Shred/Socket
  on original verified **118 BPM Socket MIDI**, explicitly avoiding the separately
  unqualified new-120-BPM context; Shred/Buzz on new Shred 120 BPM MIDI. Old
  definitions/IDs/levels/frozen performances are never replaced. Different mono/poly
  and envelope responses are intentional comparison variables, not equalized away.
- **11 playing captures:** fresh four-second hold versus early gate, retrigger after
  an early release, plus Shred eight-beat holds at 90/150 BPM. Pressure early gate
  is .5 s, not an effectively silent 25 ms tap; the other early gates are 25 ms.

All 27 captures repeat exactly. Controls and own120BPM playing captures have zero
64/257 error; Shred at150BPM also zero. Shred-at90BPM maximum **3.73e−9**,
Shred/Socket118 pair **1.86e−9 / 2.91e−11**; all below the renderer's1e−5 acceptance.
No faults/master-ceiling hits; amplitude releases finish. This does not repair
the known Socket-on-new-120 case or imply universal discontinuous-wave timing.

Focused native tests check Shred playing/stopped phase at90/150BPM/nonzero PPQ,
missing-position fallback, and exact first-quarter-second Pressure/Sink audio
independence from stopped90 versus playing150 transport. These are specific
contexts, **not** a new tempo-change/seek/export/automation qualification.

## Raw held-note shape and release evidence

Fresh four-second gates at equal stored output; compare **raw/live** levels rather
than independently normalized previews. Analysis measures stereo RMS in the first
and last .5 s, and 10 ms post-note-off windows above−60dBFS:

| Study | Late−early RMS | Last post-off window above−60dBFS | RMS .5–1s after off |
| --- | ---: | ---: | ---: |
| Pressure | +17.47 dB | .34 s | −73.96 dBFS |
| Sinkhole | −39.04 dB | none | −77.56 dBFS |
| Shred | −1.02 dB | .08 s | analysis floor |

Sink's held impact has mostly decayed **before** the four-second note-off; this
table is not its entire tail duration. Shred's similar early/late level does not
prove timbral evolution or musical success. All last-second values reach the raw
analysis floor (−200dBFS); the production reporter separately uses−100dBFS.
Early/held prefixes match exactly for24,000/1,200/1,200frames, then gate-dependent
behavior differs. Early Pressure preview gets **+15.02dB** normalization: auditioning
that as a supposedly full-strength riser would be misleading.

## Production evidence / 0.7.20, eval-020

48 kHz stereo24-bit production processor exports; identity/conditions survive
export/reimport, exact same-block repeats and **zero64/257 maximum difference**
for all three own demos. No numerical faults/master-ceiling hits; completed
amplitude releases and final-second RMS at−100dBFS reporting floor.

| Study | Raw peak dBFS | Active RMS dBFS | Preview gain dB | Voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Pressure Lift | −15.79 | −24.00 | +.004 | 1 | `545b041ed9da11cf` |
| Sinkhole | −12.93 | −24.51 | +.513 | 1 | `33727661e6ee24be` |
| Shred Relay | −14.19 | −23.99 | −.008 | 1 | `fc178bdb1a355109` |

Only output was calibrated before freezing: initial−6dB becomes−4.7/−1.1/−2dB.
Pre-registration eval-019 captures are archived separately. Gated-window shifts
mean measured RMS changes need not equal gain deltas. Preview target−24dBFS gated
active RMS is not LUFS/perceptual matching and does not alter stored output.

Apple M4 Max, two full own-demo passes at48kHz/64: mean occupancy **2.18/1.23/2.26%**,
p99 **57.17/56.08/55.88µs**, zero measured overdue callbacks against1,333.33µs.
One voice plus rests/tails, not max-polyphony/device-xrun/cross-rate or renewed full
sanitizer qualification. F1 combined96kHz and Socket-on-new120 reference limits
remain open; no quality reduction/cap or global DSP repair.

Read `evaluation/CURRENT.md` for installed build/trial. There are now **34 families**:
31 kept directions plus three transition auditions, not final factory approval;
47 review revisions/four original factory programs. No commits/pushes.
