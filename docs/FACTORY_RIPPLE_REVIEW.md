# Factory Ripple integration review

Reviewed **all 77 currently selected factory presets**, using their names, current
definitions, authored intent notes and revision history, against the gain-corrected
**0.7.49 / eval-049** implementation. Historical/unselected revisions are not extra
factory voices; the twelve Ripple Study revisions already exercise Ripple and are
not conversion candidates. The catalog is currently **61 Classic / 16 Rage**.

**Recommendation: 12 first auditions, 12 conditional experiments, 53 retain their
current family.** These are musical hypotheses, not rendered/listened approvals or
factory replacements. No preset, MIDI, selection, program, installed component,
native file or ownership receipt is changed by this review.

Sources: [current selection](../presets/factory/candidate-001.json),
[immutable registry](../presets/review/builtins.json), selected preset JSON and its
batch intent notes. For the sixteen batch-22 Rage revisions, the review also follows
the explicit parent back to its underlying musical brief; a short filter-revision
description is not treated as the whole sound's intent. Revision notes saying
“pending listening” are historical notes, not a reversal of later user approval.

## Selection criteria

- **Give phase cancellation a musical job.** Moving hollows, mouth-like articulation,
  wiry plucks, glinting partials, supple bodies and patterns through sustained sound
  are good hypotheses. Names alone are insufficient: Stillwater is a patient, quiet
  reference, not permission for a watery phaser; Sub Canvas needs a solid fundamental.
- **This is a replacement family, not a phaser insert after Tone.** Ripple removes
  the current Classic or Rage filtering while active. It does not retain a serial
  low-pass, and it cannot run the approved Rage character at the same time. Protect
  voices whose actual low-pass opening/closing or Rage breakup is essential.
- **Use existing spectral material and motion.** Prefer sources with useful partials
  in the phase-transition region. Begin with broad, lower-mid phase centers rather
  than copying high low-pass cutoffs/high resonance. Do not increase oscillator FM,
  add motion or turn up Drive merely to rescue a poor fit.
- **Keep phasing distinct from more distortion.** Most first-pass proposals start
  with Drive=0. Signed-feedback mode can sculpt the existing spectrum without
  waveshaping it. Subtle/Medium are sensible starting depths; Hard is not a quota.
- **Protect arrangement and corrections.** Maintain root readability, transients,
  two-stage FM stories, deliberate thinness, detune, rhythmic gaps and accepted
  reductions of buzz/colored tails. A more conspicuous effect is not automatically
  a more appropriate preset.
- **Respect ownership and cost.** Classic→Ripple moves filtering before each voice's
  amplitude envelope and makes filter-envelope FM sources voice-local. Existing
  routes can therefore sound different despite unchanged stored values. Global
  LFOs remain global, not stereo, independently phased or note-reset modulation.
  Long releases/chord helpers need particular voice-cost checks. The existing CPU,
  M2 and manual Logic/device-soak limits are not waived by this review.

## First audition pass — 12

The following settings describe **starting directions, not measured optimum values**.
For waveshaping mode, “linear” means Drive=0; use additive Dry to expose phasing.
For feedback, begin around magnitude .15–.5 rather than treating the near-full
feedback of a demonstration Study as a factory default. Match levels independently
of these choices; eval-049 makeup is not a guarantee of equal loudness or headroom.

| Preset / current family | Ripple's musical job | Starting direction | Must survive |
| --- | --- | --- | --- |
| **Late Glass — Classic** | Let the new FM partials acquire a glassy, hollow contour as they emerge, rather than dirtying the initial warm key. | Subtle, linear waveshaping; mostly stationary phase center through the later partials. Retain the existing shallow .11 Hz cutoff motion, not a new sweep. | Immediate warm strike followed by the separate slow FM bloom; no extra attack grit or cyclic wash that hides the delay. |
| **Closing Argument — Classic** | Make the already speaking, inward-folding lead more distinctly vowel-like. | Medium, restrained negative feedback; move the phase center downward with the existing negative envelope. | Readable speaking contour and held pitch. Reject if holes merely replace the original meaningful darkening with a bright, thin tail. |
| **Folded Strings — Classic** | Add gentle folded/hollow string color to the rich, near-unison triangle tap. | Subtle, linear waveshaping and balanced Dry; use the existing slow contour, not added vibrato/LFOs. | Soft swell, relative beating, ordinary room tail and distinction from the harder reverse voices. |
| **Velvet Pulse — Classic** | Turn the existing clocked cutoff movement into shifting spectral slots inside the amplitude pulses. | Medium, moderate signed feedback; compare both signs while keeping clock divisions and AM unchanged. | Velvet body, pulse depth and clear gaps. Audit filter-envelope FM-route ownership on overlapping notes. |
| **Carousel Wire — Classic** | Emphasize hollow/wiry differences as the existing ratio/operator-mix motion changes a repeated pluck. | Subtle/Medium, linear waveshaping; static or short-envelope phase color. No additional modulation lane. | The original alternation between rounded, hollow and wiry; do not make every step equally hollow or obscure the attack. |
| **Jaw Twang — Classic** | A fixed-register set of moving hollows is a credible alternative way to suggest a mouth cavity. | Medium, restrained negative feedback; retain zero key tracking and a low fixed-register phase region. | Low jaw-like articulation, stable played root and no pitch bend. This is still not a formant bank or faithful Mukkuri model. |
| **Vowel Pick — Classic** | Separate the picked strike from a moving, mouth-colored body. | Subtle/Medium, restrained negative feedback following the existing filter contour; no extra Drive initially. | Nail/strike clarity, slower vowel body, pitch and dry overlap behavior. Check the filter-envelope→FM-depth route separately. |
| **Loose Change — Classic** | Selectively hollow the metallic knock so its existing global FM steps catch different spectral glints. | Subtle, low negative feedback or linear Dry addition; mostly static color, not a long whoosh. | Short pitched knock, deterministic variation and empty spaces. No claimed per-hit randomization, samples or internal note generation. |
| **Undertow Grid — Rage** | The held mono bed leaves time to hear stepped, submerged phase movement between the AM pulses. | Medium, moderate signed feedback; reuse the existing stepped cutoff clock and protect the region below the played fundamental. | Low weight, sustained bed, glide and pulse gaps. Compare against the approved Diode version: this replaces its bite/low-pass, not adds to it. |
| **Fault Bloom — Classic** | Give the evolving, already rich network source moving spectral cavities rather than another layer of distortion. | Medium, moderate feedback with Drive dormant; reuse its existing slow cutoff movement. | The network/ratio evolution must remain the main story; reject a generic phaser wash or stronger constant grit. |
| **Orbit Shards — Classic** | Let selected inharmonic glints appear/disappear while the existing stepped pitch and drifting ratio continue. | Subtle, linear waveshaping, restrained static/light-envelope phase color; no new sample-and-hold lane. | Distinct glints, light poly separation and the original global motion. Ripple shapes the excitation before Space, not the diffuse room tail itself. |
| **Tendon Drift — Classic** | A supple, hollow body can bend with the existing long Pitch + Tone drag without becoming continuously dirty. | Medium, restrained negative feedback; reuse the existing drag, slow cutoff movement and contour. | Smooth legato root, intermittent strain and contrast with Fault Bloom. No new wobble, pitch step or blanket distortion. |

**Small first listening set:** Tendon Drift, Jaw Twang, Vowel Pick, Velvet Pulse,
Undertow Grid and Folded Strings. These cover supple mono motion, two different
mouth/pluck articulations, a rhythmic pad, a held sequence and a soft chordal swell.
Undertow is the one approved Rage alternative in this first pass; its original
must win unless the phase version genuinely retains the low-body role.

## Conditional pass — 12

These are plausible **alternatives**, but not recommendations to replace their
defaults without a particularly convincing comparison.

| Preset / current family | Why it might work | Why conditional |
| --- | --- | --- |
| Rubber Relay — Rage | A brief phase cavity could reinforce the rounded, chewing bass against continuing eighth-note FM. | The approved Diode bite and settled low fundamental already do the job; cancellation could simply remove the bass. |
| Classic Tines — Classic | A restrained phased-EP alternative is a familiar musical use for the decaying harmonic key. | This is an exposed dry FM reference, not an effects demonstration. Preserve its index/shape strike and do not call voice-local Ripple a conventional stereo post-EP phaser. |
| Brown Console — Rage | Slight phasing could add worn organ-register color to the sustained beating body. | It replaces Tape rounding and low-pass darkness. No Leslie/chorus claim, extra vibrato or loss of readable clusters. |
| Replica Voice — Classic | Moving notches could articulate its contracting vocal color. | Filter-envelope→operator-mix ownership changes, negative velocity response and the specifically shortened colored tail make this a fragile migration. |
| Budget Heaven — Classic | Static shallow phase notches could suit the exposed, cheap synthetic-string edge. | Its brief explicitly excludes slow cutoff evolution and ambient polishing. A swirling lush ensemble would be the wrong preset, however attractive. |
| Melted Ensemble — Rage | Gentle phasing is plausible on its rich, slowly beating string/organ blend. | Losing the approved Tape/low-pass body may make the deliberately dark ensemble brighter or thinner. No extra motion that masks the beating. |
| Hollow Bronze — Classic | Linear/low-feedback phase coloration could emphasize a hollow metallic body without adding new oscillator harmonics. | Preserve the accepted reduction in settled buzz, pitch and strike/ring contrast. No added Drive to rebuild the removed density. |
| Reel Clav — Rage | Short, nearly static phase coloration could give repeated woody figures a useful hollow edge. | Replacing the accepted Transistor transient with a swish or ringing body loses the dry rhythmic job. |
| Spool Guitar — Classic | Low-stage negative feedback could strengthen its nasal/choked string impression. | Its actual closing low-pass contour is part of the brief. A moving notch is not equivalent; reject if the endpoint stops feeling choked. |
| Taffy Coil — Rage | A hollow elastic phase body could complement the existing opposed bends. | Strong pitch gestures, filter FM and routed shape already interact. Protect punch/root readability and compare against the accepted Diode material, not an invented stack. |
| Gate Garden — Classic | Spectral slots could make a held chord pattern feel more interleaved. | The original slow **shared** filter opening is structural. Voice-local phase envelopes reinterpret it; reject if staggered notes lose the intended collective behavior. |
| Paper Motor — Rage | Restrained phase cavities might give the already clocked motor a more mechanical hollow color. | It loses the approved Transistor edge and must remain small and papery, not a metallic resonator or continuous phaser sweep. |

## Complete 77-preset disposition

**First** = first audition pass; **Conditional** = only after the first pass;
**Keep** = retain the current family/default. “Keep” does not mean Classic:
the nine retained Rage voices keep their approved Rage versions.

### Bass — 9

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Crooked Velvet | Classic | Keep | r2 restored clear settled pitch; loping amplitude, bend and glide already provide motion. Don't hollow the fundamental or undo that correction. |
| Classic Bass | Classic | Keep | Dry, restrained 1:1 FM utility with a brief bright front; keep the stable classic bass reference. |
| Felt Weight | Classic | Keep | Damped 300 Hz pluck with low FM/stress; rounded low weight is the point, not spectral holes. |
| Pocket Slap | Classic | Keep | Brief index pop and clear settled fundamental already give the slap/rubber job without cyclic motion. Protect concise stops. |
| Rubber Relay | Rage | Conditional | Chewing FM could benefit, but only if the existing Diode bite and bass weight survive replacement. |
| Silt Bass | Classic | Keep | Restrained, dry 0.996-ratio foundation; preserve low-end steadiness and shallow drift rather than phase-modulated weight. |
| Sub Canvas | Classic | Keep | Near-sine, zero-FM foundation has little spectral material to sculpt; Dry cancellation would mostly vary its level. |
| Suction Bass | Classic | Keep | Accepted smooth near-sine sub gulp specifically removed bite; its modest closing low-pass must not become a notched bass. |
| Tension Core | Rage | Keep | Dirty body, approved Distortion and real opening bite are already coherent. Preserve the settled pitch and removal of the onset burp. |

### Keys & Organs — 11

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Afterimage Keys | Classic | Keep | Immediate FM strike then delayed filter-envelope-driven FM bloom is already a two-stage story; phase coloration/ownership changes could conceal it. |
| Backspun Tines | Classic | Keep | Explicit clean, still near-sine reference with restrained late tine edge; no resonant whoosh or grit belongs in its brief. |
| Classic Tines | Classic | Conditional | Worth a phased-EP alternative, not an automatic change to the dry harmonic reference. |
| Glass Keys | Classic | Keep | Original bright high-FM/network key reference. A new phase-key voice is already covered by the Ripple Studies; no need to displace this anchor. |
| Brown Console | Rage | Conditional | Worn phased-organ color is plausible, but replacing Tape darkness/rounding must not obscure beating and tremolo. |
| Late Glass | Classic | First | Stationary/shallow phase coloration can become audible with the late-arriving FM partials, preserving the warm-first/glass-later story. |
| Low Battery Piano | Rage | Keep | Wear comes from its hammer, detuned ring and approved Tape voice. Thinning the body or adding a whoosh is not the same intent. |
| Clear Stops | Classic | Keep | Stable, dry, even sustained organ explicitly without cyclic motion; retain the clean counterpart to Brown Console. |
| Tine Keys | Classic | Keep | The accepted EP identity depends on substantial shared filter FM (2.5) and resonance; changing family is a major texture/ownership redesign. |
| Sugar Teeth | Classic | Keep | Very brief abrasive front must resolve to a delicate rounded body. Fixed post-phase drive could put teeth in the entire tail. |
| Vanishing Clav | Classic | Keep | The 110 ms high-ratio flick relies on audio-rate Tone FM and no slow sweep. Preserve the accepted papery endpoint, not a phase-ring demonstration. |

### Leads & Brass — 10

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Copper Lift | Rage | Keep | Approved Softclip plus an actual opening brass contour makes solid stabs/swells; hollow notches are not an obvious improvement to brass body. |
| Bent Aerial | Rage | Keep | Narrow, beating, off-center lead already has a specifically approved Transistor edge; do not trade it for generic phaser movement. |
| Closing Argument | Classic | First | Speaking, inward-moving spectral color is a strong phase-notch hypothesis, provided the existing closing gesture stays intelligible. |
| Contact Lead | Classic | Keep | Original high-FM/high-stress lead reference already has strong network character; preserve the anchor rather than add another effect identity. |
| Glass Current | Classic | Keep | r2 deliberately reduced coupling/stress on a liked lead. Preserve its readable voice, glide and controlled modulation. |
| Reed Snap | Rage | Keep | Fast poly snap, strong velocity opening and the approved Softclip/triangle refinement matter more than making every reed hollow. |
| Rewind Reed | Classic | Keep | Deliberately rough, short reverse-like endpoint with index falling as amplitude rises. Preserve the contracting vowel and timing rather than adding motion. |
| Replica Voice | Classic | Conditional | A vocal phase contour is plausible, but routed source-mix ownership and corrected tail length need a separate strict comparison. |
| Whistle Stem | Classic | Keep | Pure, soft near-unison whistle/tap and mild vibrato; cancellation would mostly weaken the intended simple pitch center. |
| Plastic Hero | Classic | Keep | Explicitly a glossy melodic hook, not a resonant/experimental effect lead. Preserve the pointed front, strong held center, scoop and glide. |

### Pads — 9

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Budget Heaven | Classic | Conditional | Static shallow phase color might fit cheap synthetic strings; new slow sweeping or lush polish contradicts the brief. |
| Melted Ensemble | Rage | Conditional | Beating rich tap supports gentle phasing, but its approved dark Tape/low-pass body must not be replaced by brightness/thinness. |
| Classic Voice | Classic | Keep | Preserve the dry soft choir reference and simple vowel clarity; an all-pass cascade is not a missing formant bank. |
| Folded Strings | Classic | First | Soft rich swells and relative beating can carry gentle phase folds without extra grit or a new modulation layer. |
| Soft Alloy | Classic | Keep | Chord helper, long release and several network/filter routes already make a dense moving pad. Protect modulation ownership and voice budget. |
| Soft Pad | Classic | Keep | Keep the original chord-helper pad as an unchanged reference and lower-cost option. |
| Stillwater | Classic | Keep | Despite the watery name, its intent is patient harmonic space and long quiet fades—not an audible bubbling/swishing effect. |
| Undertint | Classic | Keep | Luminous FM emergence and FM closing before the amplitude tail, without obvious wobble, should remain exposed. |
| Velvet Pulse | Classic | First | Existing clocked cutoff and deep AM provide a clear role for moving phase slots without inventing a new rhythmic mechanism. |

### Plucks & Mallets — 12

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Classic Koto | Classic | Keep | Dry 4:1 nail/string reference; keep the distinct FM strike and damped body rather than effect-coloring every pluck. |
| Classic Tubes | Classic | Keep | Exposed 3.5:1 clang and long relaxing ring are the reference. Moving partial emphasis can compromise that bell identity. |
| Carousel Wire | Classic | First | A static/short-envelope phase shape can reveal the existing ratio/mix-driven hollow/wiry changes instead of adding redundant motion. |
| Jaw Twang | Classic | First | Rich FM through a fixed-register mouth-color region is a strong use for moving phase hollows; retain fixed-register tracking and root. |
| Vowel Pick | Classic | First | Picked strike plus a slower damped vowel body is a purposeful note-articulated phase-filter use case. |
| Hollow Bronze | Classic | Conditional | A linear/low-feedback hollowing experiment is plausible; do not reintroduce the settled buzz removed in r2 or change perceived pitch. |
| Orch Chime | Classic | Keep | High-ratio clang-to-held-body transition and delayed FM contour already define its orchestral chime job. |
| Rosewood Tap | Classic | Keep | Stable, quickly damped wooden mallet is an understated reference; extra dispersion/ringing can cost its tactile attack. |
| Reel Clav | Rage | Conditional | A short phased-clav alternative might work, but preserve the approved woody edge and empty rhythmic gaps. |
| Spool Guitar | Classic | Conditional | Phase hollows fit nasal string color; they cannot automatically replace the accepted choked, closing low-pass gesture. |
| Unstruck Bell | Classic | Keep | Exposed thin 7.13-ratio upper ring and thinning endpoint are deliberate. Avoid changing the partial hierarchy or making a denser bell. |
| Taffy Coil | Rage | Conditional | Hollow elastic color could complement opposed bends, but approved Diode punch, filter FM and pitch readability take priority. |

### Drums & Percussion — 9

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Bowl Toms | Classic | Keep | r2 specifically restored attack and reduced excess tone. Do not smear the brief FM/pitch strike with another resonant body. |
| Floor Pulse | Classic | Keep | Praised bare, dry low kick: protect its settled fundamental, downward entrance and damping. |
| Hard Hit | Classic | Keep | Original stressed-network impact reference already supplies a hard alternative; phase coloration has no clear missing role. |
| Inward Tom | Classic | Keep | The compact rising-pitch suction and rough endpoint are the mechanism. Preserve the short dry handoff rather than adding a hollow tail. |
| Loose Change | Classic | First | Metallic knocks with existing global FM-step variation can catch different spectral hollows without a new sequence or noise source. |
| Oxide Rim | Rage | Keep | Accepted Transistor/papery edge and 130 ms articulation fit the rim brief; a swishing or ringing phase body would change its role. |
| Rivet Kick | Classic | Keep | Preserve its compact harmonic front and contrast with Floor Pulse; cancellation of low body is a poor integration goal. |
| Skin Knock | Classic | Keep | Short clean membrane-like pitch relaxation is an important material contrast to Taffy, not another resonant effect impact. |
| Slack Drum | Classic | Keep | Low-FM detuned body and a separate five-semitone entrance already supply looseness. A sparse source risks level loss rather than useful phase color. |

### Sequences — 5

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Buzz Loom | Classic | Keep | Its distinguishing 48-cycles/s AM sidebands are generated after voice filtering. Ripple would sculpt the carrier, not directly filter those sidebands. |
| Gate Garden | Classic | Conditional | Interleaved spectral slots are plausible, but its explicitly shared slow chord opening changes under per-note filtering. |
| Pulse Lattice | Rage | Keep | Approved Diode plus short independent plucks already clarifies the lattice. Preserve this against a more phase-colored held Undertow alternative. |
| Socket Dance | Classic | Keep | Accepted basic chop/clock anchor with deliberately shortened release; protect gaps and its role as a clean rhythmic reference. |
| Undertow Grid | Rage | First | Sustained mono sound and stepped cutoff leave time for submerged phase movement; preserve the bass/AM contract against the approved Rage version. |

### Textures — 5

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Fault Bloom | Classic | First | Rich evolving network spectra give moderate phase cavities real material to shape without adding more distortion. |
| Orbit Shards | Classic | First | Selective phase hollows can articulate low-stress inharmonic glints; preserve independent note separation and existing global pitch/ratio motion. |
| Overtone Lantern | Classic | Keep | The exposed fifth-ratio operator and its implied pitch are the entire job. Do not notch away the defining partial hierarchy. |
| Paper Motor | Rage | Conditional | A small mechanical hollow could fit, but replacing approved papery Transistor edge must not produce a generic metallic phaser motor. |
| Tendon Drift | Classic | First | Supple phase coloration has a direct job alongside the long Pitch + Tone drag and intermittent strain, without constant extra grit. |

### FX & Transitions — 7

| Preset | Current | Decision | Reason / identity to protect |
| --- | --- | --- | --- |
| Brake Bloom | Classic | Keep | Self-braking network-activity→FM-depth is the special mechanism; a new phase effect would compete rather than supply the missing brake. |
| Flash Room | Classic | Keep | Fixed short excitation is deliberately there to expose an ordinary diffuse room tail. Ripple is not a new room processor. |
| Pressure Lift | Rage | Keep | Rising pitch, increasing FM and actual low-pass opening plus approved body breakup form the pressure arc. A phase sweep is not equivalent. |
| Reverse Veil | Classic | Keep | Amplitude plus true low-pass opening on a fixed source creates the reverse-like reveal. Do not substitute a moving notch for that controlled construction. |
| Shred Relay | Rage | Keep | Approved breakup increases while the actual spectrum closes within AM. Preserve that two-way development, not a differently bright comb-filter voice. |
| Sinkhole | Classic | Keep | Metallic entrance must collapse to low weight and a spacious tail; avoid notching the foundation or prolonging the perceived metallic endpoint. |
| Spectral Cut | Classic | Keep | The whole brief is a fixed bright source becoming genuinely muffled. Ripple alone cannot replace the closing low-pass. |

## Audition and adoption plan — not executed by this review

1. Make append-only Study revisions for the twelve first-pass candidates, never
   edits to selected/frozen sources or existing native/user files. Keep all current
   defaults until individual listening approval; “First” is not a conversion quota.
2. Compare **A: actual current selected preset**, **B: linear wet-only Ripple**
   (Drive=0, Feedback=0, Dry=0), **C: the proposed phase blend/feedback** on the same
   original MIDI. B exposes the family/route change rather than disguising it as
   extra drive. If Drive has a specific job, add a separate shaped comparison.
3. Preserve source FM/network, pitch gestures, amplitude/Space and clocks. Retune
   phase center, bandwidth, Dry and the existing cutoff contour only for a stated
   reason; do not blindly copy low-pass settings. Retaining stored FM-route values
   does not imply unchanged source behavior when envelope ownership changes.
4. Use whole-patch references for A/B. Turning Ripple off while retaining its
   retuned shared Cutoff/Resonance is **not** the original Classic/Rage preset.
   Matched listening copies should remove level bias while raw levels, transient
   peaks and the new makeup gain remain visible. Require meaningful magnitude/
   temporal spectral color, not merely a waveform phase-null difference.
5. Test root/register, soft/hard playing, legato, staggered chord entrances, release
   overlap, original rhythmic gaps and CPU at actual voice counts. Verify native
   state/audio recall and output headroom. Gain calibration does not excuse a thin
   bass, softened attack, overdriven common ceiling or a failed deadline.
6. Adopt only versions that still earn their original name/role. A phase-colored
   sound that is good but has become a different instrument belongs as a separately
   named optional preset, not an automatic replacement. Promotion remains separate.

This review makes no new sonic-equivalence, hardware, real-time qualification,
installation, factory-promotion, commit or push claim.
