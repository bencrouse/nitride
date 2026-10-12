# Code-review validation and fixes

Findings refer to the 2026-10-12 review of `3cb32d1`. Each finding is addressed
in its own commit. The original review report is left intact.

## F2 — confirmed and fixed

CC123 now releases physical keys while respecting sustain. CC120 terminates
the affected channel's voices, including voices already in amplitude release.
Voice origin is tracked without changing the synthesis equations. An editor
Hold capture can inherit its original voice when the host owner is silenced;
independent same-pitch voices on other channels are preserved without keeping
redundant panicked voices alive.

The shared decimator, Tone state and Space tail are not globally reset by a
channel panic: doing so would cut unrelated channels. CC120 is immediate
oscillator termination, not a guarantee of zero post-mix filter/effect history
at the event sample. Pedal state itself is preserved for MIDI channel-mode
messages; editor teardown still clears its controller/owner bookkeeping.

Regression coverage: long-release panic, pedal-held CC123, pedal-up, mono/poly,
channel isolation, same-pitch host/editor ownership, released voices and event
offsets in `ReviewRegressionTests.cpp`.

## F3 — confirmed and fixed

Preset selection and whole-patch `apply()` now finish outstanding edits under
the document transaction lock, invalidate their generation, and record a
separate coherent recall history entry. Keep/import share that selection path.
Undo/Redo also invalidate obsolete callbacks. Invalid preset IDs do not cancel
an edit. An interrupted edit remains its own history operation: Undo first
returns to the edited pre-selection document, then undoes that edit normally.

Regressions cover grouped FM/network/Response edits, stale individual/grouped
writes before a polling tick, complete identity/value Undo/Redo, balanced host
notifications, and Keep/import/apply interruption. Release session and existing
UI/state regressions pass.

## F4 — confirmed and fixed

Value controls now pair their formatter with an explicit physical-unit parser:
milliseconds/seconds, normalized percentages, carrier:modulator ratios, signed
octaves/semitones, index and dB. Bare numbers and numeric accessibility setters
retain physical units. Malformed/unknown-unit/non-finite strings are rejected
without changing sound or starting a host gesture. Route Amount parsing follows
the selected destination's unit.

Real-editor regressions exercise amplitude/filter/FM controls, numeric/string
equivalence, invalid input and parsing every value control's displayed text.

## F5 — confirmed and fixed

Motion tooltip and FM routing caption use ASCII punctuation. The FM caption
and accessible description share one tested string. A source-wide scan also
found/fixed an ASCII-constructor violation in the performance CLI's range error.
Real-editor checks compare the complete tooltip/description text, rather than
accepting successful construction with a logged JUCE assertion.

## F6 — confirmed and fixed

Replacing a Compare reference now ends active audition, leaving the view/audio
on the edited sound. The replacement is published on the next explicit Compare
enable. This avoids rewriting an active atomic audition bank piecemeal and does
not claim a new whole-bank publication guarantee. Pending host program selection
is synchronized first. On/off replacement tests include mono, all parameters,
and preservation of edited conditions.

## F7 — confirmed and fixed

The render `jthread` now observes its stop token. Normal cleanup requests stop
and joins; exceptional main-thread unwinding uses the same RAII stop/join path,
so the processor, MIDI/audio buffers and exception state outlive the worker.
A deliberate exception through the actual concurrency harness must retain its
original message and finish within ten seconds (well below CTest's timeout).
The normal render-thread exception is still captured and rethrown after join.

## F1 — confirmed; exact optimizations, residual qualification limit

The reviewed overload was reproduced on the Apple M4 Max: advanced triangle
used 108.47% and full twelve-voice integration used 114.63% of the 96 kHz / 64
audio budget, with 8,780 and 8,834 missed deadlines per 9,000 callbacks.

The following retain the original arithmetic/clock/voice count:

- Cache advanced FM coefficients by exact index, shape amount and shape weights;
  cache harmonics separately and masks by their actual frequency dependencies.
  Unity masks stay valid during pitch movement while the highest actual taper
  remains exactly one. Companion masks are computed once per host sample, not
  four times at the oversampled clock.
- Share the nine frustration-bias sin/cos pairs only between bit-identical
  stress/excitation inputs. Node offsets, links, torques and feedback still evolve
  independently for every voice at the original internal clock.
- Reuse the already-computed primary sine for the audible modulator tap.
- Skip log/exp smoothing only when current/target ratio bits are equal.
- Defer unused spectral-phase work, reconstructing the exact preceding inactive
  step on transition into the historical spectral treatment.
- Move historical delay arrays out of the hot voice record and clear each buffer
  before its first use, rather than clearing ~32 KiB of unused delay storage at
  every network-voice note-on. Historical delay behavior is not removed.

A diagnostic uncached/eager path verifies exact output under cache dependency
changes, spectral/delay treatment transitions, toggled caching and voice stealing.
The static-FM test requires only one coefficient/harmonic/mask rebuild over
24,000 host samples. No approximate trig, reduced oversampling, voice limiting
or quality downgrade is introduced. Held voices keep their advanced phase
representation when controls return to neutral.

### Acceptance policy

Development qualification is **specific to the tested Apple M4 Max / Release
configuration**, not a minimum-hardware or universal DAW guarantee:

- Target for claiming a workload qualified: median occupancy below 85%, p99
  below 95% of its callback deadline, and zero misses in repeated isolated runs.
- Continue to recommend 48 kHz for arbitrary twelve-voice advanced sounds.
- At 96 kHz, four-voice integration is the qualified regression workload; do not
  claim universal maximum-polyphony support based on a static shape-only case.
- The fully combined 96 kHz / 64 case still misses deadlines despite substantial
  exact speedups. **F1 is mitigated, not declared fully resolved.** It requires
  further profiling/optimization or a separately approved quality/polyphony policy.

Timing is run separately from builds, suites and playback. Golden factory,
historical study and benchmark fingerprints are checked, not regenerated.

### Final evidence (2026-10-12)

Apple M4 Max, macOS 27.0.1, Clang 21.0.0, JUCE 8.0.15, arm64 Release.
Every timing case used three passes, two audio seconds per pass and a 0.5-second
warmup. Final timing ran with no concurrent builds, suites or playback.

| Final workload | Rate / frames | Median occupancy | p99 | Overdue callbacks |
| --- | --- | ---: | ---: | ---: |
| Advanced triangle, 12 voices | 48 kHz / 64 | 40.64% | 573.08 µs | 0 / 4,500 |
| Full advanced integration, 12 voices | 48 kHz / 64 | 47.64% | 739.83 µs | 0 / 4,500 |
| Advanced triangle, 12 voices | 96 kHz / 64 | 79.42% | 558.71 µs | 0 / 9,000 |
| Full advanced integration, 4 voices | 96 kHz / 64 | 25.55% | 198.54 µs | 0 / 9,000 |
| Full advanced integration, 12 voices | 96 kHz / 64 | 91.32% | 684.46 µs | 255 / 9,000 |
| Full advanced integration, 12 voices | 96 kHz / 256 | 91.03% | 2,720.79 µs | 67 / 2,250 |

The fully combined 96 kHz deadlines are 666.67 µs / 2,666.67 µs, respectively.
Intermediate isolated repetitions also retained overruns (170–255 of 9,000 in
the latest two comparable 64-frame runs); this is not treated as a passing
maximum-polyphony qualification. Static triangle passed the final case, but that
does not extend qualification to arbitrary FM/network combinations or hardware.

All **64 advanced and 32 ordinary fingerprints** match the original 0.7.0
captures exactly. Against those captures, twelve-voice advanced triangle median
time fell **27.66%** at 96 kHz / 64, and full integration fell **21.84%**.
The original 0.7.0 full integration was 116.83% occupied; the separate review-fix
baseline was 114.63%. Differences between runs include scheduling variation.
Both final matrices had zero overdue callbacks at 48 kHz. The ordinary 96 kHz
retrigger case still recorded 69 / 9,000 overruns at 64 frames and 3 / 2,250 at
256; maximum-polyphony limitations are not confined to the advanced matrix.

Final local verification: six Debug and six Release suites, focused ASan/TSan
safety suites, benchmark smoke, installed-AU validation and 89-control state/MIDI
checks passed. All thirteen native interaction checks passed. Active callback
allocation/deallocation guards and deliberate exceptional test cleanup passed.
App-local built-in-speaker playback could not be revalidated: three attempts
reported peak/CPU zero despite an open/playing device (one also reported an xrun).
This is recorded as a device/callback verification limitation, not a playback
pass. No system-wide device setting was changed.

Artifacts: `build/checks/review-f1-before-96k.json`, staged optimization reports,
`review-f1-final.json`, `review-f1-ordinary-final.json`, profile and installed-AU
logs. Baseline comparison files are the original `fm-expansion-final.json` and
`fm-expansion-neutral-final.json`. The original review report remains untouched.
