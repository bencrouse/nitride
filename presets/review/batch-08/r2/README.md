# Replica / Socket r2 — release-only finishing pass

The user likes Replica's concept but finds its distorted ending too long; Tendon
is praised and stays unchanged. Socket needs less release but is explicitly valued
as a useful basic anchor. Do not add novelty to justify its place.

Only **amplitude release** changes:

| Revision | r1 | r2 |
| --- | ---: | ---: |
| Replica Voice | .9 s | .4 s |
| Socket Dance | .1 s | .045 s |

Every other sound condition—including FM/filter envelopes, all routes/clocks,
network/Response, Space, output, mono/chord—and every MIDI byte stays identical.
Original names/indices/evidence remain available. New programs 31/32 append;
Tendon and all sixteen kept directions are untouched. Artist lenses stay original
imagined briefs, not authentic recipes or endorsements.

## Why amplitude release rather than redesigning the tone

Code inspection shows Replica's .9 s active amplitude tail outlasts its .4 s
filter/FM releases. Its negative envelopes relax during note-off, potentially
changing late color; the exact perceived distortion cause is not uniquely diagnosed.
Uninstalled eval-014 probes compare original wet/dry Replica and shorter amplitude
release, plus Socket reference/shorter release on isolated held notes.

Raw stereo 24-bit PCM analysis uses 10 ms post-key-off RMS windows and an absolute
−60 dBFS threshold, not a perceptual decay/distortion metric:

| Probe | Last window above −60 dBFS after off | RMS .5–1 s after off |
| --- | ---: | ---: |
| Replica wet reference | .91 s | −37.36 dBFS |
| Replica dry reference | .89 s | −36.29 dBFS |
| Replica shorter release | .54 s | −67.39 dBFS |
| Socket reference | .09 s | at analysis floor |
| Socket shorter release | .04 s | at analysis floor |

Replica's dry reference persists almost as long as wet, supporting an active-voice
duration edit rather than removing Space. Existing Space/filter state remains after
voice retirement; total audible wet tail need not equal the amplitude setting.
The analysis floor is −200 dBFS; render reports separately use −100 dBFS.

**Both shorter-release probes have exactly identical raw PCM before their first
key-off**, at the original output. One note/phase per family, no pedal or external
Logic effects: not a universal tail-duration, click-audibility, spectral-distortion
or musical-success verdict. Full phrases with overlaps need user listening too.
Inputs/audio/reports/analyzer are preserved under `evaluation/diagnostics/` and
`evaluation/artifacts/diagnostics/`, not substituted for frozen reference MIDI.

## Production evidence / eval-015, 0.7.15

Original 88/118 BPM MIDI, 26/22 s stereo 48 kHz / 24-bit exports. Preset export/
reimport preserves identity and conditions; same-block repeats are exact. 64/257
maximum differences are **0 / 2.91e−11** for Replica / Socket. Socket's nonzero
error is far below a 24-bit PCM step. No faults/master-ceiling hits, completed
amplitude releases and final-second RMS at the −100 dBFS reporting floor.

| Sketch | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
| Replica Voice r2 | −12.55 | −23.82 | −.178 | 2 | `d9f51c551e0fc897` |
| Socket Dance r2 | −11.08 | −23.81 | −.186 | 2 | `ce4b5301818a9405` |

**No output compensation is applied.** Removing quieter tails can raise gated
active RMS by changing its denominator/window inclusion; it does not mean we
made the held body louder. For release A/B, use live presets or raw captures at
identical gain. Preview normalization targets −24 dBFS gated active RMS, not LUFS,
and can slightly alter held comparison levels; its gain is not stored in the preset.

Apple M4 Max, two full demos at 48 kHz / 64: mean occupancy **3.28 / 2.49%**, p99
**124.42 / 105.92 µs**, zero measured callbacks beyond **1,333.33 µs**. Includes
rests/releases and only two voices; not maximum-polyphony or Logic/device-xrun
qualification. F1 96 kHz workloads remain unqualified. No architecture, global
curve, quality/voice policy or host contract changes.

First check whether the endings feel better without losing Replica's gesture or
Socket's cell/usefulness. Socket still needs playing transport for held-note chops.
Then use `evaluation/collection-review/pass-001/README.md` for the prepared
eighteen-family role/overlap review. R2 preferences and collection judgments are
pending; technical checks and provisional roles do not imply library approval.
