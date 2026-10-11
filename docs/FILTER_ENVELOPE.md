# Filter envelope / Nitride 0.4.0

The existing stereo post-mix Tone filter now has its own ADSR and signed Amount.
Amplitude and filter envelopes are independent; this does not add per-voice filters
or change the adaptive-network synthesis.

![Dedicated native filter envelope](filter-envelope-0.4.png)

## Try it

Choose **Glass keys**, set Tone cutoff to approximately **800 Hz**, and set filter
envelope **Amount to +3 octaves**. Try **Attack 20 ms, Decay 100 ms, Sustain 30%,
Release 400 ms**. Each strike opens the filter before settling to its sustain
offset. For a closing gesture, start with a higher cutoff and use negative Amount.

The new **Filter Envelope** strip has the same draggable graph and vertical
ADSR value editing as Amplitude. Amount supports dragging, arrow keys, accessible
value editing and double-click reset. All controls participate in host automation,
Keep, Compare, Undo/Redo and session recall. The native panel is now **1200 × 960**;
the original panels keep their geometry, with the keyboard/footer moved down to
make room for the dedicated envelope strip.

## Controls

| Control | Range | Initial value |
| --- | --- | --- |
| Attack | 1 ms–4 s | 10 ms |
| Decay | 10 ms–4 s | 300 ms |
| Sustain | 0–100% | 50% |
| Release | 20 ms–8 s | 400 ms |
| Amount | −6 to +6 octaves | **0** |

The envelope uses a linear attack from its current level, exponential decay toward
Sustain and linear release from its note-off level, matching the amplitude
envelope's shapes. Decay is a time constant: after one Decay interval, approximately
63% of the peak-to-sustain movement is complete. Sustain is an envelope fraction,
not an absolute cutoff. Retriggering and timing edits preserve the current level;
Amount changes are dezippered over 25 ms.

For envelope level `E`, Amount `A` in octaves and pitch-motion Tone offset `P` in
semitones, cutoff is:

```text
base cutoff × 2^(A × E + P / 12)
```

The combined moved cutoff retains the existing 10 Hz to 0.42 × sample-rate bounds.
The Tone readout displays the base cutoff, not the modulated value. Zero Amount
removes only the filter ADSR contribution: Glide/Autobend Pitch + Tone still works.
The envelope continues tracking notes while Amount is zero, so enabling modulation
during a held note uses its current stage.

## Shared gate and note behavior

- **Poly:** fresh note-ons retrigger the shared envelope. Releasing part of a chord
  does not start Release; the final held or pedal-held owner does.
- **Mono:** fresh note-ons retrigger. Returning to an already held note preserves
  the filter envelope rather than creating a new strike.
- **Legato:** overlapping physical keys preserve the envelope, just like Amplitude.
  Pedal-held notes alone do not count as physical-key overlap.
- Repeated presses require their matching releases. All 16 host MIDI channels and
  the editor's separate owners participate in the shared gate.
- Hold captures an existing note without retriggering. Closing the editor releases
  only its ownership; host notes and pedal-held host notes remain intact.
- Channel panic and editor overflow recovery release only affected owners. Audio
  reset/reprepare clears the envelope. Filter Release does not prolong amplitude
  voices: once the amplitude is silent, a remaining filter release cannot make sound.

Host MIDI triggers remain sample-positioned; editor input keeps the bounded,
next-block handoff. The envelope advances once per host sample and allocates no
render-thread storage.

## Compatibility

Five parameters append after the existing 28 controls, for **33 total**:
`filter_attack`, `filter_decay`, `filter_sustain`, `filter_release`, `filter_amount`.
The new controls use version hint 3; all earlier IDs, ordering, ranges and version
hints remain unchanged. AU continuous automation still uses normalized 0–1 values;
presets and APVTS raw values use physical units.

Presets now use **v3**; XML/session documents use **v4**. Preset v1/v2 and XML v1/
session v2/v3 migrate with filter Amount zero, including reference/history/portable
presets. The four factory patches also retain zero Amount and their original sound.
Serialized scoped-undo masks use 64-bit integers to cover host indices 31 and 32;
old 32-bit masks remain loadable. Mid-note envelope state is not serialized.

## Regression and measurement commands

```sh
make check
python3 scripts/check.py --full --resume-from debug
build/local-release/nitride_perf_artefacts/Release/nitride_perf \
  --filter-envelope --output build/checks/filter-envelope-enabled.json
```

`FilterEnvelopeTests.cpp` runs within the automation suite. It checks analytic
44.1/48/96 kHz stage timing, retrigger/automation continuity, signed audible movement,
different host block sizes, Pitch + Tone composition, ownership/pedal/Hold/overflow,
mono return-note behavior, UI/automation, high-bit serialized undo masks, Compare,
preset round trips and legacy defaults. Existing independent 0.3.0 captures are
unchanged and remain the inactive-feature sound baseline. Safety tests exercise
enabled modulation and filter automation under allocation auditing and sanitizers.

The `--filter-envelope` benchmark adds paired on/off MIDI performances for mono,
four/twelve held notes, closing modulation and combined Pitch + Tone. On/off pairs
intentionally have different sound; compare cost separately from the ordinary
before/after matrix, which must retain exact audio fingerprints.

## Local results

Debug and Release each pass all six suites. The allocation-audited enabled-render
scenarios, focused ASan/TSan checks and installed 33-control AU validation/state/
MIDI checks pass. All eight preserved captures remain sample-exact. The signed
sound test measures output energy of **2.98 / 11.39 / 54.80** for closed / neutral /
open modulation, with **zero sample difference** between its 64- and 257-frame
renders.

Native interaction checks pass, and device playback with Glide/Autobend plus filter
modulation reports **zero xruns and numerical faults**, with completed note release.
All 20 measured envelope on/off pairs have distinct audio fingerprints.

The ordinary Release benchmark retains all **32 exact audio fingerprints** against
the pre-feature build. On the M4 Max, three passes × two audio seconds per case:

| Audio-budget occupancy, 48 kHz / 64 frames | Before | After |
| --- | ---: | ---: |
| Idle | 0.37% | 0.37% |
| One lead | 3.84% | 3.85% |
| Four-voice pad | 14.63% | 14.77% |
| Twelve-voice stress | 48.42% | 48.70% |

Inactive-feature relative changes across the matrix range from approximately
−0.02% to +2.37%; most are below 1.5%. These small timing differences include
scheduling noise and are not interpreted as optimization gains.

| Paired performance, 64 frames | 48 kHz off → on | 96 kHz off → on |
| --- | ---: | ---: |
| One retriggered mono lead | 4.03 → 4.04% | 7.79 → 7.82% |
| Four held notes / retrigger | 37.11 → 37.29% | 71.17 → 71.68% |
| Twelve held notes / stress | 48.65 → 48.55% | 94.84 → 94.77% |
| Twelve notes + Pitch + Tone | 49.28 → 49.26% | 99.29 → 98.84% |

Retriggering poly performances include overlapping amplitude-release voices, so
four held keys can use more than four sounding voices. The shared envelope adds
little cost relative to synthesis. The existing maximum-polyphony 96 kHz headroom
limitation remains, including over-budget blocks; these measurements do not promise
universal real-time deadline compliance.

Raw reports are local artifacts under `build/checks/`: `filter-envelope-before.json`,
`filter-envelope-after.json`, `filter-envelope-enabled.json`, profile logs and
`filter-envelope-ui.png`. The new envelope's on/off fingerprints intentionally differ.
