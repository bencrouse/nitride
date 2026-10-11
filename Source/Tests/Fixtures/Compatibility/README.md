# Historical compatibility inputs

`session-v1-*` were produced by `0235b4b`; `session-v2-*` and `preset-v1-*`
by `968750d`, using each revision's real serializer with pinned JUCE 8.0.15.
The fixture producer is `CaptureHistorical.cpp`, built only in detached historical
worktrees. Ordinary tests read these files and never regenerate them.

Both mono and poly sessions have FM depth 3.25 and master pitch +5. Version-2
sessions additionally contain a kept portable patch, reference/edited difference,
undo history and an active Compare audition. Recall must select the edited sound
with Compare off. Legacy mono migrates to All / Classic / 25 ms Glide; Autobend
remains off. Mono presets retain the original 0.025-second compatibility slide.
