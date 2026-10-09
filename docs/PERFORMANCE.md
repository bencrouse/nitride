# Performance / measured milestone-3 work

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
