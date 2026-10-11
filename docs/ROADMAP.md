# Nitride development roadmap

## Direction

The next priorities are a handful of minor features, a stronger preset collection,
and better audio performance. The recommended sequence is:

**Engineering hardening → sound-affecting minor features → preset exploration →
measured performance work → final preset polish.**

Features determine what presets can express. Preset exploration identifies the
musically important sounds and workloads. Performance work makes those sounds
practical, and final voicing brings them together into a coherent instrument.

This is the next development sequence within the broader sound-quality and
performance milestone. It does not imply that all engineering cleanup must finish
before musical development can continue.

## Current foundation

- Milestone 1: playable native AU and Standalone, shared audio/UI paths and host MIDI.
- Milestone 2: host automation, persistent presets, session recall and Compare/Undo/Redo.
- Initial milestone-3 work: Release performance harness, measured baseline and
  exact DSP optimizations with matching audio fingerprints.
- Feature work in 0.3.0: independent Glide/Autobend, destination routing, legato
  and mono return-note behavior, compatible automation/state migration and paired
  performance measurements. See [pitch motion](PITCH_MOTION.md).
- Bounded local hardening: pinned independent builds, preserved historical/audio
  fixtures, bounded editor handoff, generation-aware recall, allocation/concurrency
  checks, production UI/DSP ownership and local sanitizer validation. See
  [hardening results](HARDENING.md).
- Feature work in 0.4.0: dedicated shared-filter ADSR and signed octave Amount,
  compatible state/automation, ownership-aware gating and measured on/off workloads.
  See [filter envelope](FILTER_ENVELOPE.md).
- Feature work in 0.5.0: six independent global LFO lanes, five waveforms,
  musical tempo sync and selectable pitch/FM/cutoff/amplitude/network modulation.
  Preset/state design is now greenfield, without legacy migration requirements.
  See [Motion](LFO.md).

See [milestone 2](MILESTONE_02.md) and [performance measurements](PERFORMANCE.md).
Current performance is usable at 48 kHz, but maximum polyphony at 96 kHz still has
tight headroom. The harness establishes progress; it does not establish commercial
production readiness.

## 1. Bounded engineering hardening

Strengthen the development foundation before adding more sound-affecting behavior.

### Real-time guarantees

- Completed in 0.3.0: remove the condition-writer spin-wait. Host parameter updates
  now publish independent scalar atomics without waiting. A coherent whole-bank
  transaction protocol remains a possible follow-up beyond per-control snapshots.
- Completed: exercise active playback with real APVTS writes, recall and editor
  lifecycle changes; test ownership collisions and bounded overflow recovery.
- Completed: self-verifying local allocation instrumentation and focused ASan/TSan.
  Residual JUCE callback allocations/locks and whole-bank ordering are documented
  explicitly in `HARDENING.md`.

### Reproducible regression protection

- Completed: local `make check` / `make check-full` profiles for builds, regression,
  installed-AU validation, benchmark smoke and sanitizer checks.
- Completed: checksum-pinned JUCE 8.0.15 acquisition independent of Carbide.
- Completed: preserved historical session/preset fixtures and frozen host contract.
- Completed: independent reference audio captures for representative performances. The
  existing hosted/review comparison shares the underlying engine and primarily
  protects integration; independent captures also protect against DSP sound changes.
- Keep timing comparisons on controlled hardware. Local checks verify benchmark
  functionality and sound correctness; timing reports quantify changes separately.

### Maintainability, incrementally

- Completed: production UI uses normal components and no study `.inc` dependency.
- Completed: production-owned `Source/DSP/Engine.*` / `nitride_dsp`; studies consume
  compatibility aliases. Deeper study scheduler/treatment decomposition remains incremental.
- Completed in 0.3.0: centralize typed parameter metadata and explicit host mapping
  in `Parameters.h`, preserving published IDs and legacy ordering.
- Retain the processor/session/view separation and regression coverage while making
  focused changes around the areas being developed.

Real-time handoff and reproducible checks are the initial priorities. Module
extraction and other cleanup can proceed alongside feature work.

## 2. Finish sound-affecting minor features

Confirm the feature list, then implement changes affecting modulation, note behavior,
parameter ranges or expression before final preset voicing.

Implemented: independent Glide/Autobend and a dedicated filter ADSR for the existing
shared Tone filter, plus the six-lane Motion bank. New modulation starts neutral.

Pure workflow improvements—browser conveniences, shortcuts and naming—can happen
alongside preset development.

**Exit condition:** the intended sound-design and performance controls are available,
their behavior is covered by meaningful checks, and current-schema sessions round-trip.

## 3. Explore a small reference preset collection

Develop approximately **8–12 strong, representative sounds** before attempting a
large factory library:

- Useful pads, basses, leads and percussion.
- Distinctive, extreme Nitride timbres.
- Restrained and expressive uses of the adaptive network.

Use this exploration to identify control limitations and missing affordances.
Record a deterministic MIDI performance for each reference sound, including its
relevant expression and note-release behavior. Add these sounds and performances
to the benchmark workload set.

**Exit condition:** the collection demonstrates the instrument's musical range and
provides realistic, repeatable workloads for optimization and sound regression checks.

## 4. Improve performance against real sounds

Define acceptance targets before the next substantial optimization pass:

- Minimum supported Mac/hardware class.
- Expected polyphony.
- Sample rates and buffer sizes.
- Desired p99 audio-thread headroom.

Measure both the reference preset performances and synthetic stress cases. Profile
separately from timing runs, change measured bottlenecks, and compare candidates
against preserved baselines.

For exact optimizations, require matching audio fingerprints. For intentional DSP
changes, assess explicit numerical/spectral/level differences and listening results
rather than silently accepting changed sound.

**Exit condition:** representative workloads meet the agreed headroom targets,
regression checks pass, and real-host playback supports the offline measurements.
Maximum-polyphony 96 kHz rendering remains a known area requiring further work.

## 5. Finish the preset library

Once features and relevant DSP behavior have settled, complete the deeper voicing:

- Consistent and appropriate output levels.
- Useful velocity, sustain, pitch-bend and other supported expressive responses.
- Musical variations and comfortable control ranges.
- Deliberate attack/release behavior and effect tails.
- Clear names, organization and presentation.
- Testing in actual musical contexts and DAW sessions.

**Exit condition:** the library contains compelling, playable sounds that remain
practical at their intended polyphony and supported performance settings.

## Open decisions and review points

- Glide/Autobend, filter ADSR and expanded LFOs are implemented; any further feature list is not yet
  specified. Changes to voice architecture or major sound behavior could alter
  the sequence above.
- Hardware, polyphony and headroom acceptance targets are not yet specified.
- Revisit feature scope after the reference preset exploration.
- Revisit final preset voicing after any intentional DSP sound change.
- Preserve measured before/after evidence for each performance optimization.
