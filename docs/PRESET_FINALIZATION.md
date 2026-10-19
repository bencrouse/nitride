# Preset consolidation review

## Subsequent finalized pop trio

The user approved Plastic Hero, Budget Heaven and Sugar Teeth after the eval-028
audition and explicitly requested finalization/review/commit. Eval-029 expands the
curated catalog from50to53 without changing any sound definition or any of the
66historical programs. The generic named Study receipt is retired only after full
checksum/identity/status and exact sound-document checks; unrelated user settings
remain untouched. Regression tests cover successful named promotion, edited files,
wrong identities, missing exact replacements and receipt-failure rollback. No push
is implied. The following report preserves the earlier50sound consolidation context.

Promotion validation: all 53 actually installed native files pass exact full-document,
89-parameter and audio recall; AU validation and all 66 historical program/state/MIDI
checks pass. All prior 50 preset files and other user settings remain byte-identical.
The three former Study files and their receipt are archived outside the host preset
tree. Factory/extension regressions pass 42 tests; pop recipe/installer tests pass 5.
Clean staged-source Debug and Release builds each pass all nine core suites without
the private evaluation workspace. Seven historical research-evidence tests explicitly
skip there; the full local 41-test evaluation and optional 8-test FS1R suites also pass.
The original audition definitions and MIDI are unchanged. No DSP, schema or host-control
changes are included. The private evidence workspace and old untracked review report
are excluded from the commit.

## Scope

The selected catalog contains 50 sounds across nine categories: the 43 kept
directions, the earlier Classic Tines/Bass/Tubes trio and the four original patches.
Superseded revisions remain in the compatibility archive. The user confirmed
inclusion, edit/save workflow and levels, and authorized a commit, not a push.

## Review outcomes

- Historical programs 0–62, stored names/IDs, v6 preset/v7 session formats and the
  89-control host contract are preserved. No DSP or frozen recipe is changed.
- AU browsing is host-native. Standalone uses the same curated metadata. The
  generated checksum-pinned JUCE adapter suppresses duplicate factory-list discovery
  without removing legacy numeric selection or user-state handling.
- Native exports retain clean complete ClassInfo state, empty history/portable-kept
  lists and exact Compare references. The recall probe was corrected to restore
  original program 1 over a genuinely different program rather than itself.
- The explicit catalog extension only adds files when every prior managed file
  matches its receipt AND the new package byte-for-byte. User edits/collisions fail
  before publication. Exact owned Study duplicates and old receipts are archived,
  not broadly deleted. Publication/receipt fault injection exercises rollback.
- Portable acceptance replaces dependence on the private journal for catalog
  validation. The full journal, reference assets/engines, media and installed build
  archives stay local and ignored. Native sources/recipes need none of those assets.
- Frozen CRLF preset bytes are preserved with `.gitattributes`, not reformatted and
  rehashed. Original recipes remain checksum-verifiable across checkouts.
- Unrelated working changes in `Source/DSP/Lfo.h`, `Source/Tests/LfoTests.cpp` and
  `docs/CODE_REVIEW.md` are excluded. Clean-index validation excludes them too.

## Validation boundaries

Clean staged-source Debug and Release builds each passed all eight core suites.
The portable factory suite passed 38 tests. The evaluation suite ran 41 tests,
with seven private-evidence tests explicitly skipped in the clean export (34 ran).
The full local evaluation suite and opt-in eight-test FS1R suite also passed with
their preserved inputs. Installed AU validation and all 63 legacy program recalls
passed, including the 89-control/state/MIDI contract. Eight quick workload audio
fingerprints matched eval-026; timing variations are not claimed as a speedup.
All 50 installed categorized files subsequently passed exact full-document,
89-parameter and audio recall. The only edited factory file detected was the
user's Bowl Toms save test; it was restored only after explicit discard permission.
The three unchanged owned Study files/receipt were archived outside the preset tree.

The locally installed eval-027 preserves the pre-existing working-tree LFO code
used by eval-026. That unrelated implementation/test work is not part of this
commit; clean-index Debug/Release checks separately validate the committed source.

Core source can be exported from the Git index with no private `evaluation/` tree.
The portable factory suite includes generator checks for the final non-pitch,
classic and FS1R-inspired batches, original-program selection, metadata/compatibility,
exact-state packaging and non-destructive installation/extension tests.

Seven historical research-evidence checks explicitly skip when their private inputs
are absent. The separate eight-test FS1R source/installer suite requires the opt-in
local research workspace. Neither is presented as included hardware evidence.

Native export/recall checks cover complete document/89 parameters and sample-exact
stereo serialization at 48 kHz/64 frames over four seconds. These are not performance,
maximum-polyphony, long-tail or likeness scores. User edit/save confirmation does
not separately establish project-close/reopen. Historical F1 combined 96 kHz and
Socket's 120 BPM block-size limits remain documented, not silently fixed here.

See [host workflow](../presets/factory/HOST_PRESETS.md) and
[provenance](PRESET_PROVENANCE.md). Detailed machine-specific logs and immutable
source/AU/package receipts remain in the local finalization checkpoint.
