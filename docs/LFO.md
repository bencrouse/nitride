# Motion / Nitride 0.5.0

The measurements and schema below record the 0.5.0 milestone. For the current
filter extensions and 0.6.0 state schema, see `FILTER_MODULATION.md`.

Motion is a bank of **six independent global LFO lanes**: Pitch, FM depth, Filter
cutoff, Amplitude, Coupling and Stress. Multiple lanes can run at once. The native
panel remains 1200 × 960; the new controls replace the old Motion XY pad without
moving the sound, envelope, Tone or Space panels.

## Play it

1. Choose **Glass keys** and hold a note.
2. In Motion, choose **Filter cutoff**, turn **ON**, select **Triangle**, and set
   Depth to **2 octaves**. Lower Tone's base cutoff to approximately **1200 Hz**.
3. Turn **SYNC** on and choose **1/8 dotted**. A DAW supplies tempo; Standalone uses
   the editable fallback BPM, initially **120**.
4. Choose **Pitch**, turn it on, use Sine at **5 Hz / 0.3 st** for vibrato. The
   filter lane keeps its own timing while the pitch lane runs independently.
5. Try **Sample & Hold** on FM depth, Coupling or Stress for stepped textures.

Factory patches start with all LFO lanes off and depth zero. Enable a destination
and raise its depth to hear it. Destination selection only chooses which lane is
displayed; it does not reroute or disable the others. The header shows enabled-lane
count. The waveform preview's orange marker follows the selected lane's phase.

## Waveforms and timing

- **Sine**, **Triangle**, **rising Sawtooth**, **50% Square**, **Sample & Hold**.
- Free rate per destination: **0.01–20 Hz**. Rate edits preserve phase. Oscillators
  are global, not restarted by each note; reset/reprepare clears their phase.
- Sync subdivisions: **4 bars, 2 bars, 1 bar**, plus **1/1 through 1/64 notes**,
  each available straight, dotted or triplet.
- Synced lanes follow host quarter-note position. Tempo changes, loops and seeks
  retarget musical phase; while stopped with a valid position, phase stays there.
- Bar divisions use host meter, bar start and bar count when available, including
  meter changes. Missing bar metadata falls back to the quarter-note grid/current
  meter. Bar subdivisions count actual bars; note subdivisions count quarter beats.
- Without musical position, synced lanes advance continuously at host BPM or
  fallback BPM. Without valid host BPM, use the **20–300 BPM** fallback control.
  The readout explicitly identifies **HOST** or **FALLBACK** timing.
- Sample & Hold is deterministic: each lane hashes its cycle number with a distinct
  seed. A positioned seek back to a cycle reproduces its held value rather than
  drawing a new random number.
- Waveform/seek transitions use a **3 ms** smoothing time constant. Depth and
  enable/disable changes ramp over **20 ms**. These ramps soften square/saw/S&H
  transitions while retaining their low-frequency character.

## Depth and routing

| Destination | Depth range | Behavior |
| --- | --- | --- |
| Pitch | −24 to +24 st | Adds equally to carrier/modulator pitch, alongside tuning, wheel and pitch gestures |
| FM depth | −4 to +4 index | Adds to base FM index; effective index stays within 0–4 |
| Filter cutoff | −6 to +6 octaves | Adds in logarithmic pitch space to base cutoff, filter ADSR and Pitch + Tone |
| Amplitude | 0–100% | Tremolo attenuation; never boosts above nominal gain |
| Coupling | −100 to +100% | Adds to base coupling; effective value stays within 0–1 |
| Stress | −100 to +100% | Adds independently to base stress; effective value stays within 0–1 |

Negative bipolar depth inverts that lane's waveform. All modulation is applied to
rendered values, not written into base parameters or automation readback. Amplitude
modulation is post-Tone/pre-Space, so it does not repeatedly gate the reverb tail.
Effective cutoff stays between 10 Hz and 0.42 × sample rate. The shared filter and
network architecture remain unchanged.

## State and implementation

This is deliberately **greenfield**: no legacy Motion mode or preset/session
migration. The current format is preset **v4**, XML/session **v5**, with **68 host
controls**. Unsupported old formats are rejected without changing current sound;
old user files are not deleted. Keep/Compare/Undo/Redo and portable kept sounds use
the new schema. Serialized history stores parameter-index arrays rather than a
fixed-width integer mask, supporting controls beyond host index 63.

- `Source/DSP/Lfo.h`: waveform definitions, musical divisions, deterministic cycle
  hashing, six independent phase/depth/smoothing states and transport context.
- `MotionPanel.*`: selected-lane controls, waveform preview and tempo status.
- `PluginProcessor.cpp`: once-per-block host timing capture without UI access.
- `InstrumentRenderer.cpp`: bounded stack frames for sample-clock modulation.
- `Engine.cpp`: pitch/FM/network offsets consume sample frames without rewriting
  base values or resetting oscillator phases.

## Local verification

```sh
make check
make check-full
build/local-release/nitride_perf_artefacts/Release/nitride_perf \
  --lfo --output build/checks/lfo-enabled.json
```

`LfoTests.cpp` covers all 30 waveform/destination combinations, independent clocks,
phase-preserving rate edits, every subdivision, host BPM/PPQ and bar/meter handling,
stopped transport, seeks, missing/invalid timing, fallback BPM, dezippering,
deterministic S&H, block-size invariance, visible controls, lane setting retention,
Compare, current preset/state recall, pitch composition/band suppression, numeric
depth entry and scoped history beyond index 63. Active
six-lane host writes run through the allocation/concurrency sanitizer harness.
Transport behavior is exercised with a JUCE playhead harness; the installed-AU
check verifies the control contract, state and MIDI/audio integration, not a
manually operated DAW transport session.

Current factory regressions check fresh 0.5.0 fingerprints, deterministic rendering,
exact zero-depth neutrality and block-size invariance. The earlier historical fixtures are retained
as archival material, not run as a compatibility gate. DSP/pitch/filter analytic
tests continue protecting the underlying synthesis and note ownership.

`--lfo` benchmarks paired one/four/twelve-voice six-lane workloads and all-S&H
stress. Ordinary pre/post reports can use `--allow-sound-change` to explicitly
record the intentional removal of legacy Motion instead of claiming matching old
fingerprints. This flag does not relax normal optimization comparisons by default.

### Measured results

Local Release measurements on **2026-10-11**, Apple M4 Max, Clang 21.0.0,
macOS 27.0.1, pinned JUCE 8.0.15: **three passes, two audio seconds per case/pass,
0.5-second warmup**, 48/96 kHz, 64/256-frame blocks. No concurrent build or playback
was run during the final measurements. Figures below are median pass audio-budget
occupancy, not the operating system's overall CPU percentage.

| Paired workload | Rate / block | Six lanes off | Six lanes on |
| --- | --- | ---: | ---: |
| One voice | 48 kHz / 64 | 3.88% | 3.92% |
| Four voices | 48 kHz / 64 | 14.68% | 14.17% |
| Twelve-voice stress | 48 kHz / 64 | 48.18% | 48.30% |
| Twelve-voice stress | 48 kHz / 256 | 48.06% | 48.19% |
| One voice | 96 kHz / 64 | 7.65% | 7.63% |
| Four voices | 96 kHz / 64 | 28.16% | 27.34% |
| Twelve-voice stress | 96 kHz / 64 | 94.18% | 95.35% |
| Twelve-voice stress | 96 kHz / 256 | 93.89% | 95.24% |

All **16 on/off pairs** had different audio fingerprints. These are total sound
workloads, not an isolated LFO microbenchmark: modulation changes the network's
operating point and branch costs, so lower enabled timings do **not** establish a
DSP optimization. The all-S&H twelve-voice cases measured 47.87% at 48 kHz / 64
and 93.12% at 96 kHz / 64.

The ordinary pre/post matrix measured about **0.15–1.59% more render time** for
polyphonic/release workloads with the new bank neutral. Lead overhead was
1.95–2.63%; idle's 64-frame median increased from 4.83 to 5.83 microseconds.
**24/32** ordinary fingerprints intentionally changed after removing legacy Motion;
the eight idle/lead fingerprints remained exact. This is not a sound-preserving
optimization comparison.

Both final matrices recorded **zero overdue blocks at 48 kHz**. Maximum polyphony
at 96 kHz remains marginal: mixed-wave twelve-voice Motion recorded **236/9,000**
overdue blocks at 64 frames and **51/2,250** at 256 frames. Its 64-frame p99 was
679.46 microseconds against a 666.67-microsecond deadline. Do not interpret the
feature as solving the existing maximum-polyphony headroom limit.

Six Debug and six Release suites, focused ASan/TSan safety checks, and the installed
68-control AU's automation/state/MIDI checks passed. Audited callbacks exercised
active host-synced lanes without allocating/deallocating memory; JUCE callback
limitations remain as documented in `HARDENING.md`. All eleven native interaction
checks passed. App-local playback through MacBook Pro Speakers passed with peak
0.0417, approximately 12.03% device CPU, zero xruns/faults and completed release.
The default LG display output produced no callbacks; an initial explicit device
switch run recorded one xrun, while repeat and final runs recorded none. The review
app's `--audio-check-output "MacBook Pro Speakers"` selects an app-local device
without changing the system default.

Artifacts are under `build/checks/`: `lfo-before.json`, `lfo-after.json`,
`lfo-enabled.json`, `lfo-ui.png`, profile test logs and installed-AU logs.
