# Host-native presets / 0.7.46 · eval-046

The sixteen [approved filter revisions](../../docs/FACTORY_FILTER_AUDITIONS.md) now
replace their parents under the same clean names/categories. The library remains
**77 sounds: 16 Rage and 61 Classic**. Approved definitions/MIDI are not retuned.
Historical parents remain recallable through their old program numbers and the
standalone Evaluation archive. Local unchanged owned factory originals and exact
duplicate Study files are durably archived during the explicit revision update;
user edits cause a refusal and unrelated files remain untouched. No commit/push or
broader host/hardware/performance approval follows from this promotion.

The local eval-046 promotion is installed and registered, with all 77 clean native
recalls and all 132 historical programs / 92 controls verified. Restart Logic and
use the normal clean names, not the retired duplicate Study labels. The previous
factory files/receipt and exact retired Study files/receipts remain in the private
`evaluation/filters/factory-002-promotion/native-archive/` evidence folder.

All eight [motion/envelope presets](../../docs/MOTION_STUDIES.md) from eval-031 are now
explicitly promoted, unchanged, with clean names in both normal browsers. Only exact
owned Study duplicates are archived during the guarded local promotion; unrelated
files and user saves are never scanned for deletion.

All eight stronger-detune [analog r2 presets](../../docs/ANALOG_STUDIES.md) are now
explicitly finalized too, unchanged from eval-034. Original r1 stays unselected and
recallable; its comparison files are not retired with the exact r2 duplicates.

All eight more-distinct [backwards-instrument r2 presets](../../docs/REVERSE_STUDIES.md)
are now finalized too, unchanged from eval-037. Their original r1 comparison files
and receipt remain intact; only exact owned r2 Study duplicates are retired locally.

The finalized library has **77 sounds in nine musical-role categories**. In Logic,
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
  browsing contains the same 77 finalized sounds, plus user-kept sounds.
- Superseded revisions remain in standalone's **… → Evaluation archive**.
- All original starting patches, Classic Tines/Bass/Tubes and the FS1R-inspired trio
  are now in normal browsing. Existing startup/default conditions are unchanged.

The AU does **not advertise a Factory Presets list**. Logic otherwise adds a redundant
**AU Presets** submenu alongside the real category folders. Categorized `.aupreset`
files are the sole host-facing browser; the 61 non-replaced factory files remain byte-identical.
Historical numeric selection still works for compatibility, without advertising a list.
Names/categories are release metadata: original preset JSON, stored inner names and
sound definitions are not edited. An old project can still contain an archived name.

## Legacy sounds and session compatibility

No historical program was deleted or renumbered. All **132 program numbers**
(four starting patches plus 128 review revisions, including archived parents,
reverse/analog revisions, ten filter-character studies and sixteen promoted filter
revisions), legacy v7 and Rage v8 documents, undo/redo histories,
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
Budget Heaven and Sugar Teeth were explicitly finalized, bringing the catalog to53.
They use clean labels in Leads & Brass, Pads and Keys & Organs respectively, with
their auditioned sounds unchanged. Owned temporary Study duplicates are archived.
The user subsequently requested promotion of Late Glass, Rubber Relay, Gate Garden,
Paper Motor, Closing Argument, Undertint, Loose Change and Carousel Wire, then commit
and push. That brings the catalog to 61 without modifying any r1 recipe, MIDI or program.
The subsequent explicit approval of all eight stronger-detune analog r2 versions
brings it to 69. R2 is promoted without retuning; all original r1/r2 definitions,
MIDI and numeric programs remain unchanged.
The user then called all eight reverse r2 versions excellent and requested first-
class promotion, commit and push, bringing the catalog to 77 without retuning.
No mass normalization
or sound edits are part of consolidation. Inclusion/levels/save-edit acceptance is
not certification of every register, hardware reference or maximum-polyphony case.

## Safe export and installation

### Normal source installation (including a fresh clone)

```sh
make reload-au
```

This builds/installs the AU, then exports and verifies all 77 categorized `.aupreset`
files directly from that exact component and installs any missing files. Restart Logic
and create a new instance, then use the host's **Settings** menu above the interface.
The AU intentionally does not advertise the separate **AU Presets** submenu.

Only checked-in selection, acceptance and sound definitions are needed. No private
`evaluation/` journal, archived build, manual preset copying or separate package is required.
The native export helper is built even with `NITRIDE_BUILD_TESTS=OFF`. It loads the specified
bundle directly in an isolated process, avoiding an older component cached by AU discovery.
Every generated preset receives the same full-document, 92-parameter and stereo-audio
recall checks used by the checkpoint exporter before any preset is published.

Re-running the command adds only missing factory paths. **No existing path is replaced**,
including factory files edited in Logic or unrelated user saves. Existing ownership receipts
and old Study/Preview folders are also untouched. It reports how many files were installed
and how many paths were preserved; a preserved path is not a claim that its contents match
the factory sound. This additive installer does not claim ownership of existing files or
provide an automatic sound updater/uninstaller. New files use exclusive, staged publication;
on failure it rolls back only unchanged files newly created by that attempt.

### Explicit approved revision replacement

`scripts/factory_presets.py replace-factory PACKAGE --archive NEW-ARCHIVE` is separate
from normal installation. It requires the full current verified factory package,
the existing factory ownership receipt and one `--replace-file` for **every and only**
changed clean path. It refuses edited owned files, additions/removals, symbolic links
and non-current packages. The sixteen paths are the names in the approved filter guide.
The archive must be new and outside the native preset tree.

Only explicitly named `--retire-study factory-filters-r2-001` and
`--retire-study factory-filters-r3-001` receipts may be retired for this promotion.
Every owned Study file must be checksum-identical and have the exact full document
of a changed clean replacement. The previous sixteen factory files/receipt are saved
under `previous-factory/`, and the sixteen duplicate Study files/two receipts under
`retired-studies/`, before mutation. Failed publication restores moved originals;
a concurrent user save wins and conflicting moved originals are retained in the
archive. Existing user/other Study files are never swept for deletion.

Fresh clones get the promoted selections automatically. Existing installations keep
their files on a normal `make reload-au`; replacing an owned old factory revision is
an explicit versioned operation, never permission to overwrite a user-edited sound.

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
FS1R-only `--retire-studies`. For the motion/envelope batch, the named receipt is
`--retire-study motion-envelopes-001`. This explicit managed-catalog migration is
separate from the missing-only source installer, which never removes old Study files.
The finalized analog r2 receipt is `--retire-study analog-mood-002`; do not request
retirement of `analog-mood-001`, whose different r1 sounds remain for comparison.
The reverse r2 receipt is `--retire-study reverse-instruments-002`; preserve
`reverse-instruments-001` and its different r1 comparison sounds.
Explicit named-study retirement verifies the receipt's
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

`evaluation/preset-finalization/pass-008/` records the reverse r2 promotion: 77
exact native document/89-parameter/audio checks, actual registered 0.7.38 AU
validation, all 106 numeric-program checks, 14/14 Release and Debug suites, and
unchanged approved r2 render hashes. Prior 69 native files and original r1 comparison
files/receipt are unchanged; only exact owned reverse r2 Study duplicates are archived.

`evaluation/preset-finalization/pass-007/` records the analog r2 promotion: 69 exact
native document/89-parameter/audio checks, actual registered 0.7.35 AU validation,
all 90 numeric-program checks, 13/13 Release and Debug suites, and unchanged r2
audition render hashes. All prior 61 native files and r1 comparisons/receipt were
preserved; only exact owned r2 Study duplicates were archived outside the host tree.

See `evaluation/preset-finalization/pass-002/` for frozen root-category packages,
source/AU identity, generated adapter/metadata, native recall tests, installed receipts,
UI snapshots and validation. `pass-003/` records the menu-only correction: the native
FactoryPresets property is unsupported at both discovery and read, while all 60
historical programs, user metadata and existing categorized files still recall.
Prior checkpoints remain immutable. The native tests
distinguish valid AU custom/user preset number −1 from invalid positive factory numbers;
the initially mistaken negative-number assertion and its correction are preserved.

Promotion preserves every sound condition, DSP, voice policy, parameter ID and the
89-control contract. See [release qualification](../../docs/RELEASE_QUALIFICATION.md)
for the separately fixed Socket timing case and measured maximum-polyphony limits;
finalizing the sounds does not close the pending M2/host-soak/project-reopen gates.
The user explicitly authorized the motion promotions and subsequently the analog r2
promotions, including commit and push, on2026-10-30.
The reverse r2 approval explicitly authorizes first-class promotion, commit and push
on 2026-11-03; it does not extend the separate release/host qualifications above.
Private evidence remains local; ZIPs are not off-device backup.
