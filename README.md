# nitride

A native FM synth experiment for macOS, exploring playable ways to push sounds
from familiar tones into unstable textures.

**Work in progress.** It's playable as a standalone app or an Audio Unit, with
presets and host automation. The sound and interface are still evolving; this
isn't a finished release.

![Instrument interface](docs/filter-envelope-0.4.png)

## Try it

Requires macOS 15+ on Apple Silicon, a macOS SDK + Clang, and CMake 3.24+.
The build downloads JUCE automatically.

```sh
make standalone
```

For your DAW, run `make reload-au` to build, install and validate the AU in
`~/Library/Audio/Plug-Ins/Components`. It also generates and installs the 53 categorized
native presets in `~/Library/Audio/Presets/nitride/nitride/`, including on a fresh clone.
Existing preset files and user edits are left untouched. Restart your DAW, then look
for **nitride** as an AU instrument; in Logic, browse its host **Settings** menu above
the plugin interface. No private evaluation workspace or separate preset download is needed.

## Poke around

- `make studies` — sound experiments and comparisons.
- `make gestures` — the playable control-surface study.
- `make check` — local development checks.
- [Contributing](CONTRIBUTING.md) — build, test and development notes.
- [Architecture](docs/ARCHITECTURE.md) — how the instrument fits together.
- [Design notes](docs/DESIGN.md) — the original brief and evolving direction.
- [Motion LFOs](docs/LFO.md) — waveforms, independent destinations and tempo sync.
- [Filter modulation](docs/FILTER_MODULATION.md) — key tracking, velocity and audio-rate FM.
- [FM expansion](docs/FM.md) — primary ratio, independent envelope, shapes and network-preserving operator mix.
- [Preset exploration](docs/PRESET_REVIEW.md) — versioned review sounds, MIDI demos and iteration notes.
- [Evaluation journal](docs/EVALUATION.md) — embedded Logic trial presets, persistent feedback and resumable history.
- [Factory presets](presets/factory/HOST_PRESETS.md) — 53 finalized sounds, Logic-native categories and the standalone browser.
- [Preset provenance](docs/PRESET_PROVENANCE.md) — original Nitride interpretations and the local-only research boundary.
- [Release qualification](docs/RELEASE_QUALIFICATION.md) — measured timing/performance evidence and pending M2/Logic acceptance.
