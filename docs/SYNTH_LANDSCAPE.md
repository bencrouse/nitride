# Synthesis landscape / first research pass

Research date: 2026-10-02. This is a targeted documentation survey, not an
exhaustive market inventory or an audition of the instruments. Descriptions below
are sourced capabilities; proposed sounds and Nitride implementation directions
are hypotheses that need listening tests.

## Updated brief

- An expressive FM instrument that rewards discovery through playing.
- A dependable path to familiar jobs such as a beautiful pad.
- One exceptional sound-making capability that can produce a track's centerpiece.
- "Moment" describes the role a sound plays in a production, not a timed event control.
- Interface text should identify functions and show useful feedback. No marketing copy.
- Native JUCE UI, following Carbide's infrastructure conventions.

The existing Rupture topology-excursion sketch is an earlier hypothesis. Its name,
trigger, automatic return, and character modes are not requirements.

## Current products and relevant capabilities

| Product | Documented capability | Implication for Nitride |
| --- | --- | --- |
| Serum 2 | Wavetable, sample, multisample, granular and spectral oscillators; spectral sample resynthesis at the harmonic level; FM / PD / distortion and dual warps | Adding spectral or granular synthesis alone is not enough to establish distinction. |
| Zebra 3 | Additive oscillators with up to 1024 potentially inharmonic partials; FM pairs accepting external audio as another operator; modal resonators with feedback/damping; resonator output can feed FM; vector/scan mixing | FM plus resonators is already available. A proposal needs a specific interaction or sonic behavior beyond that pairing. |
| Pigments (current page identifies Pigments 7) | Modal, granular, wavetable, sample, harmonic and virtual-analog synthesis; engine cross-modulation; macros, Play View and MPE | Acoustic/digital hybrids and simplified performance surfaces are established. |
| Sumu | Additive resynthesis of up to 64 bandwidth-enhanced partials with individual frequency, volume and noisiness; FM, per-partial delay and vector-field spatialization | Partial-level manipulation can support unusual sonic transformations, but spectral FM and independent partial timing already exist. |
| Kaivo | Granular excitation, tuned resonators and 2D physical bodies using FDTD models; internal vibration can be affected at different locations | A model can supply physical response and identity, rather than simply a post-synth effect. |
| Chromaphone 3 | Eight modeled resonator types; bidirectional coupling between resonators; four performance macros per layer | Coupling itself is not new. A small expressive surface can sit above a detailed model. |
| Generate | Eight chaotic oscillator types, five wavefolders, sine-to-chaos transitions, MIDI/MPE modulation | A chaos knob is already a product category. Nitride would need an identifiable behavior beyond increasing instability. |
| KULT | More than 30 oscillator models, including strange attractors; FM / AM and MPE | Chaos combined with FM is also established. |
| MYTH | Sample-derived resynthesis, transformer controls, trainable resonators, MPE and section-lockable randomization / preset breeding | Learned/sample-derived material and exploratory patch navigation are both occupied territory. |
| KONTRAST | Nonlinear wavetable scanning, modulation and MPE | A different traversal of a familiar engine can be a product's central idea. |
| Synplant 2 | Genetic patch exploration; local Genopatch AI estimates synthesis settings from recordings; deeper DNA editor | AI patch matching and exploratory interfaces are established; this is distinct from running a neural audio generator continuously. |
| Neutone Morpho | Real-time learned timbre transfer; input audio is resynthesized into another style; custom model training | Neural timbre transfer is shipping technology, although Morpho is an audio-processing product rather than an FM instrument. |

Version/release detail checked: Zebra 3's official page lists initial release on
2026-04-20 and version 3.0.2 on 2026-07-15. Treat other product version details as
page snapshots rather than inferred release dates.

## Technical research references

**RAVE:** IRCAM's official repository documents real-time variational audio
autoencoders, exposed latent representations, streaming model export, and newer
style-transfer configurations. A learned continuous space can be manipulated as
audio/control signals. Pitch, attack fidelity, latency, and polyphonic cost are not
automatically solved by choosing RAVE.

**DDSP:** Google's original 2020 research combines interpretable DSP elements with
learned control. The distinction matters: neural synthesis may generate a waveform,
control a known synthesis engine, or estimate a patch. Those imply different
instrument designs and development work. This is a foundational reference, not a
claim about the newest research publication.

## Candidate directions

These are design proposals. None is established as commercially unique by this survey.

### A. Nonlinear body coupled to FM

FM excites a tuned virtual body; the body's state feeds back into the voice. Explore
amplitude-dependent stiffness and coupling so that timbre and articulation emerge
from a dynamical interaction. Distinguish genuine stateful interaction from routing
FM through a resonator and then distortion.

- Possible main control: **Coupling**, with an optional **Stiffness** dimension.
- Target sounds: elastic pitched attacks, hollow/wet resonances, strained metallic
  tones, sustained textures that change with playing intensity.
- Pad behavior: low coupling and stable damping, with conventional FM still usable.
- Main uncertainty: whether the distinctive territory is broad and playable, rather
  than a narrow set of resonant effects. Tuning, stability, aliasing, and polyphonic
  cost need testing.
- Relative project scope: substantial but potentially incremental in native C++.

### B. Live spectral deformation

Give a control access to partial spacing, coherence, or timing in a live FM voice.
Choose one intelligible operation; avoid packaging several unrelated spectral
effects under a vague name. Consider partial-domain synthesis versus FFT processing
before assuming latency or perfect tracking.

- Possible main control: **Spread** or **Deform**, after selecting the precise operation.
- Target sounds: tonal structures stretching into inharmonic ones; brittle, fragmented
  or formant-like timbres; coherent pads with spectral motion.
- Main uncertainty: how it differs audibly and behaviorally from existing spectral
  warps and Sumu's partial manipulation. Transients, phase, tracking and latency matter.
- Relative project scope: substantial; representation choice determines complexity.

### C. Learned timbre transformation

Use a locally running model to transform FM articulation into a learned material or
to navigate a learned sound space. Decide explicitly between waveform generation,
timbre transfer and learned control of the FM engine.

- Possible main control: **Position** in a bounded learned space, or a transformation
  strength for one selected model. Do not assume arbitrary models can be interpolated.
- Target sounds: hybrid synthetic/object/vocal qualities and unusual intermediate
  textures. These are listening hypotheses, not demonstrated Nitride results.
- Main uncertainty: preservation of MIDI pitch, envelopes and attack; meaning of the
  control axes; consistency away from training examples; native inference cost.
- Relative project scope: highest, including dataset/model development and deployment.

## Provisional recommendation

Start the discussion with A and B. A has the strongest initial fit to an expressive,
material-like instrument; B has strong FM continuity and a comparatively direct
visual representation. C is worth investigating if the learned-material sound is
specifically what the user wants, rather than selecting it on technical novelty alone.

Confidence is moderate about fit and low about uniqueness or sonic success until
auditioned. Combining FM and physical modeling is already established, including
in Zebra 3; the hypothesis for A is the exact nonlinear interaction and playable
control range, not the existence of resonators.

The next useful experiment, once authorized, is a small sound comparison rather than
another polished panel: the same plain FM patch and performance sent through two
candidate mechanisms, with loudness controlled. Compare a held chord, a short hit,
and a pressure/velocity variation. Look for a distinctive family of sounds, useful
intermediate positions, playable pitch/articulation, and a restrained pad setting.

## Sources

Official product descriptions:

- [Serum 2](https://xferrecords.com/products/serum-2)
- [Zebra 3](https://u-he.com/products/plug-ins/zebra3/)
- [Pigments](https://www.arturia.com/products/software-instruments/pigments/overview)
- [Sumu](https://madronalabs.com/products/sumu)
- [Kaivo](https://madronalabs.com/products/kaivo)
- [Chromaphone 3](https://www.applied-acoustics.com/chromaphone-3/)
- [Generate](https://www.eventideaudio.com/plug-ins/generate/)
- [KULT](https://www.tracktion.com/products/kult)
- [MYTH](https://www.tracktion.com/products/myth)
- [KONTRAST](https://www.tracktion.com/products/kontrast)
- [Synplant 2](https://soniccharge.com/synplant)
- [Neutone Morpho](https://neutone.ai/morpho)

Research and implementation references:

- [IRCAM / ACIDS RAVE repository](https://github.com/acids-ircam/RAVE)
- [DDSP introduction and original paper links](https://magenta.tensorflow.org/ddsp)

The survey relied on the pages above. It did not infer undocumented proprietary
algorithms from product names or promotional descriptions.
