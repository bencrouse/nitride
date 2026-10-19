# Factory preset library

**Current workflow:** [Host-native presets / eval-029](HOST_PRESETS.md).
The user finalized **53 sounds**: the prior50 plus Plastic Hero, Budget Heaven and
Sugar Teeth, unchanged from the approved pop-lens audition. Editing/saving and levels
for the prior library are confirmed. Logic and the
standalone browser use the same nine categories. Superseded iterations stay archived
for recall, not duplicated in the release browser. Sound definitions and all66host
program numbers are preserved. Temporary owned Study labels are retired safely.

`candidate-001.json` remains release-facing metadata, not a replacement program table.
`acceptance.json` carries portable verbatim inclusion/host-flow feedback without the
private evaluation journal. `originals.json` pins inspectable snapshots of programs0–3;
their runtime source is still the unchanged C++ factory definitions. The legacy name
and numeric API remain supported without advertising a duplicate AU Presets list.

See [provenance and source boundaries](../../docs/PRESET_PROVENANCE.md). This selection
does not assert hardware fidelity or close the documented performance limitations.

**Everything below is historical pilot documentation**, not the current installation
layout, selection count, pending-decision list or instructions to execute now.

## Historical checkpoint001 preparation

The following describes the archived pilot, not the current rollout:

The AU will use the host's preset browser; standalone will retain a small browser.
**First confirm categorized browsing in Logic before removing either existing UI or
the legacy program table.** No new plugin binary or sound changes in this stage.

## Proposed selection, not another bank of revisions

`candidate-001.json` is a separate release-facing manifest: **40 retained families,
one preferred revision each, in nine proposed musical-role categories**. It references
the original stable IDs. It does not modify `presets/review/builtins.json`, any preset
JSON/MIDI/render, or any host program number. Historical definitions stay available
for exact recall. Tine Keys replaces the identical-sounding Cutoff Alloy entry in the
proposed browser, without changing either historical definition.

The latest Voice/Orch/Koto trio is direction-kept from actual user feedback. Earlier
Classic Tines/Bass/Tubes remain explicitly pending, not silently approved or discarded.
Original starting programs0–3 still await a usefulness/overlap decision. Categories
are proposed roles, not a claim that every texture or playing style has been audited.
Release counts/names remain subject to the final bounded collection audition.

## Installed Logic pilot

Only three presets are installed at:

```text
~/Library/Audio/Presets/nitride/nitride/Nitride Preview/
  Keys & Organs/
    Clear Stops.aupreset
    Tine Keys.aupreset
  Pads/
    Classic Voice.aupreset
```

1. In Logic, open a Nitride instance and its **host Settings/preset menu above the
   instrument interface**, not Nitride's internal `Review / evaluation` dropdown.
2. Look for **Nitride Preview → Keys & Organs / Pads**. Confirm the folders actually
   appear; slash-separated entries in the flat Factory Presets list do not count.
3. Select all three. Edit a parameter, use **Save Setting As** with a new name
   (do not overwrite a pilot), and reload it. In a disposable project, save/close/
   reopen to confirm recall. Native host Compare/navigation can also be checked.
4. Report whether both folders appear and load correctly, and your Logic version.
   If absent, report where Logic offers to save a Nitride setting; do not install
   duplicate copies in several guessed folders. The correct discovery location in
   the actual host is part of this checkpoint, not something offline tests prove.

No Logic project is automatically opened, captured, modified or uploaded. Audio
Units are exercised in isolated test processes, not in a running Logic project.
Actual Logic/project sample rate, buffer and monitoring settings remain unknown.

**The native selector/KEEP/legacy flat Factory Presets list deliberately still
exist in eval-023.** Only outer `.aupreset` labels are clean at this stage. Internal
Nitride patch names still show their archived `Review / … rN` names. Native export
does not rewrite the sound document merely to make the pilot look finished.

## Reproducible native export

```sh
make factory-selection
make factory-pilot FACTORY_OUTPUT="$PWD/build/factory-presets/a-new-pilot"
python3 scripts/factory_presets.py install-preview build/factory-presets/a-new-pilot
```

Export and install refuse existing destinations rather than replace anything.
`nitride_au_presets` is an offline tool linked only to Apple AudioToolbox/CoreFoundation,
not to Nitride DSP or the AU. It opens a fresh installed-AU instance for each export,
checks the original program/name, selects the factory voice, gets native `ClassInfo`,
and changes **only the outer host label**. No hand-built state is shipped. The Python
validator reads the XML envelope but never invents or rewrites production AU state.

Each preset is loaded through the native AU `ClassInfo` API over a different patch.
Tests require exact full-document bytes (including mono/chord, Compare reference and
identity), all 89 host parameters, and sample-exact stereo audio against a fresh original
factory program. The serialization probe is four seconds, 48 kHz/64 frames, a three-note
chord held for two seconds then released. This proves serialization, not good playing
range, maximum-polyphony performance, device-xrun safety, long-tail completion, hardware
accuracy, or actual Logic category navigation. Existing per-voice evidence is separate.

Factory files contain no Undo/Redo history or portable kept sounds. A frozen source
copy, source/hash/legacy-program mapping, installed AU binary/build identity, native
tool hash, exact verification results and file checksums accompany the package.
`verify` checks the files, clean state and original conditions before installation.

The full 40-voice candidate can be exported/verified offline with `--scope candidate`,
but the installer **refuses to publish it before the Logic checkpoint**. Packaging
is not release approval. The default install is a new owned `Nitride Preview` subfolder;
it never claims, moves, renames or deletes existing presets or user library files.

```sh
python3 scripts/factory_presets.py verify build/factory-presets/pilot-001
python3 scripts/factory_presets.py uninstall-preview
```

The last command is optional, not automatically run: it checks every installed hash
against the ownership receipt before deleting only those unchanged pilot files. It
refuses if a pilot was edited and leaves unrelated user additions/directories alone.
There is no forced uninstall or broad recursive removal.

## After host confirmation

1. Separate curated host visibility from historical recall. The stock JUCE AU wrapper
   currently exposes every program, so simply installing files does not hide retired
   iterations. Removing/renumbering historical entries is not a safe shortcut: current
   v7state decoding validates numeric programs, including entries in saved history.
   Design/test the visibility adapter and old-session path before changing enumeration.
2. Remove the AU's internal selector/KEEP/library-management UI, retain a minimal
   standalone browser, and provide export/migration for existing user-kept sounds.
   Decide Undo/Compare behavior separately; host Compare behavior must first be verified.
3. Apply release labels as metadata with compatibility, not edits to frozen recipes.
   Finish pending selections, the bounded role/overlap/level/register/velocity audition,
   and actual Logic save/edit/reopen testing. No mass normalization or extra synthesis.
4. Resolve or explicitly qualify F1combined96kHz and Socket's newer120BPM block-size
   limit. Earlier independent non-pitch/reference/perceived-pitch checks stay unresolved;
   this serialization pass does not pass them. Freeze/retest the release package, safe
   update/rollback behavior and backup. No commits/pushes without a separate request.
