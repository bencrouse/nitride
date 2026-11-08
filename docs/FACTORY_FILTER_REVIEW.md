# Factory preset filter review

Follow-up: the user requested continuation. The sixteen first-pass ideas now have
separate [audition revisions and three-way comparisons](FACTORY_FILTER_AUDITIONS.md),
not promoted factory replacements. The review below is the original recommendation.

Reviewed all **77 currently browsable factory presets**, including the four originals,
against their selected definitions, batch intent notes, revision history and current
eval-044 routing. Historical/unselected programs and the ten Rage Study presets are
not additional factory entries. Source of truth: [factory selection](../presets/factory/candidate-001.json),
[review registry](../presets/review/builtins.json) and each selected preset's `batch.json`.

**Recommendation: audition 16 targeted integrations, keep 8 conditional, leave 53
Classic.** This is a construction/intent review, not a listening verdict. No preset,
MIDI, program number, factory selection, installed native file or receipt was changed.
The proposed amounts below are audition ranges, not measured optimum settings or
instructions to promote these changes without hearing them.

## What counts as a useful integration

- **A specific musical job:** more resonant chewing, rounded brass articulation,
  dark sustained body, a wiry/papery edge or deliberate breakup. Not generic warmth
  on every patch, and not an obligation to use every character equally.
- **Independent articulation can be the main improvement.** Rage gives overlapping
  notes their own filter envelopes, tracking and velocity-dependent cutoff. Copper
  Lift and Reed Snap have especially good reasons to benefit beyond extra drive.
- **Changing family is not a transparent saturation insert.** Classic filters the
  summed, amplitude-shaped signal; Rage filters each voice before its final amplitude
  and velocity gain. Filter FM changes from the shared oscillator mix to each voice's
  source. Even zero-Rage family switching can change response, chord interaction and
  articulation. FM routes using the filter-envelope source also use each voice's
  filter envelope in Rage; Classic keeps the shared source. Global LFOs still do
  **not** become independently phased or note-reset oscillators.
- **Tape is not a master tape machine.** Its compression/history belongs to each
  pre-amplitude voice source. Do not promise velocity-scaled drive, compression of
  the complete amplitude envelope, wow/flutter or chord-bus glue. Existing detune
  and pitch motion still do their original jobs.
- **Protect contrasts and approved corrections.** Clean subs, exposed bell ratios,
  pristine keys, sparse ambient tails and patches deliberately revised to reduce
  buzz should not all acquire another nonlinear layer.
- **Protect CPU and playing behavior.** A small amount does not make Rage a cheap
  pad mode. Test overlapping releases/chords at 48 kHz; do not imply maximum-polyphony
  96 kHz qualification. Keep the present resonance initially, especially where filter
  FM or resonant mouth-like color is already important.

## First audition pass: 16 presets

All proposals start by changing only **family, character and amount**. Keep source FM,
detune, network, envelope times, resonance, pitch gestures, Space and MIDI fixed for
the first comparison. Retune another control only if the comparison identifies a
specific problem; do not hide a poor fit by rebuilding the whole preset.

| Preset | Character / amount to explore | Why it fits this construction | Preserve / reject if lost |
| --- | --- | --- | --- |
| **Rubber Relay** | **Diode, 40–65%** | Mono 550 Hz / 0.30-resonance filter, +2-octave short filter contour and continuing eighth-note FM motion. Give the existing chewing bass a more definite resonant bite. | Rounded low body, fixed carrier pitch and distinction between short notes and held chewing; no acid-like permanent squeal. |
| **Tension Core** | **Distortion, 30–50%** | Dirty mono bass already uses coupling 0.66, stress 0.17 and a short opening filter contour. The body-focused excitation is a better hypothesis than simply adding brighter stress. | Clear settled bass and legato glide. Existing network dirt makes overload/redundancy a real rejection risk; do not revive the removed onset burp. |
| **Brown Console** | **Tape, 30–50%** | Dark sustained organ surrogate, 2700 Hz cutoff, almost full sustain, 0.994 tap ratio and restrained drift/tremolo. Sustained rounded color suits the blunt, worn body. | Audible beating and tremolo. No Leslie/chorus or literal tape claim; no flattened, indistinct held clusters. |
| **Low Battery Piano** | **Tape, 25–40%** | Brief FM hammer, decaying body, 1.006 near-unison tap and velocity-sensitive Tone. Voice-local filtering can separate hard/soft overlapping keys; restrained rounding fits the worn impression. | Hammer/ring contrast, detune and quiet detail. Do not make the body uniformly fuzzy or describe the pre-amplitude detector as a velocity compressor. |
| **Copper Lift** | **Softclip, 30–50%** | Poly brass with 45 ms filter entrance, +1.8-octave contour and velocity opening. Independent filter articulation can keep a fresh stab from reopening held notes; rounded excitation suits brass body. | Bright entrance followed by solid held body; avoid a permanently dull or compressed swell. |
| **Bent Aerial** | **Transistor, 35–55%** | Narrow fuzzy mono lead, moderate network, 1.01 beating tap and slow drift. Asymmetric/edge-emphasized excitation can give the intended off-center edge a clearer identity. | Narrowness, readable intervals, beating and legato. Do not substitute a blanket distortion wash for its moving partials. |
| **Reed Snap** | **Softclip, 35–55%** | Dry poly stabs, 1 ms / 200 ms filter articulation, resonance 0.55 and strong velocity opening. A leading use of per-note envelopes, with rounding rather than still more high-frequency grit. | Snap and separated dyads; a new note must not swell every older note. Do not erase the triangle/shape refinement that already differentiated it. |
| **Melted Ensemble** | **Tape, 25–40%** | Dark 2400 Hz sustained pad with 1.007 triangle-rich tap and separate slow common/relative motion. One restrained pad application is enough to demonstrate sustained color. | Slow audible beat motion and dark exposed ensemble. Reject if compression masks it, chords smear, or CPU cost outweighs a negligible benefit. |
| **Reel Clav** | **Transistor, 25–45%** | Dry 280 ms woody key, brief filter/FM strike and slight 2.006 inharmonicity. A small edge contribution can make repeated figures tactile without making a long tail. | Woody body and empty rhythmic gaps; do not turn it into the papery Oxide Rim or Vanishing Clav. |
| **Taffy Coil** | **Diode, 35–55%** | Elastic poly material already combines opposed attack bend, resonance 0.50 and filter FM 0.40. Nonlinear resonance can reinforce rubbery articulation while each impact owns its filter. | Pitch-bend identity, punch and readable overlapping impacts. Extra resonance/FM is not needed; reject uncontrolled ringing or pitch ambiguity. |
| **Oxide Rim** | **Transistor, 20–40%** | A 130 ms papery clack with shaped FM, resonance 0.45 and filter FM 0.60. The edge-oriented character has a specific percussion role, unlike warming both kicks indiscriminately. | Short dry clack. Existing roughness may already be sufficient; reject longer ringing, excessive glassiness or loss of the transient. |
| **Pulse Lattice** | **Diode, 35–60%** | Fast poly plucks, low 900 Hz base cutoff, +2-octave filter contour and stepped FM color. Per-note resonant articulation can clarify played note sequences. | Short plucks and distinct stepped timbres. Preserve the MIDI and clock divisions; do not turn it into a sustained acid drone. |
| **Undertow Grid** | **Diode, 35–55%** | Mono held bed with 850 Hz cutoff, resonance 0.30 and dotted-eighth stepped cutoff against sixteenth AM. Useful counterpart to Pulse Lattice's independently articulated plucks. | Sustained bed, low weight and pulse gaps. Do not mistake pre-AM filtering for a new sequencer or LFO reset behavior. |
| **Paper Motor** | **Transistor, 30–50%** | Modest stepped Stress, smooth Coupling and a short FM tick already imply a papery motor. Edge color can make it tactile without maxing network stress. | Small scale and changing ticks, not a constant distortion blast. Preserve clock and stress contrast. |
| **Pressure Lift** | **Distortion, 35–60%** | Mono pressure riser already opens Tone/index over seconds while pitch climbs. Body-focused breakup can make the exposed source feel more strained as those existing contours open. | Quiet beginning, rising trajectory and clean handoff. Static Rage amount is not envelope modulation of drive; do not flatten the whole build. |
| **Shred Relay** | **Distortion, 45–70%** | Explicit signal-breakup brief: rising inharmonic FM/ratio motion inside a closing filter and steady sixteenth AM. This is an appropriate stronger feature example, not gratuitous dirt on a clean voice. | Audible progression rather than maximum breakup from onset; recognizable clock and dry endpoint. |

For a small first listening set, start with **Rubber Relay, Reed Snap, Bent Aerial,
Brown Console, Taffy Coil and Shred Relay**. These test resonant bass, poly articulation,
asymmetric lead edge, sustained rounding, elastic impacts and intentional breakup
without auditioning sixteen variations of the same musical role.

## Complete 77-preset disposition

**First** = include in the first audition pass above; **Conditional** = a plausible
experiment, not a recommended default yet; **Classic** = retain the current default.
The reasons below refer to actual construction, not just preset names.

### Bass — 9

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Crooked Velvet | **Classic** | r2 specifically reduced settled FM and restored pitch focus. Loping amplitude, modulator bend and carrier glide already supply identity; avoid obscuring the correction. |
| Classic Bass | **Classic** | Dry 1:1 FM utility; brief index/shape pop and very low resonance. Preserve the clean classic reference rather than make a second dirty bass. |
| Felt Weight | **Classic** | 300 Hz, low stress/FM, short damped envelope. Its rounded static pluck is the deliberate opposite of Tension Core. |
| Pocket Slap | **Conditional — Softclip, 20–35%** | Could round the short index pop, but the settled fundamental and concise stops already work. Reject if it just softens the essential slap. |
| Rubber Relay | **First — Diode** | Short opening Tone bite plus ongoing triangle FM is a natural chewing/resonant application. |
| Silt Bass | **Classic** | Restrained dry foundation, 0.996 tap and shallow drift. Its point is low-end steadiness, not another saturated/dynamically animated bass. |
| Sub Canvas | **Classic** | Zero coupling, stress, FM, routes and Space. Keep the near-sine sustained foundation clean. |
| Suction Bass | **Classic** | Reverse r2 deliberately removed FM/network bite for a smooth near-sine closing sub gulp. Nonlinearity would undo that contrast. |
| Tension Core | **First — Distortion** | Already-dirty mono source with opening filter bite; evaluate focused body, not more indiscriminate stress. |

### Keys & Organs — 11

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Afterimage Keys | **Classic** | Immediate FM strike followed by a delayed filter-envelope-to-FM-depth bloom. Preserve the two-stage harmonic story, not another attack/color layer. |
| Backspun Tines | **Classic** | Explicit clean, still, almost sine-like reference with a late fourth-harmonic edge; no grit/whoosh is part of the accepted brief. |
| Classic Tines | **Classic** | Dry electric-key reference with independent short shape strike and longer index decay; retain the exposed FM contour. |
| Glass Keys | **Classic** | Original bright 12000 Hz / high-FM reference. No strong filter-articulation problem that justifies replacing its established voice. |
| Brown Console | **First — Tape** | Dark sustained beating organ body gives rounded history-dependent color a clear job. |
| Late Glass | **Classic** | Slow FM attack turns a warm key glassy after the strike. Saturation could blur the intended delayed emergence. |
| Low Battery Piano | **First — Tape** | Worn decaying key plus velocity-sensitive Tone; test restraint and independent note articulation. |
| Clear Stops | **Classic** | Stable dry sustained organ with no cyclic motion and very low resonance; important clean counterpart to Brown Console. |
| Tine Keys | **Classic** | Its accepted EP identity is already resonance 0.50 plus substantial shared filter FM 2.50. Voice-local FM is a major redesign, not a harmless polish. |
| Sugar Teeth | **Conditional — Softclip, 20–35%** | Per-note velocity cutoff is attractive, but the 1:1 shaped-FM teeth must be brief and the body delicate. Continuous nonlinear color could destroy that contrast. |
| Vanishing Clav | **Classic** | The 110 ms high-ratio papery flick relies on filter FM 2.10 / resonance 0.62. Changing FM ownership risks replacing the accepted r2 texture. |

### Leads & Brass — 10

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Copper Lift | **First — Softclip** | Poly brass stabs/swells have a real independent-filter-articulation use case. |
| Bent Aerial | **First — Transistor** | Restrained fuzzy/unequal edge can complement the detuned, narrow lead without raising global Stress. |
| Closing Argument | **Conditional — Diode, 20–40%** | Resonant contracting vowel could gain definition; its mono negative filter contour already does most of the job. Preserve intelligibility, not just louder resonance. |
| Contact Lead | **Classic** | Original high-FM, coupling 0.75 / stress 0.40 mono lead already has a strong network signature. Extra nonlinear processing is more redundant than useful. |
| Glass Current | **Classic** | r2 explicitly lowered coupling/stress on a liked lead. Avoid adding back broadly similar roughness through another route. |
| Reed Snap | **First — Softclip** | Fast poly articulation and strong velocity-sensitive resonant Tone justify a per-note filter. |
| Rewind Reed | **Classic** | Intentionally rough r2 already has network 0.64 / stress 0.27, noninteger FM and a contracting vowel. More dirt risks erasing the index-falls/amplitude-rises gesture. |
| Replica Voice | **Conditional — Softclip, 15–30%** | Per-note contracting vowels could separate overlapping voices. But negative velocity response, FM routes and the carefully shortened colored tail must remain legible. |
| Whistle Stem | **Classic** | Low-coupling near-unison sine/tap whistle with mild vibrato; purity and softness are the job. |
| Plastic Hero | **Classic** | Explicit glossy melodic hook, not a gritty/resonant lead. Preserve clear held center, scoop and glide. |

### Pads — 9

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Budget Heaven | **Classic** | Exposed buzzy triangle-rich tap and cheap beating ensemble, not network distortion or polished warmth. Preserve the intentionally thin artificiality. |
| Melted Ensemble | **First — Tape** | A single dark, slow, sustained ensemble is a better rounding candidate than converting the entire pad category. |
| Classic Voice | **Classic** | Dry soft choir approximation with near-unison color and restrained motion. Preserve vowel clarity and the clean poly reference. |
| Folded Strings | **Classic** | Accepted r2 is a soft detuned pull with an ordinary Space tail and no bright index strike. New harmonics/compression could turn it into another harder reverse voice. |
| Soft Alloy | **Classic** | Chord helper, long release, active network/shape modulation and slow filter bloom already make a dense pad. Independent filtering has a high voice/CPU cost and risks changing its ensemble ownership. |
| Soft Pad | **Classic** | Preserve the original chord-helper pad as an unchanged reference and lower-cost option. |
| Stillwater | **Classic** | Patient near-unity harmonic space, long fades/releases and large Space; extra continuous color would obscure the low-stress contrast with Soft Alloy. |
| Undertint | **Classic** | Its story is luminous slow FM emergence and FM closing before the amplitude tail. Keep that spectral opening/closing exposed. |
| Velvet Pulse | **Conditional — Softclip, 15–30%** | Light rounding could make the AM pad denser, but its rhythmic amplitude contrast and existing network-shaped timbre should lead. Test release overlap and CPU first. |

### Plucks & Mallets — 12

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Classic Koto | **Classic** | Dry 4:1 nail/string FM reference. Preserve the short index strike and damped body without adding another source of grit. |
| Classic Tubes | **Classic** | Long 3.5:1 bell ring and slowly relaxing index. Extra nonlinear products would compromise the exposed inharmonic decay. |
| Carousel Wire | **Classic** | Ratio/operator-mix motion deliberately changes repeated plucks between hollow, wiry and rounded. Saturation risks making the changing spectra sound uniformly dense. |
| Jaw Twang | **Conditional — Diode, 15–30%** | Nonlinear resonance could reinforce a mouth-like twang, but resonance 0.72 is already central. Check peak movement/pitch and the fixed-register vowel before adopting anything. |
| Vowel Pick | **Conditional — Softclip, 15–30%** | Per-note vowel articulation could help overlapping picked notes; existing resonance 0.63 and routed index contours make the timbral identity fragile. |
| Hollow Bronze | **Classic** | r2 deliberately reduced settled FM sideband density while keeping the strike. Do not rebuild the removed buzz with filter nonlinearity. |
| Orch Chime | **Classic** | High 6.28-ratio chime and delayed index contour; preserve the distinct clang-to-held-body transition. |
| Rosewood Tap | **Classic** | Stable pitched wooden mallet, brief FM strike and quick damping. Keeping this understated provides a useful contrast to the harder clav/elastic candidates. |
| Reel Clav | **First — Transistor** | Brief, woody, dry rhythmic key has room for a controlled attack-edge identity. |
| Spool Guitar | **Classic** | Half-ratio shaped FM and closing resonance already provide the accepted r2 nasal/choked twang. Do not homogenize it with a brighter driven pluck. |
| Unstruck Bell | **Classic** | Thin high 7.13-ratio inharmonic ring is the entire r2 contrast. New low/even/odd products could destroy the thinness. |
| Taffy Coil | **First — Diode** | Elastic resonant impacts and opposed bends are a strong musical nonlinear-filter use case; independent voices add value. |

### Drums & Percussion — 9

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Bowl Toms | **Classic** | r2 corrected too much tone/not enough attack using an almost bare body and brief FM/pitch strike. Do not extend resonant tone or soften the attack again. |
| Floor Pulse | **Classic** | Praised dry rounded low kick with an almost bare body. Preserve the low fundamental/decay rather than applying automatic drum saturation. |
| Hard Hit | **Classic** | Original high-coupling/high-stress impact already supplies a hard network alternative. More distortion is redundant. |
| Inward Tom | **Classic** | Accepted compact rising-pitch suction/rough-endpoint gesture. Keep amplitude/Autobend trajectory and short dry endpoint unblurred. |
| Loose Change | **Classic** | Short metallic knock varies through deterministic global FM steps. Retain the variation instead of smoothing every hit toward the same noisy clack. |
| Oxide Rim | **First — Transistor** | Short papery/inharmonic edge is a specific use for asymmetry/pre-emphasis, subject to a strict transient/ringing check. |
| Rivet Kick | **Classic** | Already praised compact synthetic front with short shaped-FM bite. Preserve its distinction from Floor Pulse rather than changing both kicks. |
| Skin Knock | **Classic** | Short membrane-like pitch relaxation and velocity-sensitive Tone already supply tactile contrast. Keep one clean, punchy material reference. |
| Slack Drum | **Conditional — Tape, 15–30%** | Rounded source-history color might suit the loose drum, but must not mask the long 1.017-ratio beating body or turn it into a tighter generic tom. |

### Sequences — 5

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Buzz Loom | **Classic** | Fast clock-relative AM sidebands are the distinguishing feature. Keep a clean rhythmic counterexample rather than over-coloring every sequence. |
| Gate Garden | **Classic** | A held chord revealed by slow shared filter attack and crossed clock divisions. Per-note filter entrances change the collective-opening construction; keep that behavior unless a separate redesign is requested. |
| Pulse Lattice | **First — Diode** | Individually articulated resonant plucks can clarify the played-note rhythm and stepped harmonic motion. |
| Socket Dance | **Classic** | Useful accepted basic chop anchor; r2 only shortened amplitude release. Preserve clock/gap behavior and the reference against which the new driven sequences are judged. |
| Undertow Grid | **First — Diode** | Low sustained mono pulses with stepped cutoff make a complementary resonant held-note application. |

### Textures — 5

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Fault Bloom | **Classic** | Strong stressed network and activity-driven ratio already provide continuous nonlinear bloom. More filter distortion risks redundancy and hides the network behavior. |
| Orbit Shards | **Classic** | Deliberately low-stress poly glints and diffused tails contrast Fault Bloom. Preserve inharmonic separation and the lightness of overlapping notes. |
| Overtone Lantern | **Classic** | Exposed fifth-ratio operator at mix 0.86 determines implied pitch. Extra nonlinear products could change the core identity, not merely enhance it. |
| Paper Motor | **First — Transistor** | Controlled papery edge can complement stepped network strain while preserving the motor's small-scale variation. |
| Tendon Drift | **Classic** | Long Pitch + Tone drag and intermittent stepped strain must remain supple, not become another constant-grit Fault Bloom. |

### FX & Transitions — 7

| Preset | Disposition | Construction / reason |
| --- | --- | --- |
| Brake Bloom | **Classic** | Activity-to-FM-depth braking is the special mechanism. Additional nonlinear dynamics could make that restraint harder to perceive; it is not missing a generic limiter/drive. |
| Flash Room | **Classic** | A short fixed excitation exposes an ordinary diffuse room tail. Changing source distortion changes the excitation reference, not the room feature. |
| Pressure Lift | **First — Distortion** | Existing opening FM/Tone and rising pitch give pressure-oriented body breakup a clear transition role. |
| Reverse Veil | **Classic** | Deliberately fixed source, no network/pitch/ratio motion; amplitude and Tone alone create the reverse-like build. Preserve this clear controlled alternative. |
| Shred Relay | **First — Distortion** | Increasing inharmonic signal breakup is explicitly the job, making this a suitable stronger showcase. |
| Sinkhole | **Classic** | Fast metallic entrance must fall into low weight and a clear spacious tail. Do not fill the tail with continuous harmonics and compromise the implosion. |
| Spectral Cut | **Classic** | Fixed bright source closing through Tone is the entire gesture. Added nonlinear excitation undermines the clean filter-only contrast. |

## How to turn the review into preset changes

1. Make **new audition revisions**, not edits to frozen sources or installed user/native
   presets. Retain historical program numbers and the current 77-entry selection until
   replacements are explicitly approved. Do not silently turn the Rage Study bank into
   additional factory sounds as part of this review.
2. Compare **A: current Classic**, **B: Rage with zero amount**, **C: proposed character
   and amount**, using the same original MIDI and held/retriggered/overlapping notes.
   B separates the family/routing change from nonlinear character. Loudness-match the
   listening copies; do not mistake extra level for better sound or use Output as
   though it controlled the pre-amplitude nonlinear drive.
3. Keep the preset's own playing contract: low/high registers, soft/hard velocities,
   legato where applicable, staggered poly entrances, release tails and exact rhythmic
   gaps. In chordal patches, specifically check that older notes are not re-articulated
   by a new key. Check existing filter FM and the changed ownership of filter-envelope
   FM routes separately.
4. Adopt only audible improvements that still sound like that preset. A failed audition
   stays Classic; sixteen is a shortlist, **not a conversion quota**. Favor clear useful
   contributions over tiny always-on amounts that merely decorate the UI.
5. Verify headroom, finite audio/faults, actual voice/release cost and exact native state
   recall for every accepted revision. Do not raise resonance or reduce quality/polyphony
   to make demonstrations more dramatic or benchmarks appear to pass.

No audio auditions, preset revisions, installation, factory promotion, commit or push
are claimed by this review.
