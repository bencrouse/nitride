# Architecture

Nitride follows Carbide's native JUCE architecture. Milestone 1 now connects the
approved view and audible engine to the AU. `MILESTONE_01.md` describes the current
production boundary; the sections below also retain the earlier study history.

`Source/Studies` now provides a separate native sound-comparison app and a
JUCE-independent DSP library for the two candidate mechanisms. `make studies`
opens the audible app; `make render-studies` exports matched files. See
`SOUND_STUDIES.md`. Study 02 adds two mechanisms, two source profiles, and a lead
phrase; `make render-studies-02` exports those comparisons. See `SOUND_STUDIES_02.md`.
Study 03 preserves E and adds a separate note-coupled phase-delay path; its comparisons
use 30/70/100% amounts. See `SOUND_STUDIES_03.md` and `make render-studies-03`.
Study 04 compares F fixed at 50% with an adaptive oscillator network, including
fixed-link ablations. See `SOUND_STUDIES_04.md` and `make render-studies-04`.
Study 05 preserves G and adds an extended network-stress range, with checked
lower-half equivalence and independent state. See `SOUND_STUDIES_05.md`.
Control study 06 adds a separate native gesture app (`make gestures`) with independent
Coupling/Stress/Response. Its audio thread publishes fixed-size atomic network-state
snapshots for the visual surface. See `CONTROL_STUDY_06.md`.
Control study 07 is a separate target (`make deformation`) that selects a relative,
visible-object surface while sharing the gesture study's DSP and surrounding controls.
The original field target remains intact. See `CONTROL_STUDY_07.md`.
Instrument review 08 (`make review`) combines the approved field with source,
amplitude, tone, motion, space and session-local patch workflows. Its opt-in FM index
and envelope controls preserve earlier study defaults. See `INSTRUMENT_REVIEW_08.md`.
The product processor now uses the shared instrument renderer.

## Components

- **PluginProcessor:** stereo host audio/MIDI and basic condition-state hooks.
- **PluginEditor:** hosts `InstrumentView`; no audio device ownership.
- **InstrumentSession:** processor-owned numeric conditions, factory patches,
  keyboard input queue and atomic live metrics.
- **InstrumentRenderer:** shared FM/network, amplitude, motion, tone, space and gain.
- **InstrumentView:** approved native field and supporting controls, connected to a session.
- **Tests:** frozen-review equivalence, host MIDI/ownership, DSP checks and installed AU rendering.

## Parameters

The current instrument has 16 numeric conditions plus mono/poly and audition-chord
flags. The obsolete silent-shell operator/Rupture parameter layout is no longer
published. Editor writes reach the processor-owned session directly. Host automation
parameter registration is milestone 2. Basic versioned XML hooks preserve starting
conditions, not mid-note DSP state or editor-local Keep/Compare/Undo libraries.

## Drawing and interaction

The approved editor is a fixed 1200 × 840 dark/orange native surface. Source and
supporting graphs are control illustrations; network traces use live model-state
snapshots. The view polls atomic publication at 30 Hz, with a reduced-motion option.
The older pale and object studies remain reference apps.

## Future DSP boundary

The core lives in `Source/Studies/StudyEngine.*` and is linked into the shared
instrument rendering path. MIDI owners are distinguished between host and editor;
events are processed at sample offsets. The renderer owns mutable DSP and effects,
while views receive atomic metrics. Production sound-quality/performance work and
complete automation/session UX remain later milestones.

## Build and inspection

`make standalone` builds the AU and app, then opens the native editor.
`make test` runs native UI/state checks via CTest. The test executable can also
render the actual editor to a PNG without desktop capture permissions:

```sh
./build/nitride_ui_tests_artefacts/Debug/nitride_ui_tests --snapshot /absolute/path/to/ui.png
```

`make reload-au` builds, copies, and validates the component. JUCE defaults to the
existing sibling checkout at `../carbide/JUCE`; all build output is under `build/`.
