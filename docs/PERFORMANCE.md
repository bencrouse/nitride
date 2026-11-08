# Performance / measured milestone-3 work

## Approved-sound optimization / eval-044

The final isolated 64-case M4 Max Release matrix retains **all approved eval-043
audio fingerprints**, with identical workload settings/machine/compiler metadata.
Median pass CPU falls **6.6–16.9%** across cases (median improvement 10.9%). All 50
spectral result records are also exactly unchanged. Full-quality results:

| Voices / workload | 48 kHz median budget load | 96 kHz median budget load |
| --- | ---: | ---: |
| 1, five characters | 5.9–7.2% | 11.3–13.8% |
| 4, five characters | 20.1–23.9% | 37.0–44.6% |
| 12, five characters | 61.0–73.1% | 116.5–138.3% |
| Advanced 12 | 73.1–73.2% | 141.5–142.8% |

**48/64 cases** meet the strict zero-overdue / p99 <= 80% headroom gate. There are
two overdue Transistor 12-voice / 48 kHz / 64-frame blocks, none in other 48 kHz
cases or any one/four-voice case. All 96 kHz 12-voice medians still exceed budget;
four 48 kHz maximum-polyphony cases also fail the stricter headroom gate. These
failures are not waived by the overall speedup or by benchmark completion.

The separate continuity workload completed **1,800 audio seconds**, 61 exact
in-memory host-parameter recalls and zero numerical faults. It used 4–5 live voices
during retriggering and produced finite audio. It also retained **21 overdue blocks**,
a maximum **83.301-second block**, and 1,602.963 seconds of elapsed wall time despite
the process-scoped idle-sleep assertion. The cause of those long stalls is unproven.
This passes audio/state continuity, **not** the deadline gate or a real-time host/device
soak. Raw timings remain intact; `summary-with-timing.json` explicitly separates the
two verdicts. Nothing is relabelled an M2 or Logic pass.

Rage skips unused primitives, caches converged solver evaluations and hoists frozen
excitation setup. Private helper inlining and supported Release-only DSP IPO retain
normal floating-point semantics. A direct archived-core test preserves exact
doubles and diagnostics across 1.6 million modulated/character-switching samples.
No quality, solver bound, oversampling factor or voice count is reduced.

Focused M4 Max prototype measurements (96 kHz / 64 frames, three two-second passes)
retain all 16 audio fingerprints against the approved eval-043 baseline. Twelve-voice
median loads change as follows; these are focused measurements, not the final matrix:

| Workload | Approved eval-043 | Optimized focused prototype |
| --- | ---: | ---: |
| Diode | 156.5% | 130.5% |
| Distortion | 135.4% | 115.3% |
| Softclip | 149.2% | 122.5% |
| Tape | 164.9% | 131.3% |
| Transistor | 132.7% | 110.9% |
| Advanced | 153.5% | 135.3% |

All remain over budget in that maximum-polyphony 96 kHz case. A sampled complete
processor profile shows both filter math and oscillator/network work are significant;
profiled timings are not capacity evidence. The additive filter qualification tools
retain complete raw failures and require zero overdue blocks **and** p99 <= 80% of
the deadline for an offline headroom pass. Offline continuity is an audio-duration
processor/state test, never a real-time Logic/device soak. Actual M2 and manual Logic
checks remain pending; the [no-install handoff checklist](FILTER_QUALIFICATION_MANUAL.md)
does not authorize installation or capture user projects/settings.

## Character identity / eval-043

The full 64-case matrix was captured twice. The initial run includes approximately
83-second wall-clock stalls and extreme pass aggregates, retained in the private
evidence. Their cause is not proven. A separate `caffeinate -i` repeat prevents idle
sleep for the benchmark process only; all 64 audio hashes match between runs.

Repeat M4 Max 48 kHz 12-voice medians are 68.1–86.6%, advanced 78.6%, with five
overdue character-case blocks and no advanced misses. At 96 kHz all 12-voice
medians exceed budget (131.2–164.9%; advanced 153.0–153.5%). One/four-voice repeat
cases had no observed misses at either rate. Neither the original timing failures
nor the repeat's remaining deadline misses are waived. Full 4× processing, solve
precision and 12 voices remain enabled. Actual M2 and DAW reopen/soak remain
unqualified. See [identity qualification and boundaries](FILTERS.md#identity-qualification-and-boundaries).

## Character audibility / eval-042

The added nonlinear excitation for the four revised characters retains 4× processing,
full solver precision and 12 voices. In the unchanged 64-case M4 Max matrix, 48 kHz
12-voice character medians are 69.9–87.8%, advanced 80.6–80.7%. Two character-case
blocks exceeded deadline; no advanced 48 kHz misses were observed. At 96 kHz all
12-voice medians exceed budget (132.7–165.7%; advanced 156.1–157.3%). One/four-voice
cases had no observed misses at either rate. These are not universal 12-voice,
actual M2 or DAW scheduling/soak qualification. Start with one/few notes at 48 kHz.
See [character fixes and current caveats](FILTERS.md#character-audibility--0742--eval-042).

## Integrator-return experiment / eval-041

The new nonlinear return/discrete-gradient solver retains 4× processing, full solve
precision and 12 voices. In the unchanged 64-case M4 Max matrix, 48 kHz medians were
62.3–75.4% for 12-voice character cases and 68.2–68.8% for advanced 12 voices. There
were 35 overdue character-case blocks and six advanced 64-frame overdue blocks;
median headroom is not a no-miss qualification. At 96 kHz, 12-voice medians exceed
budget (118.4–144.9%; advanced: 132.7–133.2%). One/four-voice cases had no observed
misses at either rate. Start auditions at 48 kHz with one/few notes. Actual M2 and
DAW reopen/soak remain unqualified. See [current caveats](FILTERS.md#cpu-and-audition-limits).

## Stronger Rage / eval-040

The residual-feedback redesign uses the unchanged 64-case `--rage` matrix. On the
M4 Max, measured 48 kHz 12-voice loads are 55.2–60.6% (advanced: 67.6–67.7%), with
zero observed deadline misses in this run. At 96 kHz all 12-voice characters exceed
median budget (105.2–116.6%; advanced: 129.3–129.8%). One/four-voice cases remain
within measured deadlines. See [current qualifications](FILTERS.md#eval-040-checks-and-limits).
Full quality/polyphony remain enabled; use 48 kHz for auditions. Historical results
below are not substituted for the new measurements or actual M2/DAW qualification.

## Rage family / eval-039

`nitride_perf --rage` adds five characters at 1/4/12 voices and an advanced 12-voice
case. The historical matrices below are unchanged. See [current filter measurements
and limits](FILTERS.md#measured-eval-039-limits): on the M4 Max, most 12-voice 96 kHz
Rage cases exceed their real-time budget. One/four-voice cases passed the measured
deadlines; full quality and the 12-voice limit remain available, not silently reduced.
Actual M2 and DAW/soak qualification are still separate.

Performance changes use `Source/Tests/PerformanceHarness.cpp` to measure the actual
`NitrideAudioProcessor::processBlock` path in a **Release** build. This is the first
measured optimization pass, not completion of the sound-quality/performance milestone.

## Run and compare

```sh
make benchmark
# Writes build/performance/latest.json

make benchmark PERF_ARGS='--output build/performance/baseline.json --label before'
# Make a DSP change, then:
make benchmark PERF_ARGS='--output build/performance/candidate.json --compare build/performance/baseline.json --label after'

# Short harness check:
make benchmark PERF_ARGS='--quick --output build/performance/smoke.json'

# Focused repeatability check:
make benchmark PERF_ARGS='--case stress12 --rate 96000 --block 64 --passes 5 --seconds 3 --output build/performance/stress.json'
```

`PERF_BUILD_DIR` defaults to `build/performance`, separate from the normal Debug
build. `NITRIDE_BUILD_TESTS` must be enabled. Once built, run
`build/performance/nitride_perf_artefacts/Release/nitride_perf` directly to exclude
build activity from repeated measurements. `--help` lists the options.

Capture a baseline before editing DSP. For back-to-back A/B checks, preserve its
executable before rebuilding. Compare reports only with matching machine, compiler,
configuration, duration, warmup, pass count and requested workloads. Focused runs
may compare against a covering full-matrix baseline. Keep other CPU
work out of timing runs. Profile separately; profiled timings are not benchmark results.

## Method

Default workload version 1 has **32 cases**: eight performances × 48/96 kHz ×
64/256-frame buffers. Each case runs three independent passes, two audio seconds
per pass, after 0.5 seconds of warmup. Workload order is shuffled with a fixed seed
each pass. Processor construction, patch loading, preparation, MIDI-input assembly,
output auditing and JSON writing are outside the timed blocks. Host parameter
callbacks, MIDI dispatch, rendering, effects and live-state publication are included.
No editor or audio device is opened.

| Workload | Performance |
| --- | --- |
| `idle` | No notes, normal pad/effects settings |
| `lead` | One held Contact lead voice |
| `pad4` | Four-note Soft pad chord |
| `pad12` | Twelve held Soft pad voices |
| `stress12` | Twelve voices; maximal Coupling, Stress, FM, tone resonance, motion and space; 10 ms Response |
| `automation12` | Stress workload with Coupling, Stress and Response automated every host block |
| `retrigger12` | Stress workload with note-off/on events at distinct sample offsets, eight retriggers per second |
| `release12` | Twelve warmed pad voices released at the first measured block; voices and effect tails continue rendering |

The report records:

- Mean render time / rendered-audio duration, and the median of per-pass loads.
- Nearest-rank p50/p95/p99 block times, maximum time and count over the block deadline.
- Every per-pass load, hardware/build/compiler metadata and measurement timestamp.
- Observed minimum/maximum voice counts, stereo peak/RMS and a deterministic
  fingerprint of every rendered float word, across all measured passes.

**Load % is single-thread audio-budget occupancy**, not total-machine CPU usage.
100% means rendering takes as long as the block's audio duration. Offline
over-budget blocks identify insufficient headroom; they are not measured device
xruns or proof of dropout-free DAW playback. This harness excludes GUI rendering,
host/plugin-wrapper overhead, device scheduling and concurrent plugin instances.
Its warmup focuses on sustained rendering; cold note-on/preparation latency is not
a reported metric. Very short `--quick` runs check functionality, not acceptance.

Compare mode rejects incompatible reports and fails if audio fingerprints change.
It reports timing deltas rather than applying an arbitrary noisy CPU pass/fail
threshold. Timing evidence is evaluated with pass-to-pass variation and a focused
repeat. Preserve raw reports before cleaning `build/`.

## First measured optimization / 2026-10-08

Baseline DSP: `968750d`. Machine: **Apple M4 Max, 14 cores**, macOS 27.0.1,
Apple Clang 21.0.0 (`clang-2100.3.34.2`), arm64 Release, JUCE 8.0.15.
Both binaries used the same compiler/configuration and harness.

A separate five-second `sample` profile of `stress12/48000/64` placed almost all
sampled processor work in `Engine::render`/`voiceSample`/`networkSample`, dominated
by oscillator trigonometry. The retained changes are exact computational reuse:

1. Cache per-voice Nyquist masks until the fundamental's bits change; bends and
   legato glides invalidate the cache.
2. Skip sine evaluations for reference partials with exactly zero band weight.
3. Remove the unused network carrier-node sine evaluation.
4. Recompute Response rates and the pitch multiplier only when their smoothed
   inputs actually change; still update smoothing every sample.
5. Calculate legacy body-model damping only while its treatment/fade is active.

No approximate trigonometry, reduced oversampling, reduced polyphony or altered
control-rate smoothing was used. All 32 before/after audio fingerprints matched.
All three Release CTest suites passed, including legacy mappings, treatment changes,
pitch bends, sample-rate/block-size checks and automation/session regressions.

### Full matrix

Values below are median per-pass budget occupancy. The baseline and optimized
reports are locally retained at `build/performance/baseline.json` and
`build/performance/optimized.json`. Three passes × two audio seconds per case.

| Workload | 48k / 64 before → after | 48k / 256 before → after | 96k / 64 before → after | 96k / 256 before → after |
| --- | ---: | ---: | ---: | ---: |
| Idle | 0.41 → 0.32% | 0.35 → 0.26% | 0.82 → 0.65% | 0.72 → 0.52% |
| Lead | 3.94 → 3.74% | 3.84 → 3.67% | 7.58 → 7.29% | 7.47 → 7.14% |
| Pad / 4 | 15.09 → 14.39% | 15.01 → 14.32% | 28.83 → 27.59% | 28.77 → 27.42% |
| Pad / 12 | 47.29 → 44.64% | 47.46 → 44.58% | 91.36 → 86.39% | 90.69 → 86.00% |
| Stress / 12 | 50.87 → 47.76% | 50.92 → 47.61% | 99.71 → 93.25% | 99.24 → 93.07% |
| Automation / 12 | 48.98 → 46.31% | 49.18 → 46.15% | 95.47 → 90.32% | 95.82 → 90.17% |
| Retrigger / 12 | 50.81 → 47.76% | 50.77 → 47.63% | 99.16 → 93.27% | 98.82 → 93.16% |
| Release / 12 | 32.93 → 31.02% | 32.80 → 31.01% | 63.41 → 60.06% | 63.58 → 59.80% |

Polyphonic workloads improved **4.3–6.5%** in relative render cost. Idle improved
21.5–26.9%, a small absolute saving (less than 0.2 percentage points).
The oscillator-only first experiment is retained in
`build/performance/oscillator-cache.json`; the additional control-rate caching
mostly benefits idle and lead workloads.

### Focused confirmation

Ran the saved baseline executable and the candidate back-to-back, each for five
passes × three audio seconds, `stress12/96000/64`: **22,500 measured blocks** each.
The deadline is **666.67 µs**. Raw confirmation values are archived in
[`performance-confirmation-2026-10-08.json`](performance-confirmation-2026-10-08.json).

| Metric | Baseline | Optimized |
| --- | ---: | ---: |
| Median pass load | 99.52% | 93.26% |
| Per-pass load range | 99.47–99.66% | 93.09–93.38% |
| p50 block time | 660.33 µs | 618.46 µs |
| p95 block time | 688.25 µs | 644.50 µs |
| p99 block time | 710.21 µs | 664.04 µs |
| Maximum block time | 840.96 µs | 767.25 µs |
| Over-budget blocks | 7,062 / 22,500 (31.39%) | 194 / 22,500 (0.86%) |
| Active voices | 12 throughout | 12 throughout |
| Audio fingerprint | `119baa715f2b8e39` | `119baa715f2b8e39` |

This confirms **6.29% lower render cost**, larger than the observed pass-to-pass
variation. The 96 kHz maximum-polyphony case still has tight headroom: p99 is almost
the entire deadline and some blocks exceed it. Further optimization and real-host
testing are needed before treating that workload as a production acceptance pass.
Minimum supported hardware and desired voice/sample-rate headroom should define
the eventual acceptance target; current measurements establish progress and limits.

## Pitch-motion extension / 0.3.0

`--pitch-motion` selects paired Glide/Autobend on/off MIDI performances. The original
version-1 workload matrix is retained for fingerprint comparisons. Pairs use the
same chord/retrigger input; four-note chords can reach 12 active voices through
release tails, so check the report's observed voice counts as well as held-note count.

```sh
make benchmark PERF_ARGS='--pitch-motion --rate 48000 --block 64 --output build/performance/pitch-48k.json'
```

The harness exposed an initial ~10% inactive-feature regression, addressed by
separately compiling constant-ratio and moving-ratio oscillator paths. Retained
inactive overhead is typically around 1–2%, up to approximately 3% in measured
cases, with all 32 baseline hashes matching. Combined maximum-stress motion measured
54.08% budget occupancy at 48 kHz / 64 frames and 103.55% at 96 kHz / 64 frames on
the M4 Max. See [pitch-motion measurements](PITCH_MOTION.md) for the full paired
results, method, reproduction commands and current performance limits.
