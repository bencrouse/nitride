# Batch 08 / r1 — OPN / Arca / Body Meat lenses

The user selected **Oneohtrix Point Never / Arca / Body Meat**, rejecting the
earlier proposed trio. Producer-led imagined taste briefs are preferred for
uniqueness/personality/interest; category recipes are secondary usability checks.
These are original assistant designs, not actual artist settings, quotations,
private intent or endorsements. All fifteen earlier retained directions stay intact.

## Replica Voice / Leads — Oneohtrix Point Never lens

**Imagined brief:** a recognizable synthetic phrase whose imagined mouth does
not quite belong to it. Keep a playable melody; avoid an ordinary gliding lead,
random pitch jumps or a generic slowly evolving pad.

- Poly/retriggered phrases and real dyads; 80 ms attack and .9 s release, no pitch
  attack or glide. The chord helper is off.
- Resonant Tone has zero key tracking, negative envelope amount and slightly
  negative velocity sensitivity: its center does not simply follow every key.
  It is a mouth-like **impression**, not a multi-formant bank or sampled voice.
- Nominal 2:1 lightly triangle-shaped FM with a negative index envelope, envelope-
  driven operator blend and gentle free ratio drift. These shape the internal
  gesture rather than adding the same stressed distortion to a new category.
- Contrast Glass Current's mono/gliding voice. Whether this reads as uncanny or
  only a resonant synth remains a listening question, not established by its name.

Try mid/high-register detached phrases, held notes and actual dyads. The original
88 BPM demo ends its last note at about 18.41 s in a 26 s render. Live free-lane
phase depends on previous playback; transport need not play for its FM drift.

## Tendon Drift / Experimental — Arca lens

**Imagined brief:** a supple melody that resists the hand, with intermittent strain
rather than maximal grit all the time. Preserve bodily interaction, not a fixed
good/bad score or a prettier version of Fault Bloom.

- Mono/legato, 1.1 s **linear Pitch + Tone glide** on overlapping notes. Carrier,
  modulators and existing Tone pitch routing follow the dragged note together.
  Separated phrases retrigger the soft 35 ms amplitude entrance; no Autobend.
- Moderate .58 coupling / .16 stress / .32 Response; a free .29 Hz sample-and-hold
  stress lane interrupts slower continuous coupling/filter motion. These are
  deterministic shared lanes with existing slew, not independently randomized voices.
- Nominal 1.7:1 triangle FM; a slow index envelope, filter-to-ratio pull and network-
  to-shape route provide changing tension. A modest FM-source-to-mix route moves
  the exposed operator without making amplitude chopping the defining behavior.
- Contrast Fault Bloom's sub-unity saturated FM and opposed glide, rather than
  merely increasing or decreasing its distortion amount. Musical difference is pending.

Hold one note, then overlap near/far notes and let the drag complete; compare with
separated short notes and a new phrase. The original 96 BPM demo ends at 20 s in
a 30 s render. Free motion needs no playing transport. Unconventional pitch/texture
is intentional; numerical safety is separate from a conventional quality verdict.

## Socket Dance / Sequence elements — Body Meat lens

**Imagined brief:** a small dry cell that feels like several articulated pieces
working against one another while remaining danceable. Avoid blurred tails or a
busy MIDI arrangement being the entire source of personality.

- Poly/retriggered, no glide/Autobend, 2 ms attack and .1 s release; Space mix zero.
  .62 sustain lets a held note expose real internal audio movement.
- Deep **sixteenth-triplet square amplitude** chops (.92 depth), **straight-eighth
  S&H cutoff** and **dotted-eighth triangle FM** use different divisions of one
  host clock. All notes share these lanes; no independent per-note rhythms are added.
- Nominal 2:1 sine FM with a negative index envelope gives a changing strike/body.
  Filter and FM sources move operator mix; network activity contributes index motion.
- Contrast Undertow's sustained mono/gliding sixteenth-triangle bed. This is not
  sampled vocal chopping, an internal MIDI sequencer, swing engine or a track replica.

**Play transport and hold notes/dyads**, then add stabs/rests. MIDI supplies pitches
and arrangement, not each pulse. The original 118 BPM demo's sparse note events
cannot account for all its audio chops; last note-off is about 14.75 s in a 22 s
render. Judge both danceability and changing articulation, not only faster tremolo.

Public context for this lens: [Christopher Taylor (Body Meat), The Creative
Independent, interview by Jessica Kasiama, 2024-09-30](https://thecreativeindependent.com/people/musician-christopher-taylor-body-meat-on-relinquishing-creative-control/).
His discussion of rhythmic progression, sculpting and separately seeming sounds
within a shared flow informed broad context. The brief/settings above are our own
interpretation, not a claim that this implements his software or private process.

## Frozen and common-note evidence

`make_midi.py --verify` reproduces all original SMF bytes and refuses overwrites.
Files use the existing standard-library encoder with one fixed tempo and 4/4 each.
Future revisions should preserve these performances for voicing comparisons.

Separate diagnostics compare unchanged Glass/Replica, Fault/Tendon and Undertow/
Socket definitions on identical new MIDI per pair. Original IDs/conditions and
older frozen performances remain unchanged. Active-RMS preview normalization is
separate from preset values. Labels, different clock settings or voice counts do
not prove artistic authenticity, uniqueness or sufficient musical contrast.

## Production evidence / eval-014, 0.7.14

Stereo 48 kHz / 24-bit production-processor exports; preset export/reimport
preserves identity and all conditions. Same-block repeats are exact. The 64/257-
frame maximum errors are **0 / 0 / 2.91e−11** for Replica / Tendon / Socket; the
Socket difference is nonzero but far below a 24-bit PCM step. No numerical faults
or master-ceiling hits, completed amplitude releases and final-second RMS at the
−100 dBFS reporting floor. Soft coloration still exists despite zero hard-clamp hits.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Replica Voice r1 | −12.56 | −24.005 | +.005 | 3 | `7d033a1cdfecf542` |
| Tendon Drift r1 | −15.59 | −24.007 | +.007 | 1 | `7699187b22eabd16` |
| Socket Dance r1 | −11.08 | −23.956 | −.044 | 2 | `4e535341bc5e1e95` |

Previews target −24 dBFS gated active RMS, not LUFS. Only output was calibrated
before freezing: Replica −4 → −3.35 dB; Tendon −4 → −3.15 dB; Socket −5 → −3 dB.
The eval-013 development capture is preserved separately, not relabeled as final
eval-014 or treated as a musical improvement/approval. No drive/global range change.

Apple M4 Max, two full demos at 48 kHz / 64: mean occupancy **4.07 / 3.18 / 2.57%**,
p99 **119.71 / 64.13 / 106.67 µs** for Replica / Tendon / Socket. Zero measured
callbacks beyond the **1,333.33 µs** deadline. Demos include rests/releases with
only three/one/two voices; not max-polyphony, Logic/device-xrun or universal
real-time qualification. F1 96 kHz workload limits remain open.

Programs 28/29/30 append under Leads / Experimental / Sequence elements. Total:
eighteen families, twenty-seven review revisions, four original factory sounds;
not final library approval. Host contract, DSP signal flow, progressive controls,
network/Response/Space, global curves and voice/quality policy remain unchanged.
Read `evaluation/CURRENT.md` for actual installed build, trial and next action.
