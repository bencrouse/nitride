# Release qualification 001 — internal candidate, not a release declaration

Candidate: 0.7.30/eval-030, Apple Silicon only, based on 30dd71e with the separately
fingerprinted uncommitted qualification changes. The 53 approved recipes/MIDI and
66 historical programs remain intact. No reduced-quality mode, voice cap, new preset,
schema or parameter contract is introduced. The public-release signing/licensing/
installer work remains separate from this qualification pass.

## Evidence boundaries

| Gap | Current evidence | Status |
| --- | --- | --- |
| Socket 120 BPM block partitioning | Unchanged Socket r2 / Buzz 120 MIDI; 118 control; 48/96 kHz; 64/257/1024 frames; exact repeat and seek/stop/resume | Passed locally on M4 Max; M2 pending |
| Current native AU and serialization | Registered 0.7.30, all 66 programs/89 controls/state/MIDI, all 53 installed full-document/control/audio recalls | Passed locally on M4 Max; M2 pending |
| Development regressions | 11/11 Debug and Release suites; safety + Socket under ASan and TSan (2/2 each) | Passed locally |
| M4 Max performance/headroom | Fixed 29-case matrix, 3 passes × 10 seconds, 2-second warmup; no concurrent builds | 23/29 passed the conservative offline screen; limitations below |
| Supplemental M4 performance | Same binary/settings, 12 additional ordinary 96 kHz and single-voice shape cases | 5/12 passed the conservative screen; limitations below |
| M2 execution | Pinned internal bundle and original MIDI/checklist supplied | Pending user execution/results |
| Actual Logic quit/project reopen | Disposable-project checklist, edited user setting and automation | Pending manual observation |
| Real Logic 30–60-minute soak | Multi-instance playback, transport, edits, bounce, note release | Pending manual observation |

Automated tests are not evidence that a person installed the app on another Mac,
reopened a Logic project or heard dropout-free live playback. No project or device
settings are automatically captured/changed, and no results are uploaded. Exact OS,
Logic, chip and RAM are recorded only as basic test metadata; no hostname or serial.

Recorded development configuration: Apple M4 Max, 36 GiB RAM, macOS 27.0.1
(26A434), installed Logic Pro 12.4 (6707). Reading Logic's application version is
not evidence that the manual Logic acceptance ran. The M2's exact configuration
remains pending. Native AU discovery was refreshed and validation rerun after an
initial stale 0.7.29 registration; only the explicit 0.7.30 rerun counts here.

## Timing correction

The committed S&H-only fix did NOT close the original Socket case: eval-029 still
reproduced maximum 64/257 error 0.000756569207 against the unchanged 1e-5 guard.
Socket's square amplitude LFO also has discontinuous half-cycle edges. Its positioned
phase now uses block sample offsets, and S&H/square edges canonicalize only within
1e-12 edge units, including rounding cancellation near a host loop origin. Genuine
pre-boundary positions remain protected by stopped/seek tests. Continuous sine,
triangle and saw paths and free-running clocks retain their existing arithmetic.

The original 48 kHz/120 BPM failure grid now has zero 64/257 and 64/1024 error. Across both
rates, the 118 control and seek/stop/resume variants, the largest observed error is
7.450580596923828e-9, below 1e-5; repeated same-block renders are exact. This is not a
universal bit-exact block-invariance claim. Initial diagnostic attempts/failures are
preserved locally. No acceptance threshold was weakened and no old preset was edited.
Some discontinuity-edge samples intentionally move to the correct musical boundary;
do not describe this bug fix as universal old-audio bit preservation.

## Performance gate and interpretation

`scripts/qualification_matrix.tsv` fixes 29 ordinary/advanced cases at 44.1/48/96 kHz and
64/128/256 frames. Each selected case runs 3 passes × 10 seconds after 2 seconds warmup.
`scripts/qualification.py` retains every raw report and refuses truncated matrices,
wrong settings/builds and non-finite results. A missing report remains unqualified.

The conservative offline screening gate is **p99 <=80% of deadline AND zero observed overdue
blocks**. This is headroom evidence for the measured workload/configuration, not
device xruns, host scheduling safety, minimum-hardware certification or a claim that
every preset/knob combination works there. Timing outliers are retained, not trimmed.
This 20% headroom margin is a conservative proposed screen, not a user-approved shipping
requirement. The historical 96 kHz advanced 12-voice limitation is NOT presumed fixed. If a profile
fails, document the actual limit rather than silently changing quality or polyphony.
Historical ordinary 96 kHz retrigger overruns are also a distinct open risk; passing
four held voices does not close maximum-polyphony note-dispatch qualification.
The handoff matrix is an initial screen, not a complete cross-product: it does not
include 96 kHz ordinary 12-voice/retrigger cases or every advanced 1-/4-/12-voice variant.
The additional ordinary cases were measured locally below; equivalent M2 supplemental
coverage remains pending. Dedicated four-voice shape-only and single-voice fully
combined variants are not in the existing harness and are not claimed as tested.

Run measurements only after compilation/sanitizers stop. Do not compare performance
while another build or live audio session is using the machine. The same binary hashes
and matrix are used on M4/M2; cross-machine audio bit equality is not required.

### M4 Max results — primary matrix

All 17 ordinary cases passed the conservative offline screen, including the 48 kHz
12-voice automation/retrigger workloads and the 44.1 kHz/128-frame spot checks.
Four-voice full advanced integration passed at both 48/96 kHz and both buffer sizes.
The advanced 12-voice results are more limited:

| Workload | Rate / frames | Median occupancy | p99 / deadline | Overdue callbacks |
| --- | --- | ---: | ---: | ---: |
| Advanced shape, 12 voices | 48 kHz / 64 | 42.48% | 47.35% | 8 / 22,500 |
| Full integration, 12 voices | 48 kHz / 64 | 49.19% | 57.18% | 3 / 22,500 |
| Advanced shape, 12 voices | 48 kHz / 256 | 41.68% | 43.49% | 0 / 5,625 |
| Full integration, 12 voices | 48 kHz / 256 | 49.14% | 56.81% | 0 / 5,625 |
| Advanced shape, 12 voices | 96 kHz / 64 | 81.33% | 88.94% | 16 / 45,000 |
| Full integration, 12 voices | 96 kHz / 64 | 93.59% | 105.77% | 4,170 / 45,000 |
| Advanced shape, 12 voices | 96 kHz / 256 | 81.03% | 87.84% | 12 / 11,250 |
| Full integration, 12 voices | 96 kHz / 256 | 93.41% | 105.13% | 878 / 11,250 |

The 48 kHz/64-frame shape/full-integration outliers remain failures of the zero-overdue
screen despite comfortable p99 values; they are not discarded or attributed to a
specific scheduling mechanism without evidence. The full 96 kHz workload has sustained
deadline trouble, not merely an isolated long block. Larger buffers did not qualify it.

**Conservative test recommendation:** start the manual multi-instance Logic checks at
48 kHz/256 frames. The measured 48 kHz/256 workloads passed, but they are not a guarantee
for every recipe, instance count, machine or device. Do not advertise arbitrary 12-voice
advanced operation at 96 kHz, or infer maximum-polyphony low-latency support from the
four-voice results. No quality reduction or voice cap was applied.

Three same-settings pre/post controls (ordinary lead, four-voice pad, and 96 kHz full
12-voice integration) retained exact audio hashes. That protects these unaffected paths;
it is not a claim of exact legacy audio at the intentionally corrected LFO edges.
Raw reports and failed cases remain preserved. Supplemental 96 kHz ordinary 12-voice/
retrigger and single-voice shape measurements are separate from this pinned matrix.

### M4 Max results — supplemental cases

The same binary and 3 × 10-second / 2-second-warmup settings were used, after the primary
run completed, with no concurrent builds. These are additional cases, not selective
reruns used to replace failures. Five of twelve passed the conservative screen:
single-voice shape at both rates/buffers, and the 96 kHz/256-frame held 12-voice pad.

| Ordinary 12-voice workload | Rate / frames | Median occupancy | p99 / deadline | Overdue callbacks |
| --- | --- | ---: | ---: | ---: |
| Held pad | 96 kHz / 64 | 71.36% | 78.29% | 3 / 45,000 |
| Stress | 96 kHz / 64 | 80.47% | 85.81% | 4 / 45,000 |
| Automation | 96 kHz / 64 | 78.00% | 85.09% | 30 / 45,000 |
| Retrigger | 96 kHz / 64 | 92.85% | 100.07% | 460 / 45,000 |
| Held pad | 96 kHz / 256 | 70.87% | 76.52% | 0 / 11,250 |
| Stress | 96 kHz / 256 | 80.34% | 86.58% | 7 / 11,250 |
| Automation | 96 kHz / 256 | 76.44% | 83.00% | 0 / 11,250 |
| Retrigger | 96 kHz / 256 | 92.60% | 97.75% | 20 / 11,250 |

The automation/256 case had no overdue blocks but missed the proposed 20% headroom
screen; distinguish that from an observed deadline miss. Single-voice shape p99 was
3.92–8.13% of deadline with no overdue blocks. In total, **28/41** measured cases passed
this conservative screen. Ordinary 96 kHz maximum-polyphony operation remains a risk
too, so the restriction must not be described as advanced-FM-only. This qualification
pass measures and documents the limits; it does not resolve them through optimization
or establish a user-approved shipping support policy.

## M2 handoff

`scripts/package_qualification.py` creates the internal ad-hoc-signed bundle. It checks
arm64 architecture and absence of unpackaged non-system runtime libraries. ZIP round-trip
checks verify content and signatures. This is **not notarized** and is not the public
installer. Report Gatekeeper blocks; do not disable security or re-sign the candidate.

The fresh-install helper refuses existing components/app/preset paths, records ownership,
and verifies all files before uninstalling. It removes only its own unchanged files,
leaves user saves/preferences alone and refuses edited files. Isolated fake-home tests
cover install/verify, collisions, edited-file refusal, user-save preservation, uninstall
and partial-install rollback. It does not reset the audio device or kill a DAW.

`Run Tests.command` needs no Python/compiler on the M2: it runs native AU/53-file checks,
Socket qualification and the fixed matrix, with raw JSON/status/logs in a unique results
folder. Completion is not a pass. Use native Logic (not Rosetta) for the advertised
Apple Silicon test profile, and confirm About shows 0.7.30/eval-030. Follow
[the manual checklist](QUALIFICATION_MANUAL.md); only explicit returned observations can
close the project-reopen and soak gates. The latest macOS on two machines does not
automatically qualify the binary's macOS 15 deployment target or every Apple Silicon chip.

Local immutable evidence is under ignored `evaluation/release-qualification/pass-001/`.
Private journals, research engines/Yamaha data, recordings and projects are not in the
handoff. No commit or push is inferred from permission to run qualification.
