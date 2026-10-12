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
