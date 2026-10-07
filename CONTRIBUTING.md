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

Milestone 1 is audible: the AU and review app share `InstrumentRenderer` and
`InstrumentView`. Preserve the frozen review-render comparisons when changing that
path. `docs/MILESTONE_01.md` describes the current scope; full host automation and
session/preset workflows are milestone 2.
