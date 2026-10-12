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
