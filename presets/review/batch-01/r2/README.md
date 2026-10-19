# Batch 01 / r2 — focused refinements

Parent revisions are preserved in `../r1/`; the MIDI files here are byte-identical
copies of those performances. No DSP architecture or factory sounds were changed.

## Feedback being tested

User: "they both sound really good. soft alloy is a bit clinical or overly
simplistic if i had to give feedback. tension core sounds awesome but has a slight
burp on the attack at the beginning, but love the dirtiness on that one."

### Soft Alloy

Goal: more evolving texture, not a brighter, louder or wetter redesign.

| Condition | r1 | r2 |
| --- | ---: | ---: |
| Coupling LFO rate (Hz) | 0.07 | 0.047 |
| Coupling LFO depth | 0.08 | 0.12 |
| Stress LFO rate (Hz) | 0.11 | 0.073 |
| Stress LFO depth | 0.035 | 0.055 |
| FM shape amount | 0.18 | 0.26 |
| Route 3 source | Off | Network |
| Route 3 target | Depth | Shape |
| Route 3 amount | 0 | 0.16 |

Independent slow coupling/stress cycles vary the network conditions; its own
signal also drives FM shape. Envelope, pitch, filter, output, FM ratio and Space
are unchanged. This is a candidate treatment for the clinical/uniform impression,
not an assertion that offline metrics establish musical improvement.

### Tension Core

Only **Autobend enabled: 1 → 0** changes. In r1 the carrier receives a
0.7-semitone, 25 ms bend on phrase onset. Removing that bend is the first isolated
test of the attack-burp hypothesis. Its stored depth/time remain available for
manual A/B; r1 remains built in. Amplitude/filter/FM envelopes, coupling, stress,
FM dirt, output and 65 ms legato glide are identical.

The pitch bend is a plausible cause, **not a confirmed diagnosis** of what the
user heard. If r2 still burps, investigate filter/FM attack interactions next;
do not strip away the liked dirty body by changing several controls at once.

## Evidence / eval-002

Production-processor renders at 48 kHz using the same fixed MIDI, host tempo and
durations as r1; previews use −24 dBFS gated active RMS, not LUFS. Both repeat
exactly at 64 frames, export/reimport without changed conditions, finish their
releases and have no numerical faults or master-ceiling hits.

| Revision | Raw peak (dBFS) | Raw active RMS (dBFS) | Preview gain (dB) | Max voices | Raw sample hash | 64/257 max sample difference |
| --- | ---: | ---: | ---: | ---: | --- | ---: |
| Soft Alloy r2 | −9.23 | −22.75 | −1.25 | 8 | `264bbbbede795063` | 0 |
| Tension Core r2 | −15.72 | −23.55 | −0.45 | 1 | `db07c509e8f743a1` | 5.82e−11 |

Same-build r1 raw hashes reproduce eval-001: pad `69d0f86b01d318e8`, bass
`b23dbef65131baea`. The earlier pre-eval-001 bass preview is separate historical
evidence; use the same-build r1 render for this comparison.

For these exact demos on the Apple M4 Max (two 64-frame passes), pad mean
occupancy was 15.15% / p99 430.29 µs; bass 2.72% / p99 60.63 µs, with zero measured
overdue callbacks against 1,333.33 µs. Quiet/release portions are included; these
are not sustained-polyphony or Logic/device-xrun qualification. The open F1
96 kHz headroom limits remain unchanged.

## Listening questions

- Pad: is r2 less clinical over held chords, without becoming distractingly busy?
- Bass: does the initial burp disappear, while dirtiness and punch remain?
- Check isolated/repeated bass notes as well as overlapping legato notes; r1's
  bend is phrase-only, so the contrast may be strongest at the first attack.
- Prefer either r1 if the change loses something you liked. Neither is final.

Use the review selector's r1/r2 names for A/B in Logic; keep the same MIDI,
velocity and channel-strip effects. Record host/sample-rate/buffer details only
when supplied. See `evaluation/CURRENT.md` for the current installed build/trial.
