# Bowl Toms r2 — attack before pitched body

Verbatim user feedback is recorded separately in `evaluation/events.jsonl`:

> floor pulse and rivet kick are really good. bowl toms isn't quite right though,
> it just doesn't have that drum machine tom sound to it. too much tone and not
> enough attack.

Both kicks are now direction-kept and remain unchanged. Bowl Toms r1 stays at
program 46 with its original state/evidence. **Bowl Toms r2** appends program
**47**, same `bowl-toms` family, parent `review:bowl-toms:r1`. This is a focused
attack/body revoicing, **not** a one-parameter isolation, exact named drum-machine
model or automatically successful musical result.

## Changed balance

| Area | r1 | r2 | Intended role |
| --- | --- | --- | --- |
| Amp attack / decay / release | 1.5 ms / .62 s / .28 s | 1 ms / .24 s / .09 s | Sharper entrance and less persistent body/tails |
| Base FM / nominal ratio | .22 / 1.48 | .025 / 2 | Much less continuous inharmonic coloration |
| FM shape / amount | Triangle / .12 | Sine / 0 | Harmonic settled geometry, not metallic bowl ring |
| FM envelope amount / decay / release | .65 / 60 ms / 120 ms | 2.8 / 10 ms / 25 ms | Concentrate harmonic bite at the strike |
| Autobend depth / time | +4.5 st / 65 ms | +12 st / 18 ms | Faster drum-like pitch entrance rather than slow relaxation |
| Coupling / stress / Response | .13 / .012 / 28 ms | .07 / 0 / 12 ms | Reduce lingering network coloration |
| Tone cutoff / resonance | 2100 Hz / .16 | 6500 Hz / .06 | Expose transient bandwidth without resonant ring |
| Tone envelope decay / release / amount | 55 ms / .16 s / .8 | 12 ms / .06 s / 1.2 | Brief open entrance |
| Tone tracking / velocity | .7 / .75 | .55 / 1 | Retain useful velocity accents across tom notes |
| Space mix | .035 | 0 | Dry drum-machine-oriented articulation |

Every other condition, including **output −4.03 dB**, pitch mapping, poly/chord,
zero sustain, no glide/legato/LFOs, Autobend target/phrase behavior and zero direct
operator mix, stays unchanged. All original MIDI bytes are copied exactly, including
the r1 track-name metadata. `make_refinement.py --verify` reproduces preset/MIDI/
manifest; creation refuses existing paths. Registration/tests check lineage,
the exact changed-key set, preserved output/articulation and identical MIDI.

Envelope times are control settings, **not guaranteed time-to-silence**. Amplitude
decay is exponential, and short notes still enter release early. No one-shot,
phase-reset switch, noise source, sample layer, drum map, architecture, host
contract, quality/voice policy or global DSP change is added.

## Compare in Logic

Restart Logic/create a fresh **0.7.19 / eval-019** instance. Compare r1/r2 at the
same gain, notes, velocities and gate lengths, especially **MIDI notes 41/45/53**,
then descending overlapping fills and ghost accents. MIDI numbers avoid differing
host octave labels. No transport requirement. Try .3–.6 s notes, short taps and
held hits: does the strike read as a drum-machine tom now, without becoming a
miniature kick, click or bass pluck? Judge remaining pitch/body, not just brightness.

For articulation comparison use **raw/live levels**. The new whole-demo active RMS
is about 2.09 dB lower without any output trim; shorter/less overlapping tails can
change gated windows and total energy. Separately normalized previews add +2.085 dB
to r2 versus almost zero for r1, potentially obscuring the raw balance change.
Preview target is −24 dBFS gated active RMS, not LUFS/perceptual matching.

## Diagnostics, not a musical verdict

Uninstalled packets under `evaluation/diagnostics/batch-11-tom-r2/` preserve:

- Full original MIDI: unchanged r1/r2/Skin definitions (Skin keeps its own output).
- Identical isolated 1 s hits at notes 41/45/53, velocity 104, all three voices.
- r2 with only FM envelope amount removed, and only Autobend enable removed,
  on the isolated MIDI; each is a separate same-output control.
- Fresh 25 ms/1 s gate pairs and identical 160 BPM sixteenth fills for r1/r2.

All **14 captures** repeat exactly with zero 64/257 maximum difference, no numerical
faults/master-ceiling hits and completed amplitude releases. Gate pairs match
exactly until frame 1,200, then differ; no one-shot equivalence is claimed.
Fast fills reach four voices for r1 versus two for r2 because tails clear sooner,
not because the voice limit or poly mode changed.

Raw isolated-note analysis at equal stored output measures 0–40 ms transient RMS
relative to 80–300 ms body RMS: **1.62/2.07/2.11 dB → 5.99/6.32/6.25 dB** for
notes 41/45/53. The transient rises about .75–.89 dB while body RMS falls about
3.39–3.49 dB. A simple first-order 1.5 kHz high-pass proxy in the transient rises
about 2.08–3.40 dB. These quantify the intended balance change, not perceived punch,
noise content, drum-model authenticity or approval. The reported last-above−60 dBFS
window is capped by the 1 s analysis window and is **not** a measured full decay time.

## Production evidence

Own original 108 BPM / 22 s demo, 48 kHz stereo / 24-bit export through production
processor. Identity/conditions survive export/reimport; same-block repeat exact;
64/257 maximum error zero; no faults/master-ceiling hits, completed amplitude
release and final-second RMS at the −100 dBFS reporting floor.

| Raw peak | Active RMS | Preview gain | Max voices | Raw float hash |
| ---: | ---: | ---: | ---: | --- |
| −13.64 dBFS | −26.09 dBFS | +2.085 dB | 2 | `d6526ab3ac09a2c5` |

Apple M4 Max, two full own-demo passes at 48 kHz/64: mean occupancy **2.09%**,
p99 **83.88 µs**, zero measured overdue callbacks against **1,333.33 µs**. Rests
and at most two voices; not max-polyphony/device-xrun/cross-rate or renewed full
sanitizer qualification. Pre-registration eval-018 development render is separately
archived with identical final conditions, not relabeled as eval-019.

F1 combined maximum-polyphony 96 kHz and Socket-on-new-120-BPM reference timing
remain unqualified. Both kicks and old tom preserve their original own-demo hashes
in eval-019; earlier frozen presets stay unchanged. There are still 31 families:
30 direction-kept and one tom refinement awaiting preference, not final library
approval. Forty-four review revisions/four original factory programs. Deliberate
FX/transitions remain required next. No commits/pushes.
