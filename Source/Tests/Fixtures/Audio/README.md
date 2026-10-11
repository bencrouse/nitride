# Approved audio references

Captured once from approved production DSP at `b22e504` (0.3.0), JUCE
`91ad83ae34a81e0833b1a2b0866f54846370ae53`, arm64 Release. `manifest.json`
records complete physical patch values, MIDI/sample timelines, toolchain and
the published control contract. `.f32` files are interleaved stereo little-endian
IEEE float samples at 48 kHz, four seconds each.

Normal checks consume these immutable files. They never regenerate expected
audio. Capture is an explicit developer operation requiring a listening/review
decision; `--capture-reviewed <directory> <producer-revision>` is not part of any
validation command and requires an explicit 40-character producer revision.
