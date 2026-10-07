# Instrument study 001

This document records the original UI hypothesis. The subsequent brief clarifies
that a musical "moment" means a distinctive sound-making capability, rather than a
literal timed event. The Rupture name and excursion mechanism below are provisional.
Current research and audible experiments are in `SYNTH_LANDSCAPE.md` and
`SOUND_STUDIES.md`. Interface copy should be functional, not promotional.
The current UX direction is musical/exploratory, with gesture, temporal response and
direct feedback instead of a parameter-panel emphasis. `CONTROL_STUDY_06.md` records
the first native interaction experiment around the three independent dimensions.
The object-deformation alternative was rejected in favour of the original field.
`INSTRUMENT_REVIEW_08.md` records the current dark/orange full-instrument hierarchy,
using the initial reference's functional grouping around that approved interaction.

## Musical brief

A relatively conventional but capable FM instrument with one unexpected modern
feature. It should excel at singular moments: an impact that turns inside out, a
voice that becomes a material, a texture that briefly acquires impossible physics.
Reference users: SOPHIE, Arca, Brian Eno, FKA twigs.

The supplied UI reference sets the control-density budget: grouped synthesis,
envelope displays, tactile continuous controls, and clear signal flow. Its retro
skin is not the visual direction for this study.

## Proposed distinction: Rupture

**A reversible topology excursion.** The stable six-operator patch is home. One
gesture moves its connections and timing into an unstable temporary configuration,
then returns. This is a hypothesis to explore together, not a committed DSP spec.

- **Fracture:** split or reroute connections; sharply discontinuous timbres.
- **Bloom:** multiply feedback or modulation interactions; growing harmonic matter.
- **Dissolve:** unravel coherence and timing; suspended, porous remnants.
- **Depth:** how far the event departs from the original patch.
- **Duration:** departure-to-return time in beats.
- **Scatter:** unevenness across operators during the excursion.
- **Seed:** repeatable event variation, eventually recallable with the patch.

The appeal is repeatable, performable disruption. The same event should be useful
as an arranged accent and as a live gesture. The base patch should remain legible
and easy to program. The current trajectory view and trigger are visual only.

## Visual direction

An instrument made from mineral composite and laboratory displays. Disciplined
technical surroundings; the event section carries the expressive gesture.

| Role | Color |
| --- | --- |
| Mineral chassis | `#dadbd4` |
| Paper / raised surface | `#e7e8e1` |
| Carbon ink | `#202321` |
| Display glass | `#141b18` |
| Phosphor signal | `#b4d7be` |
| Vermilion event | `#f15d3b` |

Typography uses locally available Helvetica Neue (identity and plain-language
event copy) and Menlo / SF Mono (parameters, values, and routing). No external fonts.

```text
identity                 patch study                  reset / help
-----------------------------------------------------------------
FM network       selected operator / waveform        RUPTURE
                 tuning / level / feedback           character
routing          per-operator envelope               trajectory
operator levels                                      depth/time/scatter
--------------------------------------------------   trigger
motion           filter              space
-----------------------------------------------------------------
UI milestone / engine status                         output
```

The signature element is a fine-line instability field: a coherent line fans into
folded trajectories, then converges. Avoid constant animation. Only a triggered
visual preview moves. The editor uses native JUCE components and custom drawing,
following Carbide's approach.

## Questions for the next design session

1. Is topology excursion the right musical surprise, or should the event manipulate
   time, physical material, or a captured fragment instead?
2. Is the pale hardware surface right, and is this the right control density?
3. Should Rupture be manually triggered, MIDI-triggered, note-attached, or schedulable?
4. Is the event trajectory best edited as a shape, a gesture, or a few macros?
5. Which conventional FM features need first-class placement: arbitrary routing,
   fixed-frequency operators, alternate waveforms, pitch envelopes, velocity scaling?

Do not commit the feature set to DSP before answering these through UI iteration.
