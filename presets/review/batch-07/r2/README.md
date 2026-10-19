# Crooked Velvet / r2 — clearer bass center, retained elastic concept

Taffy Coil and Stillwater are explicitly praised and kept unchanged. Crooked's
concept is interesting, but the user hears the played fundamental as obscured
enough to feel experimental rather than bass. Keep the concept and improve that
role, not silently relabel it or flatten it into a plain bass.

## Focused recipe

| Condition | r1 | r2 |
| --- | ---: | ---: |
| Settled FM depth | .8 | .4 |
| Nominal FM ratio | 1.25 | 1 |
| Direct operator mix | .11 | .035 |
| Filter-envelope-to-mix amount | .13 | .04 |
| Route 3 target (same S&H source, amount .065) | Ratio | Depth |
| Output (dB) | 0 | −1.675 |

R2 puts more weight on the carrier and a nominally harmonic settled spectrum.
The existing sample-and-hold source still moves color/index, but no longer moves
nominal ratio. The −5-semitone, .16 s **modulator-only** attack bend remains, so
the attack can still be inharmonic while the body has a more coherent center.
The active network can still alter partial relationships; nominal ratio 1 does
not make the full signal perfectly harmonic or guarantee pitch salience.

Unchanged: mono/retriggered articulation, short carrier-only overlapping-note
glide, triangle/envelope shaping, filter-envelope mix motion (smaller amount),
free 1.9 Hz amplitude movement, 2.7 Hz FM lane, all other envelopes/velocity/Tone
conditions, coupling .4, stress .045, Response .08, Space and byte-identical MIDI.
No darker cutoff, sub oscillator, chord/groove engine or global DSP fix is added.
The original Flying Lotus design-lens metadata stays an imagined brief, not an
authentic recipe/endorsement.

## Investigation and qualification limits

Uninstalled eval-012 probes used the original groove and a separate fixed
isolated-hold MIDI set: reference, unity-ratio-only, carrier-blend-only,
lower-index-only and the combined body recipe. Inputs, audio/reports and a
standard-library FFT script/report are archived; old frozen MIDI is not replaced.

Analysis uses 65,536-sample stereo-mean Hann windows starting .5 s after isolated
notes 36/43/41. Energy within ±2 Hz of the played root is divided by energy in
40–400 Hz; raw 24-bit exports, no preview gain. FFT impulse/Nyquist sanity checks
pass. R1 fractions are **.926 / .815 / .844**; the combined untrimmed recipe gives
**.987 / .949 / .945**. Lower index alone gives **.976 / .938 / .944**, the largest
measured individual contribution in these windows.

**R1 already has the largest low-band energy at its root.** This does not contradict
the user's difficulty following its pitch. The ratio is not full-spectrum pitch
salience, a monitoring test or a bass-role verdict. Three fixed windows exclude
attack/glide and cover only particular modulation phases; no unique masking cause
is established. This is evidence for a voicing hypothesis, not proof of musical
success. The retained attack can still obscure pitch briefly; listen to short
phrases as well as sustained notes.

## Level comparison and production evidence / eval-013, 0.7.13

The untrimmed combined groove raises gated active RMS about 1.638 dB. Since gate
windows shift with gain, a pre-registration −1.65 dB trim was refined to **−1.675
dB** before freezing. Original-MIDI raw r1/r2 levels are **−24.566937 / −24.566591
dBFS**, within **.00035 dB**. This is not a louder new-version comparison. Both
previews target −24 dBFS gated active RMS, not LUFS; preview gain stays separate
from preset values. The development eval-012 capture is archived separately.

22 s stereo 48 kHz / 24-bit production-processor export, same 92 BPM MIDI as r1.
Preset export/reimport preserves identity and conditions. Same-block repeats and
64/257-frame comparisons are exact. One maximum voice, no numerical faults or
master-ceiling hits, finished amplitude release and final-second RMS at the
−100 dBFS reporting floor. Shared soft coloration still exists.

- Raw float hash **`d1322d462b822438`**.
- Raw peak **−14.46 dBFS**, active RMS **−24.57 dBFS**, preview gain **+.567 dB**.
- Apple M4 Max, two full passes at 48 kHz / 64: mean occupancy **2.31%**, p99
  **58.00 µs**, zero measured callbacks beyond **1,333.33 µs**. Includes rests and
  releases; not sustained max-polyphony, Logic/device-xrun or new F1 qualification.
  Open F1 96 kHz workload limits remain unchanged.

Program 27 appends as `Review / Basses / Crooked Velvet r2`; r1 remains at 26,
with original state/index/evidence intact. See `evaluation/CURRENT.md` for actual
installed build/trial. Compare the same notes, velocities and effects, including
ghost accents, overlaps and held notes: easier bass-note recognition **and** the
interesting elastic/loping character retained? Transport need not play. User
listening determines success; no automatic approval follows these checks.
