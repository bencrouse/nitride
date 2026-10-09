# Contributing

## Principles

- Start with the musical gesture and native UI before implementing the voice engine.
- Follow Carbide's JUCE / CMake / C++20 conventions.
- Keep ordinary sound shaping legible; preserve the approved network field and Response gesture.
- Keep processor/DSP code independent of editor drawing.
- Preserve parameter IDs once playable builds are released.

## Local loop

```sh
make standalone
make test
```

For AU host integration:

```sh
make reload-au
```

The AU and review app share `InstrumentSession`, `InstrumentRenderer` and
`InstrumentView`. Preserve the frozen review-render comparisons when changing that
path. `docs/MILESTONE_02.md` describes automation, session/preset scope and checks.
Treat `Source/Instrument/Parameters.h` as the authoritative automation/condition
contract. Preserve the original 17 host positions, mono at index 16, and all IDs;
append new controls with a new version hint. Keep host parameter listeners
independent of document/file operations and unbounded waits.

`make test` covers hosted MIDI/render equivalence, automation/state/preset workflow
and DSP regressions. After AU integration changes, also run
`./build/nitride_au_component_tests` against the installed component. Native UI
changes can be checked through the review app's `--interaction-check` and
`--audio-check` modes; interaction tests use a temporary preset library.

The pitch suite covers Glide/Autobend, note priority, independent FM and band
suppression, migration and native controls. `make render-pitch` creates matched
musical listening examples. `docs/PITCH_MOTION.md` specifies the feature behavior
and measured limits. AU continuous values use the normalized API even when value
strings display physical units; indexed destinations use their discrete indices.

For performance changes, capture a Release `make benchmark` baseline before
editing DSP and compare the candidate against it. Require unchanged audio
fingerprints for exact optimizations, check per-pass variability and deadline
headroom, then run the Release regression suites. Profile independently of timing
runs. `docs/PERFORMANCE.md` defines the workloads and records the first measured
optimization; timings are not CTest pass/fail criteria.
