# Host-native presets / 0.7.29 · eval-029

The finalized library has **53 sounds in nine musical-role categories**. In Logic,
use the host's Settings/preset menu above the instrument
interface. The temporary `Nitride Preview` parent is not part of the library:

```text
~/Library/Audio/Presets/nitride/nitride/
  Bass/
  Keys & Organs/
  Leads & Brass/
  Pads/
  Plucks & Mallets/
  Drums & Percussion/
  Sequences/
  Textures/
  FX & Transitions/
```

Restart Logic and create a fresh instance after updating the component; an existing
instance can retain the old code/interface. Save your own sounds using Logic's
**Save Setting As**, preferably with new names. Do not overwrite factory files.
The user confirmed categorized browsing, preset editing/saving and acceptable levels.
Actual Logic version/project/sample-rate/buffer/monitoring settings remain unknown;
project-close/reopen is not separately inferred from the editing/saving confirmation.

## AU and standalone

- **AU:** no internal factory dropdown or KEEP button. Load/save through the host.
  Undo, Compare, Hold, synthesis controls, automation and keyboard remain intact.
- **Standalone:** a small category-organized dropdown and KEEP remain. Normal
  browsing contains the same53finalizedsounds, plus user-kept sounds.
- Superseded revisions remain in standalone's **… → Evaluation archive**.
- All original starting patches, Classic Tines/Bass/Tubes and the FS1R-inspired trio
  are now in normal browsing. Existing startup/default conditions are unchanged.

The AU does **not advertise a Factory Presets list**. Logic otherwise adds a redundant
**AU Presets** submenu alongside the real category folders. Categorized `.aupreset`
files are the sole host-facing browser; the prior50files remain byte-identical.
Historical numeric selection still works for compatibility, without advertising a list.
Names/categories are release metadata: original preset JSON, stored inner names and
sound definitions are not edited. An old project can still contain an archived name.

## Legacy sounds and session compatibility

No historical program was deleted or renumbered. All **66 program numbers**
(four starting patches plus62reviewrevisions), v7 documents, undo/redo histories,
Compare references and portable kept sounds retain their existing storage/recall paths.
The AU adapter disables public enumeration and retains outer display labels. It operates
on a checksum-verified generated copy of the pinned JUCE wrapper, not the vendor
checkout, SDK, parameters, DSP or program-selection API.

Existing `.nitridepreset` files remain in
`~/Library/Application Support/nitride/Presets`; they are not moved, converted or erased.
In the AU, **… → Legacy preset migration** can load a legacy file or an existing kept
sound (including portable sounds restored from an old project). Then use the host's
**Save Setting As** to make a native preset. The migration submenu also exports the
current sound as `.nitridepreset`. Standalone retains its kept-sound browser and
import/export workflow. No automatic copying of personal presets into the factory
package or capture of a current Logic project takes place.

## Selection and remaining decisions

`candidate-001.json` is separate from the append-only evaluation registry. It chooses
one retained revision per family, gives clean names/roles and excludes obsolete
iterations. Tine Keys appears once rather than alongside its identical Cutoff Alloy
predecessor. Archived presets/MIDI/audio/feedback remain intact.

The user's explicit all-current-sounds inclusion decision closes the earlier classic
trio/original-four selection questions. The subsequently auditioned Plastic Hero,
Budget Heaven and Sugar Teeth are explicitly finalized, bringing the catalog to53.
They use clean labels in Leads & Brass, Pads and Keys & Organs respectively, with
their auditioned sounds unchanged. Owned temporary Study duplicates are archived.
No mass normalization
or sound edits are part of consolidation. Inclusion/levels/save-edit acceptance is
not certification of every register, hardware reference or maximum-polyphony case.

## Safe export and installation

### Normal source installation (including a fresh clone)

```sh
make reload-au
```

This builds/installs the AU, then exports and verifies all 53 categorized `.aupreset`
files directly from that exact component and installs any missing files. Restart Logic
and create a new instance, then use the host's **Settings** menu above the interface.
The AU intentionally does not advertise the separate **AU Presets** submenu.

Only checked-in selection, acceptance and sound definitions are needed. No private
`evaluation/` journal, archived build, manual preset copying or separate package is required.
The native export helper is built even with `NITRIDE_BUILD_TESTS=OFF`. It loads the specified
bundle directly in an isolated process, avoiding an older component cached by AU discovery.
Every generated preset receives the same full-document, 89-parameter and stereo-audio
recall checks used by the checkpoint exporter before any preset is published.

Re-running the command adds only missing factory paths. **No existing path is replaced**,
including factory files edited in Logic or unrelated user saves. Existing ownership receipts
and old Study/Preview folders are also untouched. It reports how many files were installed
and how many paths were preserved; a preserved path is not a claim that its contents match
the factory sound. This additive installer does not claim ownership of existing files or
provide an automatic sound updater/uninstaller. New files use exclusive, staged publication;
on failure it rolls back only unchanged files newly created by that attempt.

`BUILD_DIR` and `CONFIG` select the build as usual. `AU_INSTALL_DIR` selects the component
destination; `AU_PRESET_INSTALL_DIR` can select a different preset destination for testing.
`make install-au-presets` performs the build/install/export steps without the final `auval`
run; `make install-au` remains the component-only operation.

### Historical checkpoint workflow

The explicit checkpoint exporter below still defaults to a journal-recorded evaluation
build. For journal-free export to a new package, add `--component /path/to/nitride.component`
and `--tool /path/to/nitride_au_presets`. Normal source installs use the command above instead.

```sh
python3 scripts/factory_presets.py export /new/output/path --scope candidate
python3 scripts/factory_presets.py verify /new/output/path
make factory-install FACTORY_PACKAGE=/new/output/path
```

For an existing managed catalog, use the explicit additive extension:

```sh
python3 scripts/factory_presets.py extend-factory /new/output/path \
  --archive /new/local/receipt-and-study-archive --retire-studies
```

For the finalized pop trio, use `--retire-study ag-cook-001` instead of the legacy
FS1R-only `--retire-studies`. Explicit named-study retirement verifies the receipt's
identity/status, every owned checksum and full sound-document equality with a clean
replacement. It never scans for or removes unrelated files merely named Study.

Extension refuses edited factory files, changed/removed existing sounds and unowned
path collisions. Only exact owned Study duplicates are retired; their prior files
and receipts are archived outside the host preset tree. Fault-injected tests cover
publication/receipt failure and rollback. This is NOT a general sound-replacement
updater. User edits are never overwritten or claimed.

The offline exporter uses fresh native AU instances and `ClassInfo`, not hand-built
production state. It requires exact original conditions/Compare reference/identity,
empty factory undo/redo/portable-kept lists, all 89 host parameters and sample-exact
stereo audio against the historical factory program. The four-second 48 kHz/64-frame
serialization probe is not a long-tail, maximum-polyphony, device-xrun or musical-
likeness qualification. Factory output levels are unchanged, not newly normalized.

Top-level installation requires journaled actual user confirmation of the Logic
pilot. It stages complete files and publishes without replacing any existing path.
A receipt owns only the installed factory files. Identical reinstall is a no-op;
edited files, collisions and changed packages are refused rather than overwritten.
Fault-injected publication rollback removes only unchanged newly created files,
leaving unrelated user files intact. Future versioned update/rollback packaging
is intentionally limited to exact-preserving extension; there is no force-update switch.

The optional preview-removal command checks ownership and every original hash before
removing the three pilot files. It leaves unrelated user additions intact and refuses
if an owned pilot was edited. A remaining Preview folder may therefore contain a
protected user save, not an extra factory category.

## Evidence and open limits

See `evaluation/preset-finalization/pass-002/` for frozen root-category packages,
source/AU identity, generated adapter/metadata, native recall tests, installed receipts,
UI snapshots and validation. `pass-003/` records the menu-only correction: the native
FactoryPresets property is unsupported at both discovery and read, while all 60
historical programs, user metadata and existing categorized files still recall.
Prior checkpoints remain immutable. The native tests
distinguish valid AU custom/user preset number −1 from invalid positive factory numbers;
the initially mistaken negative-number assertion and its correction are preserved.

Audio fingerprints match eval-023; no sound conditions, DSP, voice policy, parameter
IDs or host-control count changed by catalog consolidation. F1 combined maximum-polyphony 96 kHz, Socket's
newer 120 BPM block-size limit and previous independent non-pitch/reference/perceived-
pitch checks remain open. The user authorized cleanup/review/commit on2026-10-19,
not a push. Private evidence remains local; ZIPs are not off-device backup.
