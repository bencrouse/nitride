# Preset provenance and repository boundaries

The50selected sounds are original Nitride recipes within its existing architecture.
The catalog selects one preferred revision per family, including the original four
patches. Older revisions retain their names, IDs, host numbers and exact definitions
for session compatibility. Release labels/categories are separate metadata.

## Inspiration, not imported synthesis or endorsements

- Artist/producer lenses describe imagined creative briefs, not authentic settings,
  private intent, artist involvement or endorsements.
- DX7-inspired Classic voices and FS1R-inspired Whistle Stem/Jaw Twang/Vowel Pick are
  approximations, not Yamaha preset imports or exact reproductions. Native envelopes,
  ratio/shape/tap and the shared Tone filter substitute for richer source structures.
- In particular, Nitride does not implement the FS1R's independent voiced/unvoiced
  formant operators or FSeqs. Mukkuri/Fmt-Pluck were rejected as faithful-copy targets;
  their kept Nitride interpretations were accepted on their own musical merits.
- Offline MSFA and FSVR software references were not independently established to be
  hardware-exact; the VOICE1 MSFA reference has an explicit operator-AMS limitation.

## What is committed

Nitride's own source, preset JSON, authored MIDI/generators, immutable historical
registry, curated metadata, portable user acceptance and regression/installation
tools. The four inspectable original-program snapshots are frozen from Nitride's own
native AU state; they are not an additional runtime sound engine or renumbered bank.

The JUCE AU visibility/name adapter operates on a checksum-pinned generated copy of
the existing dependency. Vendor source/license notices remain intact in that copy;
no copied wrapper source is checked in. The adapter changes preset discovery/outer
names, not DSP or session state. JUCE's own licensing requirements still apply.

## What stays local and is not distributed

`evaluation/` is ignored in its entirety: the append-only private feedback journal,
machine paths/build manifests, renders, listening packets, downloaded reference code,
Yamaha factory bulk/ROM-derived data/tables/algorithms and reference binaries. Existing
assets are preserved, not deleted. No raw Yamaha data, FSVR/MSFA engine or associated
reference assets are linked or embedded in Nitride. Do not assume an upstream code
license grants rights to separately Yamaha-owned extracted data.

Core builds/tests and recipe verification require none of that workspace. Seven
historical research-evidence tests explicitly skip when their private inputs are
absent; the separate FS1R research suite is opt-in with
`-DNITRIDE_BUILD_LOCAL_EVALUATION_TESTS=ON` and requires its pinned local workspace.
These optional tests are not a claim that a fresh checkout has hardware evidence.

`acceptance.json` deliberately includes the narrow user statement needed to publish
the selected catalog, not the private conversation journal. Native export uses the
installed AU and a separately recorded local build; packaging a release checkpoint
is an explicit operation, never a download of reference assets or user-project capture.

## Acceptance boundaries

The user selected all current sounds and confirmed preset editing/saving and levels.
Project-close/reopen, independent perceived-pitch/listening qualifications, Socket's
120BPMblock-size case and the historical combined96kHzmaximum-polyphony F1 workload
are not silently passed by this decision or native serialization tests. Preserve
these limits when describing the library; no unrelated DSP fix is bundled here.
