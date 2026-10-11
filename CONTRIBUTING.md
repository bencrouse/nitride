# Contributing

## Local validation

Since 0.5.0, Motion/state development is greenfield: no legacy migration gate.
`CurrentRegressionTests.cpp` checks deterministic current factory sounds and
zero-depth neutrality; `LfoTests.cpp` covers the six-lane modulation bank.
Historical captures remain archival, unchanged. See `docs/LFO.md`.

Run `make check` during development and `make check-full` for clean Debug/Release,
ASan/TSan and installed-AU integration checks. CMake downloads checksum-pinned
JUCE 8.0.15 by default; `JUCE_DIR` is an explicit matching-checkout override.
See `docs/HARDENING.md` for profiles, artifacts, callback contracts and limitations.

Audio and compatibility expectations under `Source/Tests/Fixtures/` are preserved
inputs. Normal tests must never regenerate them. Intentional sound/contract changes
require an explicit reviewed baseline update. Production UI is under
`Source/Instrument`; production DSP is under `Source/DSP`. Keep editor keyboard
listeners message-thread-only and route commands through `EditorMidiBridge`.

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
`InstrumentView`. Keep their integration and current-schema regression checks
passing. `docs/MILESTONE_02.md` records the earlier session/preset milestone.
Treat `Source/Instrument/Parameters.h` as the authoritative automation/condition
contract. The project currently assumes a greenfield state/automation schema.
Keep host parameter listeners independent of document/file operations and unbounded waits.

`make test` covers hosted MIDI/render equivalence, automation/state/preset workflow
and DSP regressions. After AU integration changes, also run
`./build/nitride_au_component_tests` against the installed component. Native UI
changes can be checked through the review app's `--interaction-check` and
`--audio-check` modes; interaction tests use a temporary preset library. If a device
is unavailable, use `--audio-check-output "MacBook Pro Speakers"` to select an
app-local output without changing the system default.

The pitch suite covers Glide/Autobend, note priority, independent FM and band
suppression, current state and native controls. `make render-pitch` creates matched
musical listening examples. `docs/PITCH_MOTION.md` specifies the feature behavior
and measured limits. AU continuous values use the normalized API even when value
strings display physical units; indexed destinations use their discrete indices.

For performance changes, capture a Release `make benchmark` baseline before
editing DSP and compare the candidate against it. Require unchanged audio
fingerprints for exact optimizations, check per-pass variability and deadline
headroom, then run the Release regression suites. Profile independently of timing
runs. `docs/PERFORMANCE.md` defines the workloads and records the first measured
optimization; timings are not CTest pass/fail criteria.
