# Instrument review 08 / full dark-orange surface

This is the next overall instrument mockup, combining the original reference's
clear module grouping with the approved playable field and Response gesture.
It is a separate native target; `make gestures` still opens the preserved field.

![Native instrument review](instrument-review-08.png)

## Open

```sh
make review
```

Opens **Nitride Instrument Review**. Use Hold, MIDI or the keyboard to hear changes;
touching the field or Response ribbon also plays the audition voice.

## Hierarchy

```text
nitride       patch                        undo / compare / keep / hold
----------------------------------------------------------------------
SOUND                  NETWORK / PLAYABLE FIELD             AMPLITUDE
source preview         Coupling + Stress                    envelope
FM depth / pitch       Response time gesture                ADSR values
poly / mono
----------------------------------------------------------------------
TONE                   MOTION                               SPACE
cutoff / resonance     rate / depth → Coupling               mix / size
----------------------------------------------------------------------
keyboard                                             voices / output
```

The signature remains the primary interaction. Supporting controls use editable
graphs and compact drag values. There is no operator-routing matrix on the main
surface and no marketing copy or timed-event framing.

## Visual direction

| Role | Colour |
| --- | --- |
| Carbon chassis | `#151817` |
| Module surface | `#1b201d` |
| Raised controls | `#252b27` |
| Display glass | `#0d100f` |
| Ivory signal/text | `#e5dfd2` / `#d9d8ca` |
| Orange interaction | `#f87946` |

Typography remains local Helvetica Neue and Menlo. The black/orange direction is
applied to this review target only; the approved mineral gesture study is retained.
Structural dividers carry the module hierarchy. The central field is the existing
native PlayingField, not the rejected object-deformation surface.

## What works in this preview

- **Sound:** editable FM index 0–4, pitch offset ±12 semitones, and poly/mono audition.
  Drag numeric values vertically; arrows fine-tune and double-click restores a default.
- **Amplitude:** drag the attack point, decay/sustain point, or release point, or edit
  their corresponding values. These change the loudness envelope, separate from Response.
- **Tone:** drag cutoff/resonance on the 12 dB low-pass preview.
- **Motion:** drag sine rate/depth; the destination is Coupling.
- **Space:** drag diffusion/reverb mix and size. This is an audible preview effect.
- **Patch:** four starting conditions: Soft pad, Glass keys, Contact lead, Hard hit.
- **Keep:** appends the current parameter conditions as a patch for this review session.
- **Compare:** toggles between the edited state and the last starting/kept state.
- **Undo:** returns to previous control conditions; field drags are grouped as gestures.
- Hold, MIDI, keyboard capture/release, output trim and reduced motion remain available.

Source/filter/envelope plots are control illustrations, not analyzers. The central
network traces continue to use live model-state feedback. Keep is session-local;
host preset files and persistent saved sessions belong to the AU integration milestone.

The sound engine adds opt-in FM-index and amplitude-envelope controls, leaving earlier
study defaults intact. The source reference uses a precomputed coefficient table with
smoothed index changes. Tone, motion and space are preview implementations for assessing
the complete workflow, not a claim that all final DSP/host integration is finished.

## What to review

1. Does the grouping make it easy to begin, shape, explore and keep a sound?
2. Is the field still the centre of the instrument?
3. Are amplitude articulation and network Response clearly distinct?
4. What is too exposed, too hidden, or missing for ordinary musical jobs?
5. Does the dark/orange direction and control density feel right?

## Verification and files

Source/envelope DSP checks cover audible changes and correct configurable release,
in addition to the existing mapping, sample-rate, block-size and stability checks.
Native review checks exercise field edits, undo, source/envelope, supporting surfaces,
keep/compare and play/release. Native 48 kHz playback completed with no xruns or
numerical faults. The original gesture interaction checks also pass.

`Source/Studies/InstrumentReview.inc` is the private view implementation included by
the shared gesture application translation unit. `NITRIDE_INSTRUMENT_REVIEW` selects
its layout and palette for the `nitride_review` target. A reusable AU editor/view
boundary will be extracted once the overall UX is agreed.
