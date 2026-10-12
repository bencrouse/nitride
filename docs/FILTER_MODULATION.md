# Filter modulation / Nitride 0.6.0

The schema and measurements below record the 0.6.0 milestone. For the current
FM extensions and 0.7.0 schema, see `FM.md`.

Tone adds **Key tracking**, **Velocity sensitivity** and **Filter FM depth** to the
existing shared stereo filter. The dedicated filter ADSR and its note gate are
reused unchanged; this feature does not introduce another envelope or per-voice
filters. The native panel stays 1200 × 960, with the new controls beside the Tone
cutoff/resonance plot.

## Controls

| Control | Range / default | Meaning |
| --- | --- | --- |
| Key track | 0–100% / 0% | MIDI note tracking around note 60; 100% is one cutoff octave per MIDI octave |
| Velocity | −6 to +6 octaves / 0 | Signed octave offset multiplied by note-on velocity |
| Filter FM | 0–6 octaves / 0 | Maximum bipolar audio-rate cutoff offset at full normalized oscillator amplitude |

At a 1 kHz base cutoff and 100% tracking, MIDI notes 48, 60 and 72 produce 500 Hz,
1 kHz and 2 kHz before other modulation. At 50%, an octave of MIDI pitch produces
half an octave of cutoff movement. Tracking uses MIDI note number, not pitch bend,
Glide or vibrato; existing **Pitch + Tone** remains independently additive.

Positive velocity depth opens the cutoff more for harder notes. Negative depth
closes it. A depth of +2 octaves contributes one octave at velocity 0.5 and two
octaves at velocity 1. MIDI velocities are quantized to 1–127; velocity zero is a
note-off rather than a new filter owner.

Start with **Glass keys**, lower Tone cutoff to approximately 1 kHz, turn Key track
to 100%, then play across octaves. Add Velocity at +2 octaves for touch response.
Raise Filter FM to hear oscillator-rate roughness; the existing filter envelope
and cutoff LFO can shape that motion at the same time.

## Shared note ownership

The filter follows the most recent physically held note across host/editor owners.
When no physical keys remain, pedal-held owners provide the most recent note.
Hold captures retain the original press order and velocity: capturing an older
note does not promote it above a more recent held note or substitute the editor's
audition velocity. Releasing the selected note returns to another held owner;
with no owners, retain the last pitch and velocity throughout release.

This applies in both poly and mono/legato modes. Partial chord releases, repeated
notes, pedal-up, channel panic, editor teardown and overflow recovery use the
existing ownership bookkeeping. Reset/reprepare clears the note source to MIDI 60,
velocity zero. Filter note selection is cached; normal unrelated polyphonic note
events do not scan the entire owner array. Returning to another owner performs a
bounded scan on the audio thread, not on every sample.

## Cutoff composition and audio-rate FM

All control sources are additive in logarithmic frequency space:

```text
controlOctaves =
    keyTracking * (MIDI note - 60) / 12
  + velocityDepth * velocity
  + filterEnvelopeAmount * existingFilterEnvelopeLevel
  + cutoffLfoOctaves
  + pitchToneSemitones / 12

cutoffHz = clamp(baseCutoffHz * 2^(controlOctaves + fmDepth * oscillator),
                 10 Hz, 0.42 * sampleRate)
```

The base cutoff uses a **25 ms multiplicative ramp**, equivalent to linear movement
in log frequency. Note/velocity offsets ramp over **25 ms in octave space**.
Filter envelope Amount retains its existing smoothing and the LFO retains its
own waveform/depth behavior. FM depth automation ramps over 25 ms, but the
**oscillator signal and final cutoff are not smoothed**. Each audio sample's FM
contribution goes directly into that sample's filter coefficients.

The FM source is the pre-filter synthesized oscillator mix, including voice FM,
network synthesis and voice amplitude envelopes/velocities. The engine produces a
bipolar soft-limited ±0.8 mix. Tone takes its stereo mean and divides by 0.8,
clamping the normalized source to ±1. There is no automatic gain normalization,
extra FM oscillator or feedback from the filtered output. Tremolo, Space and
master output gain are downstream and do not affect this tap.

Cutoff bounds apply **after all sources**, before the existing state-variable
filter's coefficient calculation. For validation-only sample rates below about
24 Hz, the lower bound also falls to 0.42 × sample rate to keep the interval valid.
No rendered modulation is written into stored base cutoff or parameter readback.

## State and local verification

The greenfield schema is preset **v5**, XML/session **v6**, with **71 host controls**.
Old schemas are unsupported, without migration. Existing files are not deleted.
Keep, Compare, scoped Undo/Redo, automation and portable preset/session recall all
include the new values, including host indices above 63.

- `Source/DSP/FilterModulation.h`: note offsets, octave composition and bounded
  instantaneous FM cutoff conversion; no envelope implementation.
- `InstrumentRenderer.*`: note ownership, log-space ramps, pre-filter audio tap
  and integration with the existing ADSR/LFO/Pitch + Tone path.
- `InstrumentPanel.cpp`: accessible native Tone controls and generation-safe edits.
- `FilterModulationTests.cpp`: octave ratios, velocity scaling/polarity, combined
  control math, FM sample response, owner return/pedal/Hold/reset/overflow, base
  smoothing, block timing, high-resonance extremes and state/UI/history workflow.

The audio-rate integration test runs a separately instantiated production engine
and checks the renderer's actual cutoff against its instantaneous oscillator
sample at 44.1/48/96 kHz. Tremolo, Space and master output are enabled to verify
that none leaks into the FM tap. Existing filter envelope tests continue running.
The four neutral factory fingerprints remain exact; this is a neutrality check,
not a legacy state compatibility requirement.

```sh
make check
make check-full
build/local-release/nitride_perf_artefacts/Release/nitride_perf \
  --filter-modulation --output build/checks/filter-modulation-enabled.json
```

The benchmark matrix pairs tracking, velocity, one-voice FM, all three controls at
four/twelve voices, and twelve-voice composition with ADSR, six LFO lanes and
Pitch + Tone. Ordinary before/after captures measure inactive overhead separately.
Timing runs must not overlap builds or native playback; timing is quantified, not
a CTest pass/fail claim. The safety harness exercises active FM with concurrent
host writes, recall, keyboard input and editor lifecycle changes.

### Measured results

Measured locally on **2026-10-11**, Apple M4 Max, Clang 21.0.0, macOS 27.0.1,
pinned JUCE 8.0.15, Release: **three passes, two audio seconds per case/pass,
0.5-second warmup**, 48/96 kHz and 64/256-frame blocks. Final timing runs had no
concurrent builds or native playback. Figures are median pass audio-budget
occupancy, not total system CPU percentage.

| Paired workload | Rate / block | New controls off | New controls on |
| --- | --- | ---: | ---: |
| Key tracking, four held notes/retrigger | 48 kHz / 64 | 36.98% | 37.03% |
| Velocity, four held notes/retrigger | 48 kHz / 64 | 36.99% | 37.08% |
| Filter FM, one voice | 48 kHz / 64 | 3.88% | 3.90% |
| All three, four held notes/retrigger | 48 kHz / 64 | 37.01% | 37.04% |
| All three, twelve-voice stress | 48 kHz / 64 | 48.91% | 48.88% |
| ADSR + six LFOs + Pitch + Tone + new controls | 48 kHz / 64 | 47.47% | 47.45% |
| Filter FM, one voice | 96 kHz / 64 | 7.57% | 7.59% |
| All three, twelve-voice stress | 96 kHz / 64 | 95.11% | 95.18% |
| Full composition, twelve voices | 96 kHz / 64 | 97.05% | 97.13% |
| Full composition, twelve voices | 96 kHz / 256 | 97.02% | 96.96% |

Retrigger workloads vary velocities through 32/64/96/127, with identical MIDI in
each on/off pair. Four-held-note retriggers accumulated **5–12 active voices**
including release tails; they are not steady four-voice measurements. All
**24 on/off pairs** produced different audio fingerprints. Small positive or
negative timing differences include variation and operating-point changes, so
they do not establish an isolated DSP optimization.

The ordinary before/after matrix retained **all 32 exact fingerprints** with the
new controls neutral. Median render-time increases ranged from **0.16–1.74%** in
this run; environmental variation is included. Idle's 64-frame median stayed
5.83 microseconds. Both final matrices had **zero overdue blocks at 48 kHz**.

Maximum polyphony at 96 kHz remains constrained. Full composition recorded
**1,836/9,000** overdue blocks at 64 frames, p99 **700.04 microseconds** against a
666.67-microsecond deadline, and **373/2,250** overdue blocks at 256 frames. The
corresponding off cases already recorded 1,848 and 394 misses: the combined
existing synthesis workload is marginal even before enabling these controls.
The new feature is not a solution to the existing headroom limit.

Six Debug and six Release suites passed, as did focused ASan/TSan safety checks
and installed-AU validation. Audited active callbacks allocated/deallocated zero
memory; existing framework callback limitations remain documented in
`HARDENING.md`. The installed AU verified all 71 control names/IDs, new-control
state recall and MIDI/audio integration. All **twelve native interaction checks**
passed. Built-in-speaker playback with the existing ADSR, six LFO lanes and all
three new controls passed at peak 0.0588, about 12.77% device CPU, zero numerical
faults/xruns and completed release. The first app-local device-switch run recorded
one xrun; the repeat run recorded none.

Artifacts: `build/checks/filter-modulation-before.json`,
`filter-modulation-after.json`, `filter-modulation-enabled.json`,
`filter-modulation-ui.png`, profile logs and installed-AU logs.
