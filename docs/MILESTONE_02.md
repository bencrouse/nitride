# Milestone 2 / automation and returnability

Nitride **0.2.0** exposes the approved instrument's sound controls to AU hosts,
recalls the complete edited sound and patch workflow, and saves named presets to
disk. The AU, Standalone and instrument review share the same session, view and
renderer.

## Use

```sh
make standalone
make install-au
make validate-au
```

The installed stereo instrument remains **nitride / nitride**, component
`aumu NT01 NTRD`, at `~/Library/Audio/Plug-Ins/Components/nitride.component`.
Restart/rescan an already-open DAW after replacing the component.

- **Keep** asks for a name and saves the edited conditions captured when Keep was
  pressed. Kept sounds appear under the starting patches and are available to new
  instances. Files live in `~/Library/Application Support/nitride/Presets`.
- **Compare** auditions the selected starting/kept reference. It does not replace
  the edited parameter bank or write reference values into host automation. Turning
  it off returns to the edits; a control edit or host automation exits Compare.
- **Undo** restores a completed control gesture or patch selection. Control undo
  changes only the parameters that gesture wrote, preserving unrelated automation.
  Grouped graphs and the Coupling/Stress field form single undo operations.
- **...** provides Redo, Import preset, Export preset and Refresh kept sounds.
  Portable presets use the versioned `.nitridepreset` JSON format; writes replace
  the target atomically and validate the sound conditions.

Closing/reopening an editor preserves the sound, name, selected preset, reference
and history. Closing during a gesture closes host automation gestures and commits
one undo operation. Restoring state or selecting a host factory program cancels
stale pointer/Response-timer state and releases editor audition notes.

## Stable host parameter contract

`Source/Instrument/HostParameters.h` defines the version-1 IDs, ordering, ranges
and normalization curves. Preserve these IDs and meanings in subsequent builds.
`PluginProcessor` owns a JUCE APVTS bank with 16 float parameters and one bool:

| ID | Range / unit |
| --- | --- |
| `coupling` | 0–1 |
| `stress` | 0–1 |
| `response` | 0.01–1.6 seconds |
| `fm_depth` | 0–4 |
| `pitch` | −12–12 semitones |
| `amp_attack` | 0.001–4 seconds |
| `amp_decay` | 0.01–4 seconds |
| `amp_sustain` | 0–1 |
| `amp_release` | 0.02–8 seconds |
| `cutoff` | 40–18,000 Hz |
| `resonance` | 0–0.85 |
| `motion_rate` | 0.05–8 Hz |
| `motion_depth` | 0–1 |
| `space_mix` | 0–0.65 |
| `space_size` | 0–1 |
| `output` | −24–0 dB |
| `mono` | boolean |

Native control writes notify the host and bracket gestures with begin/end change
notifications. Host writes update the session's atomic conditions; the native view
polls model revisions at 30 Hz. Parameter listeners do not touch the document lock,
file library or undo history. The renderer reads atomics rather than the document.
Echoes of float parameter notifications retain the original double conditions so
the approved nonlinear factory renders remain equivalent to the frozen review.

Hold, audition notes, reduced motion and the factory audition-chord flag are
interaction/session concerns rather than additional host sound parameters.

## Session format

The binary JUCE state wraps XML `NITRIDE_INSTRUMENT` version 2. Its `DOCUMENT`
contains a versioned JSON session and the XML also includes the corresponding
APVTS parameter tree. The document's edited conditions are authoritative on recall.

The document includes:

- Current conditions, patch name, selected preset ID and factory-program index.
- Compare reference, independently of the current edits.
- Up to 40 undo/redo snapshots, including parameter ownership for control edits.
- The undo baseline of a changed gesture when saving while it is still open.
- Up to 256 recently used kept presets embedded for recall without library files.

Saving during Compare recalls the edited sound with Compare off. State restore
validates the document before replacing current conditions, closes open gestures,
resets editor audition state and synchronizes the APVTS bank. Version-1 numeric
XML from milestone 1 migrates using the selected factory patch as defaults.
Malformed and unsupported session/preset data is rejected.

Sound conditions are serialized; evolving oscillator state, sounding notes and
effect tails are not. An embedded kept sound remains usable in that session without
automatically writing files into another machine's preset library.

## Verification

Verified locally on Apple Silicon/macOS with JUCE 8.0.15:

- All three CTest suites pass: native hosted/render/MIDI regressions, automation
  and state/preset workflow, and core DSP/sample-rate/stability checks.
- Automation tests cover stable IDs, balanced field/Response gestures, visible
  host-to-control updates, unrelated automation surviving Undo/Redo (also after
  recall), Compare-safe saving, active-gesture saving, persistent and embedded
  presets, import/export, editor reopening, v1 migration and invalid-state rejection.
- Restore tests exercise active field, Response, source, amplitude and tone drags;
  Response timers and host program selection cannot overwrite the recalled patch.
- The shared review app's interaction check passes. Native playback and release
  pass with zero xruns and numerical faults on the local device.
- Apple `auval -v aumu NT01 NTRD` passes and reports **0.2.0 (0x200)**.
- The installed AudioToolbox component publishes 17 parameters, retains host
  writes through ClassInfo recall, renders real MusicDevice MIDI at sample offsets,
  and releases the note/tail.

```sh
make test
"./build/nitride_review_artefacts/Debug/Nitride Instrument Review.app/Contents/MacOS/Nitride Instrument Review" --interaction-check
"./build/nitride_review_artefacts/Debug/Nitride Instrument Review.app/Contents/MacOS/Nitride Instrument Review" --audio-check
make install-au
make validate-au
./build/nitride_au_component_tests
```

The installed-component test requires installation and is separate from CTest.
Full Logic project save/reopen and recorded-automation playback remain a manual
host acceptance check. This milestone establishes the native/API workflow;
production sound-quality and performance refinement remain subsequent work.
