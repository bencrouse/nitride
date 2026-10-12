# FM expansion / Nitride 0.7.0

The measurements below record the initial FM milestone. Subsequent exact caching
and hot/cold voice-state optimizations, updated qualification targets and remaining
96 kHz limitations are documented in [code-review fixes](CODE_REVIEW_FIXES.md#f1--confirmed-exact-optimizations-residual-qualification-limit).

Nitride extends its existing reference FM source and **six-node expressive
network**, not a replacement two-operator synth. Carrier node 0 and modulator
nodes 1–5 retain their adaptive links, phase torques, coupling/stress feedback and
Response-dependent evolution. The shared Tone filter, tremolo, Space diffusion
and output ceiling remain in their established order.

## Progressive controls

FM depth stays in Sound. **FM...** opens a native panel over the network view;
close it (or press Escape) to return to the main performance surface. Its three
pages expose Source, Envelope and Modulation without expanding the main window.
The panel uses the existing chassis/olive surfaces, orange selection feedback,
Helvetica Neue headings and monospaced utility labels. There is no new decorative
operator diagram or animation.

| Control | Range / default | Meaning |
| --- | --- | --- |
| Ratio | 1:0.25–1:8 / 1:2 | Carrier:primary-modulator frequency ratio; continuous, with integer shortcuts |
| Shape | Sine / Triangle / Soft saturated; default Sine | Primary modulator treatment |
| Shape amount | 0–100% / 0% | Interpolate from sine to the selected treatment |
| Operator mix | 0–100% / 0% | Existing FM carrier output to audible primary/network modulator blend |
| FM attack | 1 ms–4 s / 10 ms | Independent per-voice attack |
| FM decay | 10 ms–4 s / 300 ms | Independent per-voice decay time constant |
| FM sustain | 0–100% / 50% | Independent sustain level |
| FM release | 20 ms–8 s / 400 ms | Independent linear release |
| Envelope amount | −4 to +4 index / 0 | Signed offset to FM depth |

All new sound-affecting controls start neutral. Factory patches and their four
exact neutral render fingerprints are unchanged. These are neutrality checks,
not a promise to migrate old serialized schemas.

**Try:** Glass keys, ratio 1:3, Triangle at 70%, FM depth around 1, Envelope amount
+2, attack 10 ms, decay 200 ms and sustain 10%. The strike has bright FM that
decays independently of loudness. For a less harmonic source, use ratio 1:1.414
and increase coupling/stress. Operator mix introduces the evolving modulator
without bypassing the network or post-synthesis processing.

## Ratio and network preservation

The former primary frequency was exactly twice the modulator pitch anchor. Ratio
now scales that primary anchor only. Nodes 2–5 keep their existing 3/5/7/9 anchors
and all nodes retain their offset/link evolution. Integer ratios produce harmonic
**primary** FM; the adaptive network can still introduce detuning and additional
partials. Ratio composes with existing modulator Glide/Autobend routing. Carrier
keyboard tuning and independent carrier pitch gestures do not change.

The primary has a continuous phase accumulator, initialized from the existing
primary phase when advanced settings first become audible. Editing ratio, shape,
mix or routing does not restart a phase or reset the network. Once a held voice
enters this path it retains its phase until release; returning ratio to 1:2 does
not abruptly switch phase representations.

The primary waveform changes, but phase synchronization continues to use the
original sine/cosine phase relationships. Shape amplitude does not replace the
torque equations or link-learning inputs. Operator mix is applied only to the
audible per-voice output: the internal feedback signal is still derived from the
original carrier/stress-folded result. Tests compare evolving offsets/links
between different mix settings and require identical network state.

## Independent envelope and modulation

The FM ADSR reuses the existing modulation-envelope timing/shape implementation,
with independent per-voice state. It runs at the host sample clock; synthesis
retains its existing 4× internal clock. Attack starts from the current level,
decay is exponential toward sustain and release is linear from the current level.
Time edits change slopes without jumping the level. Amount automation is
dezippered. Mono legato and return notes preserve the envelope; fresh/non-legato
strikes retrigger. Sustain and Hold use existing renderer ownership and note-off
dispatch. Voice stealing resets the independent envelope on the stolen voice.

The amplitude envelope still owns voice lifetime. FM release does not prolong
an inaudible voice and a short FM release does not truncate an amplitude tail.
Neither the shared filter ADSR nor amplitude ADSR is retriggered by FM edits.

Three optional, fixed routing rows offer:

- **Sources:** Off, FM LFO, amplitude envelope, shared filter envelope, network activity.
- **Targets:** Depth, Ratio, Shape amount, Operator mix.
- **Amounts:** signed −4…+4 in target units.

The FM LFO source uses the enabled lane's normalized, dezippered waveform,
independent of its depth. The lane's normal FM-depth destination remains active;
set its depth to zero to use only the advanced routes. Disabling the lane fades
its route source to zero. The filter envelope is advanced **once per sample** and
the same value feeds Tone and FM. Amplitude uses the previous synthesis step's
per-voice envelope; network activity is the bounded absolute value of the
previous expressive-network feedback signal (including velocity/excitation).
No UI snapshot is read and no instantaneous algebraic feedback loop is introduced.

```text
index = clamp(base depth + FM-depth LFO
              + FM-envelope amount * FM-envelope level
              + sum(depth-route offsets), 0, 4)
ratio = clamp(base ratio * 2^sum(ratio-route octave offsets), 0.25, 8)
shape = clamp(base shape amount + sum(shape-route offsets), 0, 1)
mix   = clamp(base operator mix + sum(mix-route offsets), 0, 1)
```

The existing stress-dependent network index multiplier is applied afterward,
unchanged. Zero index does not switch off network evolution or its separate
feedback phase deformation. Additions happen before the final destination clamp. All routed
offsets and base continuous FM controls have a 25 ms exponential dezipper;
ratio dezippering is logarithmic. Shape choices crossfade over that same time.
Rendered values never overwrite stored controls. Route source/target changes
dezipper the summed offsets rather than jumping to a new signal.

## Reference spectrum and bounds

The former folded odd-harmonic sine tables remain the exact neutral path. The
advanced reference evaluates signed sidebands at carrier ± n × primary frequency.
Sine uses the corresponding generalized Bessel expansion; shaped references use
Fourier coefficients of `exp(i * index * waveform(phase))`, not sine coefficients
applied to an unrelated waveform.

- Triangle uses five odd harmonics (1/3/5/7/9), peak-normalized; it is intentionally
  a finite, rounded triangle, not an unbounded-corner waveform.
- Soft saturated is the smooth cubic `1.5 sin(p) − 0.5 sin(p)^3`.
- The same waveform coefficients feed the reference and primary network node.
- Each primary harmonic and reference sideband receives the existing host-Nyquist
  taper. Above-Nyquist companion/carrier output is suppressed in the advanced path.
- Reference coefficients retain orders 0–24, interpolating index and shape amount.
  A shared immutable ~1.25 MiB table is constructed during prepare, never lazily
  or with allocation from the rendering callback.

The original 4× synthesis and four-pole decimation remain. The reference is
spectrally bounded; the nonlinear adaptive network is **not claimed alias-free**.
Its existing feedback/stress folding can generate additional high harmonics.
Raw saw/square and aggressive folding are deliberately not added.

## State and verification

Current greenfield state: **89 host controls**, preset **v6**, XML/session **v7**.
Unsupported schemas are rejected without altering the current sound or deleting
files. Every advanced value participates in host automation, Keep/import/export,
Compare, portable session recall and scoped Undo/Redo, including high host indices.
Panel gestures are generation-safe on recall and finished on page changes/close.

`Source/DSP/Fm.*` owns waveform/reference tables and destination rules;
`Engine.*` extends existing voices/network output; `FmPanel.*` provides the native
disclosure panel. `FmTests.cpp` covers ratio/timing/pitch composition, phase
continuity, independent envelope, deterministic/block-invariant rendering,
output-only mix, network influence, spectra and progressive UI/state/history.
Existing filter/LFO/network/pitch suites remain in the local validation matrix.

The independent spectral test compares against a **16× phase-sampled direct PM
reference**. Across the tested shapes, amounts and indices, maximum waveform
reconstruction error was **0.0001164** and worst relative retained-band error was
**0.00008733**. Coherent high-frequency production reference renders measured
unexpected-bin energy no greater than **−54.52 dB**, including the final soft
ceiling's nonlinear distortion. These are bounded test cases, not a universal
aliasing claim for the network.

```sh
make check-full
build/local-release/nitride_perf_artefacts/Release/nitride_perf \
  --fm-expansion --output build/checks/fm-expansion-enabled.json
```

The paired matrix exercises isolated ratio/envelope/shape/mix, optional routes,
twelve-voice shaping and four/twelve-voice integration with existing ADSR, six
LFOs, Pitch + Tone and filter FM. Ordinary before/after captures measure neutral
overhead. Timing runs must not overlap builds or playback.

Inactive and advanced synthesis paths are compiled separately so shape/mix
branches do not run inside the neutral network's oscillator loop. This preserves
the existing constant/split-pitch specializations; it is not a change to network
equations. Advanced settings carry an additional cost, measured separately.

### Local results

Measured **2026-10-11**, Apple M4 Max, macOS 27.0.1, Clang 21.0.0, pinned JUCE
8.0.15, Release. Final runs used **three passes, two audio seconds per case/pass,
0.5-second warmup**, with no concurrent builds or native playback. Loads below
are median pass **audio-budget occupancy**, not total system CPU.

| Paired workload | Rate / block | New FM controls off | New FM controls on |
| --- | --- | ---: | ---: |
| Ratio, one voice | 48 kHz / 64 | 3.98% | 4.40% |
| FM envelope, mono retrigger | 48 kHz / 64 | 4.22% | 4.52% |
| Triangle shape, one voice | 48 kHz / 64 | 4.01% | 4.37% |
| Operator mix, one voice | 48 kHz / 64 | 4.00% | 4.41% |
| Optional routes, one voice | 48 kHz / 64 | 4.04% | 4.38% |
| Triangle shape, twelve-voice stress | 48 kHz / 64 | 48.65% | 56.01% |
| Full integration, four voices | 48 kHz / 64 | 14.48% | 16.43% |
| Full integration, twelve voices | 48 kHz / 64 | 47.69% | 59.98% |
| Full integration, four voices | 96 kHz / 64 | 28.14% | 31.37% |
| Triangle shape, twelve-voice stress | 96 kHz / 64 | 95.94% | 109.78% |
| Full integration, twelve voices | 96 kHz / 64 | 97.26% | 116.83% |
| Full integration, twelve voices | 96 kHz / 256 | 97.15% | 116.71% |

Full integration includes the existing ADSR, six LFO lanes, Pitch + Tone and
filter tracking/velocity/FM in both cases; only the new FM controls differ. The
four-voice integration cases stayed at four active voices, while twelve-voice
cases stayed at twelve. All **32 final on/off pairs** produced different audio
fingerprints. All cases were numerically finite, with no renderer fault reports.

The ordinary before/after matrix preserved **all 32 exact audio fingerprints**.
Active/release render-time increases were **0.25–3.70%** in this measurement,
including environmental variation. Idle's very small baseline grew by
**22.24–28.40% relatively**: at 48 kHz / 64 the median rose from **5.83 to 7.21 µs**,
or 0.44% to 0.54% of the audio budget. Independent FM envelope advancement and
control bookkeeping are not claimed to be free.

An initial advanced implementation used separate sine evaluations for each odd
harmonic and rendered an unused modulator output tap at zero mix. A verified
odd-harmonic recurrence now evaluates all five harmonics from one sine, and the
unused tap is skipped. The recurrence matches direct harmonic evaluation within
**1e−14** in its test cases. Full integration at twelve voices / 48 kHz / 64 fell
from **68.52% to 59.98%** occupancy (12.47% less median render time); four-voice
integration fell from 19.84% to 16.43%. Across the pre/final advanced matrices,
**48/64 fingerprints** stayed exact; the shaped cases are not bitwise identical
because of floating-point evaluation differences and evolving network feedback.
This is not advertised as a bit-exact whole-engine optimization. Neutral sounds
remain exact, and spectral, stability and block-determinism checks pass.

Both final timing matrices recorded **zero overdue blocks at 48 kHz**. Maximum
polyphony at 96 kHz is **not real-time safe for the measured advanced workloads**:
full integration missed **8,894/9,000** deadlines at 64 frames, p99 **853.92 µs**
against a **666.67 µs** deadline, and **2,219/2,250** at 256 frames. Triangle stress
alone missed 8,974/9,000 at 64 frames. Increasing the block size does not cure a
steady occupancy above 100%; use 48 kHz or fewer voices for these sounds pending
broader performance work. Four-voice integration at 96 kHz / 64 stayed at 31.37%,
with zero deadline misses in this run.

Final validation passed **six Debug and six Release suites**, focused ASan/TSan
safety suites, installed-AU validation and all 89 host controls' names/IDs and
advanced FM class-state recall. Audited active callbacks allocated/deallocated
zero memory; documented framework callback and whole-bank ordering limitations
remain in `HARDENING.md`. The installed AU's MIDI render reached peak 0.0777 and
released to 5.31e−13. All **thirteen native interaction checks** passed. App-local
built-in-speaker playback with advanced FM and the existing modulation path passed
at peak 0.0366, approximately 12.74% device CPU, **zero faults/xruns**, and completed
release. No system-wide audio-device setting was changed.

Artifacts under `build/checks/`: `fm-expansion-before.json`,
`fm-expansion-neutral-final.json`, `fm-expansion-enabled.json` (pre-recurrence),
`fm-expansion-final.json`, `fm-source-ui.png`, `fm-envelope-ui.png`,
`fm-routes-ui.png`, profile logs and installed-AU logs.
