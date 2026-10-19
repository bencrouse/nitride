# Batch 01 / pad and bass sketches / r1

These are **review sketches**, not replacements for Nitride's factory patches.
The six-category collection will be explored before deeper category refinement.

## Listen

The generated previews are under `build/preset-review/batch-01/r1/` (or alongside
this README in the packaged `batch-01-r1.zip`):

- **`soft-alloy-r1.wav`** — pad, 32 seconds. Intended to bloom slowly into a warm,
  gently textured harmonic field. Hear the chord changes and how their release
  tails overlap. Four held voices, at most eight including release.
- **`tension-core-r1.wav`** — bass, 12 seconds. Intended to have a focused center,
  brief timbral bite and touch response. The first phrase is separated; the middle
  demonstrates legato slides; the final longer gates expose the sustained body.

Previews are matched to **−24 dBFS active-window RMS**, not LUFS. Matching uses
100 ms windows above −50 dBFS and a constant gain, capped for −3 dBFS peak
headroom. No compression or limiter was added. `*-raw.wav` preserves each
preset's stored output level; the exported preset does not include preview gain.

These are designed directions, not listening-approved sounds. Please judge them
on the demos and in your playing context.

## Play in Nitride

From the instrument's **... → Import preset...** menu, import either
`.nitridepreset` in this folder. Load its matching `.mid` into the DAW if you want
the exact review performance. Importing uses the normal kept-sound workflow;
the renderer itself does not write to your library or change the factory list.

- Pad: nominal **90 BPM**, eight bars in 4/4. Four four-note voicings with
  note-on velocities ranging from 57 to 104; 1:1 primary FM, slow independent
  FM/filter envelopes, gentle coupling/stress/cutoff/FM LFOs and modest diffusion.
- Bass: nominal **110 BPM**, four bars in 4/4. MIDI notes 34–48, velocities 54–127;
  harmonic 1:2 FM, short FM/filter accents, small onset pitch transient, 65 ms
  legato-only glide and light diffusion. The FM-depth lane has subtle quarter-note
  sync. MIDI tempo uses the standard integer-microsecond representation.

Both use the actual production stereo processor, including Tone, tremolo, Space
and master output. No external synth/sample/effect replaces Nitride's network.

## Reproduce and revise

```sh
make preset-review
# If the first output already exists, choose a new folder:
make preset-review REVIEW_OUTPUT="$PWD/build/preset-review/batch-01/r1-check"
```

The source preset/MIDI files and `batch.json` are authoritative. The tool reads
them, exports and reimports the presets, reads the copied MIDI, renders twice at
64 frames, and checks a third render at 257 frames. It refuses to overwrite an
existing output folder. `report.json` contains levels, hashes, tail RMS, voice
counts and offline timings. Generated WAVs are ignored build artifacts; these
source assets stay versioned independently of build cleanup.

Preserve this folder for feedback comparisons. For r2, create a new revision with
the edited presets and manifest, reuse these exact MIDI performances, and render
with `REVIEW_BATCH=presets/review/batch-01/r2` plus a new `REVIEW_OUTPUT`.

## Feedback for this pass

1. Which direction is worth developing: pad, bass, both, or neither?
2. What appeals and what distracts? For example, pad density/motion or bass
   weight/bite/glide—not only whether it is technically clean.
3. What is the single most important change for each?

Do not approve a whole category from one sketch. After hearing all six category
directions, we will build contrasting candidates and then refine the collection.
