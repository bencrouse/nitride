# Bounded engineering hardening / local development

This pass protects the approved `b22e504` / 0.3.0 instrument while making future
feature development easier to verify. Validation is explicitly invoked locally.

## Local commands

```sh
make check
make check-full
```

- `make check` incrementally builds the Debug regression tools and runs six suites.
- `make check-full` cleans the dedicated Debug/Release trees, builds the product
  and checks, validates the installed Release AU, exercises ordinary/pitch benchmark
  smoke runs, and runs focused lifetime/concurrency checks under ASan and TSan.
- `python3 scripts/check.py --profile release` selects a single profile.
- `python3 scripts/check.py --full --resume-from release` resumes a failed full
  run without deleting completed build trees.

Logs and machine/compiler/SDK metadata are written to `build/checks/`. Full Release
checks replace only the user-installed Nitride component. Local ad-hoc signing
seals the bundle resources; AU discovery/validation runs through `launchctl asuser`
in the logged-in user's GUI context. This avoids an empty system-only registry
when invoking development commands from a non-GUI shell. No Developer ID is required.

The supported profile remains arm64 with a macOS 15.0 deployment target. Normal
builds obtain JUCE 8.0.15 from commit
`91ad83ae34a81e0833b1a2b0866f54846370ae53`, with a pinned archive SHA-256 checked
by CMake. An explicit `JUCE_DIR` override must match that clean revision; an
unmatched experimental override requires `NITRIDE_ALLOW_UNPINNED_JUCE=ON`.
The dependency no longer implicitly discovers the sibling Carbide checkout.

## Preserved regression evidence

`Source/Tests/Fixtures/Audio/` contains eight lossless interleaved float captures:
pad, keys, lead, hit, stressed network, combined Glide/Autobend, Pitch + Tone and
legato. Each is four seconds of stereo at 48 kHz, captured from the approved
production DSP. The manifest includes full physical patch values, sample-positioned
MIDI, compiler/OS provenance and the frozen 28-control host contract.

Ordinary checks read these immutable inputs. A baseline update is an explicit
reviewed operation, with a required producer revision; validation never regenerates
expected sound. Candidate diagnostics include exact hashes, maximum sample error
and RMS error. Current portable numerical limits are 1e-6 maximum and 1e-7 RMS;
supported toolchain changes that exceed these require investigation rather than
automatic acceptance. All eight were sample-exact in both local build configurations.

`Source/Tests/Fixtures/Compatibility/` contains actual state/preset outputs produced
by `0235b4b`, `968750d` and the approved 0.3.0 producer. Tests load historical
processor states through the public state API, verify host-bank synchronization,
reference/history/portable preset recall, and freeze the legacy mono migration:
All / Classic / 25 ms Glide, Autobend off. These complement the existing synthetic
malformed-state tests and analytic DSP tests.

Installed AU checks now require every frozen 0.3.0 parameter name and numeric ID,
in addition to the component identity and parameter count. Future controls can
append to the contract; existing IDs/positions/meanings remain protected.

## Editor/audio ownership boundary

`EditorMidiBridge` is a fixed-capacity single-producer/single-consumer command ring:

- Message-thread editor input is separate from all 16 host MIDI channels.
- Capacity is 1,024 commands; each block drains at most 256.
- Editor commands are applied at the next block boundary; host MIDI keeps its
  original sample offsets and wins equal-offset ordering.
- Closing/replacing the editor changes its epoch. The render owner releases editor
  notes and rejects stale queued commands while preserving host-held notes.
- Overflow increments a reset epoch instead of silently losing a note-off. Recovery
  clears editor ownership, leaving host ownership intact.
- Audio reset and fresh editor input have an explicit generation acknowledgement,
  so a polling tick cannot erase a fresh note's eventual release.

`MidiKeyboardState` and its listeners are now message-thread input objects only.
The renderer never processes events through them. Atomic host held/sounding masks
provide key display and Hold capture information; `InstrumentKeyboard` draws the
combined display without injecting host events into editor input state.

This fixes the concrete shared-channel failure in which a host channel-16 note-off
cleared the UI keyboard bit and caused a later editor Hold release to be suppressed.
Tests cover channels 1, 15 and 16, capture, on-screen keyboard teardown, overflow,
host-note preservation, and fresh input after reset/reprepare.

## Recall and callback contracts

UI control writes carry the restore generation to the model boundary. Grouped
field/envelope/XY updates validate once under the document transaction lock. State
serialization/restore and APVTS bank synchronization share that non-render
transaction boundary, preventing a completed recall from being overwritten by an
old UI gesture between its visual check and model write.

Host scalar listeners continue publishing independent atomics without the document
lock. Automation remains per-control; the bank is not globally transactional across
arbitrary simultaneous host writers. Non-render state methods may overlap rendering
but are not allocation-free real-time APIs.

Program/parameter notifications still use JUCE/APVTS's listener and host callback
mechanisms. The safety harness checks immediate program readback and automation
after selection. This pass retains those host semantics rather than replaying
deferred parameter values over newer automation. A fully coherent multi-writer
program-bank publication protocol remains a separately scoped follow-up.

## Real-time instrumentation and diagnostic coverage

The safety executable links a local allocation-interposer library. It marks render
scopes and counts malloc/calloc/realloc/free/aligned allocation activity on that
thread, including C++ allocations that use the malloc family. A deliberate
allocation/deallocation self-test must be detected before any results are accepted.
The instrumented cold-note, lifetime, overflow and active concurrency renders
reported **zero allocation activity** inside the tested `processBlock` scopes.

The concurrency test renders an active voice while the message thread performs
actual APVTS writes, editor creation/destruction and state restore. It explicitly
rejects stale individual/grouped edits after completed recall. ASan and TSan run
this same focused scenario with the allocation interposer disabled, avoiding
allocator instrumentation conflicts. Both diagnostic profiles pass. ASan leak
checking is disabled for the system/JUCE process-lifetime environment; memory
errors remain fatal.

Framework callback scopes are measured separately:

- First JUCE parameter-notification callback: **one allocation**.
- After active notifications: **zero allocations** in the measured callback.
- JUCE parameter notifications, AU wrapper MIDI/callback synchronization and the
  standalone MIDI collector still have framework locking paths.

Therefore the measured result is allocation-free Nitride rendering for the tested
scenarios—not a claim that every host/framework callback is lock-free or that
shared-runner/offline timing establishes universal real-time safety.

## Production ownership

- `InstrumentPanel.cpp`, `SupportingControls.h`, `NativeGestureControls.h`,
  `PitchMotionPanel.*` and `InstrumentKeyboard.h` own normal production components.
  Product compilation no longer includes study `.inc` implementations or selects
  a `NITRIDE_HOST_VIEW` branch. The shared `InstrumentView` remains the facade.
- `Source/DSP/Engine.*` and the `nitride_dsp` target own the shared implementation.
  `EngineData.h` provides lightweight snapshot types without including the engine
  in the session. DSP algorithms and arithmetic were preserved.
- Historical studies use a compatibility header/target alias. Their existing
  treatment and phrase APIs remain available; deeper scheduler/treatment
  decomposition can accompany later feature work.
- All automated product tests have isolated preset roots.

The native layout, names/accessibility, gesture workflow and async SafePointer
callbacks were retained and checked through interaction tests and a native image.

## Quantified verification

Local clean Debug and Release profiles each pass **six suites**. ASan and TSan
focused checks, native interaction, installed-AU validation/state/MIDI checks and
device playback pass. Playback with Glide/Autobend reported **zero xruns and
numerical faults**, and completed note release.

The 32-case Release benchmark comparison against the pre-hardening pitch build
retained **all 32 exact audio fingerprints**. On the M4 Max, three passes × two
audio seconds per case:

| Audio-budget occupancy | Before | After |
| --- | ---: | ---: |
| Lead / 48 kHz / 64 frames | 3.80% | 3.83% |
| Pad / 4 voices / 48 kHz / 64 frames | 14.57% | 14.63% |
| Pad / 12 voices / 48 kHz / 64 frames | 45.20% | 45.51% |
| Stress / 12 voices / 48 kHz / 64 frames | 48.40% | 48.32% |
| Stress / 12 voices / 96 kHz / 64 frames | 94.85% | 94.38% |
| Idle / 48 kHz / 64 frames | 0.33% | 0.37% |

Non-idle relative changes stayed within roughly 1% in this run. Publishing key
display masks adds a small idle cost (0.04 percentage points at 48 kHz / 64),
which is a larger relative percentage only because idle work is tiny. Timing
variation is not interpreted as an optimization win. The known 96 kHz maximum
stress/headroom limitation remains; hardening does not change that performance goal.

Raw local artifacts: `build/checks/hardening-performance.json`, profile logs,
`environment.json`, benchmark smoke reports and `native-hardening.png`.
