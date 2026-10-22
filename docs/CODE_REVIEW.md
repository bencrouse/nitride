# Systematic code review

**Date:** 2026-10-12  
**Revision:** `3cb32d1b0afa5669b2b020901dec04fc251b255e` (clean working tree before review)  
**Focus:** correctness, real-time behavior, performance, state/preset integrity and integration.

## Summary

Seven findings: **one high-priority performance limitation, three medium-priority correctness issues, and three lower-priority issues**. The performance limitation is already documented; this review independently reproduces it. No production code or preserved fixtures were changed.

| ID | Priority | Finding |
| --- | --- | --- |
| F1 | P1 | Advanced 12-voice rendering exceeds the 96 kHz real-time budget |
| F2 | P2 | All Sound Off and All Notes Off incorrectly share release behavior |
| F3 | P2 | Preset selection neither invalidates old gestures nor isolates their undo history |
| F4 | P2 | Accessible string entry misinterprets milliseconds and percentages |
| F5 | P3 | UTF-8 literals enter JUCE's ASCII constructor, corrupting text |
| F6 | P3 | Replacing an active Compare reference leaves audio on the old reference |
| F7 | P3 | A main-thread failure in the concurrency test can hang during cleanup |

P1 = address before claiming production support for the affected workload; P2 = normal correctness fix; P3 = narrower impact or development-tool issue. Findings are not assertions that every supported configuration is affected.

## Scope and method

Reviewed the active processor/editor, production `Source/Instrument` components, shared `Source/DSP` implementation, parameter metadata, preset serialization, MIDI bridge, CMake/Makefile/check workflow, regression/safety/performance harnesses, and relevant architecture/feature contracts. Historical study applications and archival capture code received a secondary inspection, not the same depth as the active product.

The review followed these boundaries:

1. Host audio/MIDI ingress, event ordering, ownership, sustain, voice lifetime and channel-mode messages.
2. Sample-clock pitch, FM, envelopes, modulation, filtering, effects and hot-loop work.
3. Render/message-thread communication, allocations, reset generations and publication.
4. Host automation, preset selection, Compare, scoped history and state restore.
5. Native UI lifecycle, callbacks, control parsing and displayed/model consistency.
6. Build/test coverage and measured deadline headroom.

Focused probes link the **actual built processor library** and existing JUCE test objects. Their source, runner and output are in `build/code-review/`; they do not replace the existing tests.

## Findings

### F1 — P1: Advanced maximum-polyphony rendering cannot meet the 96 kHz deadline

**Locations:** `Source/DSP/Engine.cpp:254–295, 657–728, 818–924`; workload definitions in `Source/Tests/PerformanceHarness.cpp:55–64`.  
**Evidence:** measured; already acknowledged in `docs/FM.md:218–225`.

At 96 kHz and 64 frames, each block has **666.67 µs** available. On the local Apple M4 Max, Release rendering of the supported 12-voice advanced cases consistently requires more than this even without an editor, device or host-wrapper overhead:

| Workload | Median pass load | p99 block time | Over-budget blocks |
| --- | ---: | ---: | ---: |
| `shape12-off` | 95.58% | 679.21 µs | 272 / 9,000 |
| `shape12-on` | 109.41% | 773.00 µs | 8,922 / 9,000 |
| `fm-combined12-off` | 96.66% | 699.96 µs | 1,728 / 9,000 |
| `fm-combined12-on` | 116.03% | 846.21 µs | 8,869 / 9,000 |

**Impact:** these sounds lack real-time headroom at 96 kHz, including on a high-end machine. Increasing buffer size alone cannot resolve steady render occupancy above 100%. These are offline deadline overruns, not measured device xruns, but they establish that the current algorithm cannot sustainably render these workloads within their budget.

At **48 kHz / 64**, the same advanced shape and combined cases used **55.70%** and **59.70%**, respectively; every case in that 48 kHz matrix had zero deadline overruns. Four-voice integration at 96 kHz used 31.14%, also with no overruns.

**Recommendation:** retain the explicit 48 kHz/fewer-voices guidance, define hardware and sample-rate acceptance targets, and optimize before claiming maximum-polyphony 96 kHz support. Keep sound-preserving optimizations separate from any intentional quality/polyphony tradeoff. Require repeated measurements and unchanged fingerprints where exact preservation is promised.

**Artifacts:** `build/code-review/fm-96k.json`, `build/code-review/fm-48k.json`.

### F2 — P2: MIDI panic and note-release messages have different semantics but use the same handler

**Locations:** `Source/Instrument/InstrumentRenderer.cpp:96–110, 171–186`; `Source/DSP/Engine.cpp:455–470`.  
**Evidence:** two focused runtime reproductions.

`isAllNotesOff()` (CC123) and `isAllSoundOff()` (CC120) both call `releaseChannel()`. That function clears sustain/ownership and invokes `engine.noteOff()`, which starts the normal amplitude release rather than terminating the voice.

This produces two incorrect behaviors:

- **CC120 does not silence the oscillator immediately.** With an eight-second amplitude release and Space mix zero, one second after All Sound Off the probe still reports **one active voice and peak 0.0891242**. The sound is not merely an effect tail.
- **CC123 overrides a held sustain pedal.** A pedal-held note with a 20 ms release becomes inactive after CC123 even though no pedal-up was sent. All Notes Off should release keys through the normal sustain policy, not forcibly turn sustain off.

**Impact:** host panic can leave a clearly audible release for seconds; host channel-mode note cleanup can prematurely cut sustained notes. Existing tests check eventual cleanup, not these distinct semantics.

**Recommendation:** implement separate owner-aware paths: CC123 releases physical keys while honoring sustain; CC120 terminates the affected sounding voices without their ordinary release. Preserve other host/editor owners on the same pitch. Decide/document the post-mix effect-tail policy separately from oscillator termination.

**Regression tests:** CC120 with maximum release and effects disabled; CC123 with pedal held; channel isolation; simultaneous host/editor ownership; sample-offset behavior.

### F3 — P2: Preset recall can be overwritten by a pre-existing gesture and contaminate Undo

**Locations:** `Source/Instrument/InstrumentSession.cpp:102–112, 147–183, 269–277`; contrasting state/program invalidation at `203–212, 341–357`.  
**Evidence:** focused runtime reproduction.

Generation-checked writes are effective for state restore and host program changes, which increment `restoreEpoch`. However, `selectPreset()` changes the patch without ending the existing edit or incrementing that epoch. `keepPatch()` and `importPreset()` also reach this selection path.

Reproduction using the session API:

```cpp
const auto generation = session.restoreEpoch.load();
session.beginEdit({ Nitride::hostIndex(Nitride::fm) });
session.set(Nitride::fm, 3.9);
session.selectPreset("factory:1"); // Glass keys: FM = 2.7
session.setIfCurrent(Nitride::fm, 0.123, generation);
session.endEdit({ Nitride::hostIndex(Nitride::fm) });
```

The obsolete write returns **true**, leaving the selected preset at FM **0.123**. The epoch stays unchanged. The first Undo then retains the Glass keys name but restores FM **1.65**, taken from the gesture's pre-recall Soft pad baseline. That is neither a clean undo of the selected preset nor an edit baseline from that preset.

**Impact:** an outstanding drag, Response timer, or asynchronous preset/import completion overlapping an edit can overwrite a recalled sound and produce confusing undo history. Ordinary mouse focus changes mitigate some UI paths, but the model boundary itself does not enforce the recall contract.

**Recommendation:** make non-host whole-patch recall a document transaction that finishes/cancels old edits, advances the restore generation and records exactly one coherent preset-selection history operation. Check whole-patch Undo/Redo and `apply()` for the same invalidation requirement; distinguish recall from deliberate in-gesture editing.

**Regression tests:** select/import a preset while individual/grouped/FM/Response edits are open; attempt old-generation writes before and after the UI polling tick; verify balanced host gestures and complete Undo/Redo states.

### F4 — P2: Accessible string setters do not parse the units they expose

**Locations:** `Source/Instrument/SupportingControls.h:43–47`; unit formatters in `Source/Instrument/InstrumentPanel.cpp:56–70` and `Source/Instrument/FmPanel.cpp:30–32`.  
**Evidence:** focused runtime reproduction.

`ValueControl` displays formatted physical values, but its accessibility string setter simply calls `setValue(v.getDoubleValue())`. It does not convert displayed milliseconds to seconds or displayed percentages to normalized values.

Observed through the real editor's accessibility interface:

- Amplitude attack `setValueAsString("100 ms")` stores **4 seconds**, the maximum, rather than 0.1 seconds.
- Amplitude sustain `setValueAsString("50%")` stores **1.0**, rather than 0.5.

The same generic implementation is reused for filter/FM timing and percentage controls. Numeric `setValue(double)` is not affected.

**Impact:** assistive clients setting a value using its exposed human-readable unit can make a large, unexpected sound change.

**Recommendation:** add a control-specific parser paired with `format` (or shared unit-aware format/parse helpers), while retaining the numeric API's documented physical units. `PitchMotionPanel.cpp:42–43` already demonstrates millisecond-aware parsing.

**Regression tests:** string/numeric entry equivalence for `100 ms`, `0.1 s`, `50%`, signed octaves, ratios and dB; round-trip each control's displayed value.

### F5 — P3: Non-ASCII literals corrupt native text and trigger Debug assertions

**Locations:** `Source/Instrument/MotionPanel.cpp:25`; `Source/Instrument/FmPanel.cpp:97`.  
**Evidence:** runtime tooltip inspection and JUCE assertion during editor construction.

The en dash in the Motion rate tooltip and curly apostrophe in the FM routing caption are UTF-8 source literals passed through implicit `juce::String(const char*)` construction. In this JUCE version that constructor expects ASCII, not UTF-8.

The probe triggers `juce_String.cpp:327`'s ASCII-validity assertion. The rate tooltip becomes:

```text
Independent free-running rate, 0.01â20 Hz. Double-click resets.
```

**Impact:** garbled tooltip/caption text and a debugger break during otherwise normal UI construction/painting. Passing CTest does not catch logged JUCE assertions.

**Recommendation:** use ASCII punctuation or explicit `juce::String::fromUTF8(...)` for these literals. Add a text-content check rather than relying solely on successful editor construction.

### F6 — P3: Changing an active Compare reference desynchronizes the view and audio

**Locations:** `Source/Instrument/InstrumentSession.cpp:242–250`; `Source/Instrument/InstrumentSession.h:76–77`.  
**Evidence:** focused API reproduction; limited current user-facing reachability.

`setCompareReference()` updates `reference` and the UI revision, but not the atomic `compareValues`/`compareMono` bank. That bank is populated only when Compare is enabled.

After enabling Compare on Soft pad and replacing the reference with Glass keys, `readVisible()` reports FM **2.7** while `audioValue(fm)` still returns **1.65**. Mono can disagree in the same way.

**Impact:** the public model API can display one compared sound while rendering another. The current shipped UI does not directly replace the reference while Compare stays enabled, so this is a latent API issue rather than a demonstrated normal-click workflow failure.

**Recommendation:** either close Compare when replacing its reference, or explicitly republish the audition bank with a defined publication policy. Test reference replacement while Compare is both on and off.

### F7 — P3: Concurrency test failures can turn into cleanup timeouts

**Locations:** `Source/Tests/RealtimeSafetyTests.cpp:78–100`.  
**Evidence:** control-flow/RAII analysis; no intentional hanging run was performed.

The render `std::jthread` loops on a separate `stop` atomic and ignores its stop token. `stop = true` is set only after the main-thread test loop. If a main-thread `require()` or another operation throws inside that loop, stack unwinding destroys the `jthread`. Its destructor requests stop and joins, but the render loop never observes that request.

**Impact:** a genuine test failure can hang until CTest's 120-second timeout, obscuring the original exception and slowing failure diagnosis. A render-thread exception does set the shared flag, so that particular path is different.

**Recommendation:** observe `std::stop_token` in the render loop or use exception-safe cleanup that sets the atomic and joins before rethrowing. Add a deliberate main-thread failure test that terminates promptly.

## Performance opportunities requiring separate measurement

These are source-backed optimization candidates, **not claims of a verified speedup**:

1. **Cache static advanced-FM derived data.** `Engine.cpp:254–282` interpolates 25 coefficients, computes harmonics and recomputes 50 sideband masks for every active advanced voice, every host sample, even in a sustained static shape/ratio sound. Cache on the exact dependencies: effective index/shape weights for coefficients, waveform shape for harmonics, and carrier/primary frequency for masks. Envelope advancement must remain sample-clock accurate.
2. **Avoid settled ratio transcendental work.** `Engine.cpp:840` evaluates logarithm/exponential ratio smoothing every sample, including when target and current are exactly equal. Consider an exact-equality fast path; do not substitute approximate smoothing without a reviewed sound change.
3. **Reduce inactive spectral bookkeeping.** `Engine.cpp:749–763` walks all 12 partials and wraps partial phases at every oversampled voice step even when only the expressive-network treatment is selected. Any lazy initialization must preserve transitions into the historical spectral treatment.
4. **Publish UI key masks only when ownership changes.** `InstrumentRenderer.cpp:308–322` scans thousands of ownership bits and writes 256 display atomics every block. An event-dirty path could reduce small-buffer/idle overhead. This is lower priority than oscillator work; existing documentation already measures its small absolute cost.

Do not disable the retained advanced phase representation just because parameters return to neutral: `docs/FM.md:49–53` explicitly requires held voices to retain that representation for phase continuity.

## Validation

Environment: **Apple M4 Max, macOS 27.0.1, Apple Clang 21.0.0, arm64, JUCE 8.0.15**.

| Check | Result |
| --- | --- |
| `python3 scripts/check.py --profile debug` | All six suites passed |
| Release build of the six test targets and `nitride_perf` | Passed |
| `ctest --preset local-release` | All six suites passed |
| `python3 scripts/check.py --profile asan` | Focused safety suite passed |
| `python3 scripts/check.py --profile tsan` | Focused safety suite passed |
| Focused production-linked review probes | Reproduced F2–F6 |
| Release FM matrix, 48 kHz / 64 frames | 16 cases, three passes; zero overdue blocks |
| Release FM matrix, 96 kHz / 64 frames | 16 cases, three passes; overruns summarized in F1 |

Each benchmark used **two audio seconds per case/pass**, **0.5 seconds warmup**, and ran separately from builds, test suites and the other benchmark. Measurements exclude editor, device scheduling and host/plugin-wrapper overhead; normal machine scheduling noise still applies. Raw reports include per-pass variation, voice counts and audio fingerprints.

Reproduction commands, from the repository root:

```sh
python3 build/code-review/run_probes.py
build/local-release/nitride_perf_artefacts/Release/nitride_perf \
  --fm-expansion --rate 96000 --block 64 --passes 3 --seconds 2 \
  --output "$PWD/build/code-review/fm-96k.json"
build/local-release/nitride_perf_artefacts/Release/nitride_perf \
  --fm-expansion --rate 48000 --block 64 --passes 3 --seconds 2 \
  --output "$PWD/build/code-review/fm-48k.json"
```

Review probe files and timing artifacts are **local, ignored build artifacts** and may disappear when cleaning `build/`. Findings and observed values are preserved in this report. Existing suite logs are under `build/checks/` and the build trees' `Testing/` directories.

### Coverage limits and existing safeguards

- No installed AU was replaced, no DAW session was opened, and no audio-device/system setting was changed. Installed-AU and actual device/host dropout behavior were not revalidated in this review.
- ASan/TSan ran the existing focused lifetime/concurrency safety suite, not every test or the entire benchmark matrix. ASan leak checking is disabled by the project workflow.
- This is not exhaustive fuzzing, minimum-hardware qualification, or a proof that all multi-writer parameter-bank observations are coherent. Independent scalar publication and framework locking/allocation limits are explicitly documented in `docs/HARDENING.md`; they are not presented here as newly discovered defects.
- Older serialized schemas and archival audio captures are deliberately not current compatibility gates. Their rejection is intentional, not a migration bug.
- The existing bounded editor queue, generation checking for host state/program recall, finite/range validation, temporary-file preset writes, and active render allocation audit are useful safeguards. Passing tests validate their covered scenarios, not the missing cases identified above.

## Suggested order of work

1. Add focused MIDI channel-mode and interrupted-preset-history regressions, then fix F2/F3.
2. Fix unit parsing and explicit UTF-8 handling (F4/F5).
3. Define the production performance envelope and measure exact hot-loop caching against F1.
4. Resolve the latent Compare API inconsistency and exception-safe concurrency-test shutdown (F6/F7).
