# Batch14 / r1 — classic DX7 factory-voice approximations

Original Nitride recipes grounded in ROM1A EU/JP **E.PIANO1, BASS1, TUB BELLS**,
not imported six-operator programs or a new algorithm engine. See
[source data, software-reference renders and A/B/C packet](../../../../evaluation/dx7-classics/pass-001/README.md).
Factory data and offline model code are not embedded/linked into the plugin.

| Appended program0-based | Nitride voice | Source brief | Existing comparison |
| ---: | --- | --- | --- |
|54 |Classic Tines / Physical materials | E.PIANO1 | Tine Keys r2 |
|55 |Classic Bass / Basses | BASS1 | Pocket Slap r1 |
|56 |Classic Tubes / Physical materials | TUB BELLS | Hollow Bronze r2 |

## What to play

**Classic Tines:** MIDI60 at soft/medium/hard velocity, middle-register chords and
runs, then low held keys. The1:1 FM body decays over time while a separate short
filter-envelope-to-shape strike supplies an attack. Tone-envelope amount stays0;
dry, low coupling/zero stress, no pitch motion. Original EP has three parallel
detuned pairs including14:1 attack beyond Nitride's8ratio ceiling: that layer,
beating, six EGs and operator-specific touch response cannot be copied exactly.

**Classic Bass:** MIDI36–48 with ghosts/stops/holds. Clear1:1 body and brief harder
index/shape strike; dry mono/retriggered, no bend/glide. Source algorithm16 has a
complex three-branch modulator structure/feedback. Its .5carrier plus ROM−12
transpose sounds24st below input. **This preset maps root to the played note**;
only the external reference MIDI is shifted+24 for sounding-pitch comparison,
not a claim to duplicate the source keyboard map or six-op velocity/key scaling.

**Classic Tubes:** sparse MIDI48/60/67 hits, then dyads. Nominal3.5:1 sine FM with
slowly relaxing index,1.8s amplitude decay/1.6s release; no Space/bend decoration.
The source has two detuned pairs and a fixed~323.6Hz carrier strike; no independent
fixed-frequency layer is invented in Nitride. Listen for enough recognizable bell
identity and useful contrast with Hollow Bronze, not just generic metallic ringing.

No transport requirement. MIDI octave labels vary by host; numbers above are MIDI
notes. Short gates still enter release, not a hidden one-shot mode. Shared Tone
velocity response is only an approximation of source brightness response.

## Frozen performance and evidence /0.7.22, eval-022

`make_classics.py --verify` reproduces3complete presets/3original MIDI/manifest;
creation refuses existing targets. 100BPM/26s keys,108BPM/24s bass,90BPM/36s bells.
Each has velocity-varied repeated notes and role-appropriate phrases/register/held
tests. B/C comparisons use identical MIDI and unchanged old definitions/IDs/outputs;
old frozen original performances stay intact. Source-model A has the documented
bass offset,44100Hz model-native output and <=32/44100s keyup quantization.

Final own production processor exports48kHz stereo24-bit: identity/conditions
survive export/reimport; exact same-block repeats and zero64/257 differences.
No faults/master-ceiling hits, completed amplitude release and finalsecond RMS at
the−100dBFS reporting floor. All6kept-anchor comparison captures also pass exactly.

| Candidate | Raw peak dBFS | Active RMS dBFS | Preview gain dB | Voices | Raw float hash |
| --- | ---: | ---: | ---: | ---: | --- |
|Classic Tines |−8.42 |−23.99 |−.012 |4 |`f6baf7033abaa351` |
|Classic Bass |−12.80 |−24.10 |+.100 |1 |`63b50322d983a655` |
|Classic Tubes |−11.01 |−24.06 |+.057 |2 |`fdbffc5a2c6db3a5` |

Only output was calibrated before freezing: initial−6dB becomes−4.7/−1.7/−3.4dB.
Pre-registration eval-021 captures are preserved separately, not relabeled final.
Previews target−24dBFS gated100ms active RMS, not LUFS/perceptual matching; gain
does not alter stored output. Source references use separately documented static
capture attenuation/normalization, not calibrated hardware levels.

Apple M4 Max, two complete own-demo passes48kHz/64: mean occupancy3.13/1.61/2.40%,
p99175.21/48.42/95.92µs, zero measured overdue callbacks against1,333.33µs. Includes
rests/tails and at most4/1/2voices, not sustained max-polyphony/device-xrun/cross-rate
or renewed full sanitizer qualification. F1 combined96kHz and Socket-new120 context
remain open; no algorithm, host contract, global curve, quality/cap change or repair.

Three approximations await actual preference/likeness/overlap judgment, not factory
approval or proof these are the closest possible settings.37earlier families kept;
40families total,53review revisions and4original factory programs. No commits/pushes.
