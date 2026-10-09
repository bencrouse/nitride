# Architecture

Nitride follows Carbide's native JUCE architecture. The approved view and audible
engine are connected to the AU, with host automation and persistent returnability.
`MILESTONE_02.md` describes the current boundary; `MILESTONE_01.md` and the sections
below retain the initial integration and earlier study history.

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
amplitude, tone, motion, space and patch workflows. Its opt-in FM index
and envelope controls preserve earlier study defaults. See `INSTRUMENT_REVIEW_08.md`.
The review and product processor now use the shared instrument session, renderer
and view, including the milestone-2 preset workflow.

## Components

- **PluginProcessor:** stereo host audio/MIDI, APVTS parameters and versioned state hooks.
- **PluginEditor:** hosts `InstrumentView`; no audio device ownership.
- **InstrumentSession:** processor-owned atomic conditions, factory/user patches,
  Compare audition bank, parameter-scoped control history, session document,
  keyboard input queue and atomic live metrics.
- **PresetStore:** validated versioned JSON presets, library loading and atomic file writes.
- **InstrumentRenderer:** shared FM/network, amplitude, motion, tone, space and gain.
- **InstrumentView:** approved native field and supporting controls, connected to a session.
- **Tests:** frozen-review equivalence, host MIDI/ownership, automation/gestures,
  session/preset recall, DSP checks and installed AU state/rendering.

## Parameters

The current instrument publishes 16 float sound parameters and a mono boolean,
defined in `HostParameters.h`. Editor writes update the processor-owned session and
notify the host with balanced gestures; host listeners update atomic conditions
without entering the document lock or preset store. The view polls model revisions.
The audition-chord flag is session metadata. The obsolete silent-shell
operator/Rupture parameter layout is no longer published.

Compare reads a separate reference bank without replacing edited parameters or host
automation. Version-2 XML/JSON state preserves edited conditions, selected-patch
metadata, reference, undo/redo and portable kept sounds. Control-history snapshots
identify their edited parameters so Undo preserves unrelated automation. Version-1
numeric XML migrates; mid-note DSP state is not serialized. `MILESTONE_02.md`
describes the format and preset library.

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
manual DAW acceptance remain subsequent work.

Performance refinement now has a separate Release harness linked to the actual
processor code. It times full MIDI/automation rendering, reports audio-budget
occupancy and block-time percentiles, and compares deterministic audio fingerprints.
`PERFORMANCE.md` records baseline/candidate measurements and the first exact DSP
caching optimizations. This is a measurement foundation for milestone 3.

## Build and inspection

`make standalone` builds the AU and app, then opens the native editor.
`make test` runs native UI/state checks via CTest. The test executable can also
render the actual editor to a PNG without desktop capture permissions:

```sh
./build/nitride_ui_tests_artefacts/Debug/nitride_ui_tests --snapshot /absolute/path/to/ui.png
```

`make reload-au` builds, copies, and validates the component. JUCE defaults to the
existing sibling checkout at `../carbide/JUCE`; all build output is under `build/`.
