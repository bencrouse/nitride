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
Treat `Source/Instrument/HostParameters.h` as the stable version-1 automation
contract. Keep host parameter listeners independent of document/file operations.

`make test` covers hosted MIDI/render equivalence, automation/state/preset workflow
and DSP regressions. After AU integration changes, also run
`./build/nitride_au_component_tests` against the installed component. Native UI
changes can be checked through the review app's `--interaction-check` and
`--audio-check` modes; interaction tests use a temporary preset library.
