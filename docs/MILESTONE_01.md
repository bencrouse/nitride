# Milestone 1 / first playable AU

The approved dark/orange native instrument view and audible engine are connected
to the actual AU processor. The AU is no longer the silent original UI shell.

## Use

The local component is installed at:

```text
~/Library/Audio/Plug-Ins/Components/nitride.component
```

In a DAW, select the stereo AU instrument **nitride / nitride**. If the host was
already open when the component was installed, restart/rescan it. MIDI notes,
velocity, pitch wheel and sustain drive the processor. The editor field, Response,
source, envelope, tone, motion, space and output all affect the same hosted sound.

```sh
make standalone   # Product processor/editor through JUCE's Standalone wrapper
make review       # The review app using the same shared view and rendering path
make install-au
make validate-au
```

The earlier gesture/sound studies remain available as reference applications.

## Shared boundaries

- `InstrumentSession`: processor-owned numeric sound conditions, factory starting
  patches, editor note input, and fixed-size live-state publication.
- `InstrumentRenderer`: the approved FM/network, envelope, LFO, tone, reverb and
  output path. It has no audio device or UI component.
- `InstrumentView`: the approved native view connected to the session. It has no
  audio device manager and does not render audio.
- `PluginProcessor`: receives the host's sample rate, buffers and MIDI and calls
  the shared renderer.
- `PluginEditor`: hosts the shared view at its approved 1200 × 840 size.
- `ReviewStandalone`: supplies a local device/MIDI collector to the same renderer
  and view for comparison and native playback checks.

Host and editor MIDI have separate ownership domains. Releasing an editor key or
closing the view does not release a still-held host note on the same pitch/channel.
The keyboard view uses a separate editor channel from field/Hold audition. MIDI
events are merged at their sample offsets; large SysEx is ignored by this instrument.
Sustain and channel note-off handling are included in the renderer.

## Preservation and verification

The frozen pre-integration review rendering is compiled into a regression fixture
without an audio device. All four starting patches are compared against hosted
processor rendering, using the same quantized MIDI velocity and performance. The
comparison includes source, envelope, motion, filter, space and gain, not just the
raw oscillator output.

Checks cover:

- Review/hosted sound equivalence for all four patches.
- Host MIDI sample offset and audible output.
- Sustain, note release and no MIDI passthrough.
- Editor changes reaching the processor.
- Hold capture without doubling a host note.
- Closing editors without stuck audition notes or ending held host notes.
- Basic numeric-condition state hooks.
- Existing voice mappings, sample rates, block-size and stability checks.
- Correct pitch at low host rates (the engine now uses the actual host clock).

Apple's `auval -v aumu NT01 NTRD` passes. `nitride_au_component_tests` loads the
installed component through AudioToolbox and exercises real MusicDevice MIDI and
stereo AudioUnit rendering, including offset timing and full note/tail release.

```sh
make test
./build/nitride_au_component_tests   # Requires the component to be installed
```

## Milestone boundary

This completes the first playable hosted instrument. The obsolete silent-shell
operator/Rupture parameters are not exposed as misleading controls. Host automation
parameter registration and complete preset/session UX belong to milestone 2.

The existing processor state hooks preserve basic sound conditions and the starting
program index, but Keep/Compare/Undo libraries remain editor-local. These hooks are
not a claim that the complete saved-session/preset workflow has been finished or
tested in Logic. Mid-note evolving DSP state is not serialized.
