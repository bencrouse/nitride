#!/usr/bin/env python3
"""Level-match identical native AU Rage probes; never overwrite evaluation evidence.

Active-window RMS matching is not LUFS or a listening verdict. The native tool
supplies the same two-note performance and all settings except character/amount.
"""
import argparse
from array import array
import cmath
from functools import lru_cache
import json
import math
from pathlib import Path
import re
import struct
import sys


def read_probe(path):
    data = Path(path).read_bytes()
    if (data[:4], data[8:16], data[36:40]) != (b"RIFF", b"WAVEfmt ", b"data"):
        raise ValueError("Expected native probe WAV layout")
    fmt = struct.unpack("<IHHIIHH", data[16:36])
    if fmt != (16, 3, 2, 48000, 384000, 8, 32) or len(data) != 44 + 192000 * 8:
        raise ValueError("Expected four-second stereo 48 kHz float probe")
    samples = array("f"); samples.frombytes(data[44:])
    if sys.byteorder != "little": samples.byteswap()
    if not all(math.isfinite(x) for x in samples): raise ValueError("Non-finite probe")
    return samples


def active(samples):
    return [samples[2*i] for start, end in ((2400, 60000), (98400, 156000)) for i in range(start, end)]


def rms(values): return math.sqrt(sum(x*x for x in values) / len(values))
def db(value): return 20 * math.log10(max(value, 1e-15))


def residual(reference, candidate):
    """Remove a single optimal gain before reporting the remaining difference."""
    scale = sum(a*b for a, b in zip(reference, candidate)) / max(sum(b*b for b in candidate), 1e-30)
    return db(rms([a-scale*b for a, b in zip(reference, candidate)]) / max(rms(reference), 1e-15))


def write_probe(path, samples, gain):
    values = array("f", (x*gain for x in samples))
    if sys.byteorder != "little": values.byteswap()
    payload = values.tobytes()
    with Path(path).open("xb") as stream:
        stream.write(b"RIFF" + struct.pack("<I", len(payload)+36) + b"WAVEfmt "
                     + struct.pack("<IHHIIHH", 16, 3, 2, 48000, 384000, 8, 32)
                     + b"data" + struct.pack("<I", len(payload)) + payload)


def compare(baseline, candidate, output, baseline_label="eval-039", candidate_label="eval-040"):
    for label in (baseline_label, candidate_label):
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", label):
            raise ValueError("Build labels must be simple file-safe names")
    if baseline_label == "candidate" or baseline_label == candidate_label:
        raise ValueError("Build labels must be distinct and baseline cannot be candidate")
    banks = {name: {f"character-{c}-rage-{a}.wav": read_probe(folder / f"character-{c}-rage-{a}.wav")
                    for c in range(5) for a in (0, 50, 100)}
             for name, folder in ((baseline_label, Path(baseline)), ("candidate", Path(candidate)))}
    windows = {name: {file: active(samples) for file, samples in bank.items()} for name, bank in banks.items()}
    rows = []
    for name, bank in windows.items():
        for c in range(5):
            clean = bank[f"character-{c}-rage-0.wav"]
            rows.append({"build": name, "character": c, "halfResidualDb": residual(clean, bank[f"character-{c}-rage-50.wav"]),
                         "fullResidualDb": residual(clean, bank[f"character-{c}-rage-100.wav"])})
    pairs = [{"characters": [a, b], "residualDb": residual(windows["candidate"][f"character-{a}-rage-100.wav"], windows["candidate"][f"character-{b}-rage-100.wav"])}
             for a in range(5) for b in range(a)]
    clean_error = max(abs(a-b) for c in range(5) for a, b in zip(banks[baseline_label][f"character-{c}-rage-0.wav"], banks["candidate"][f"character-{c}-rage-0.wav"]))
    # One shared target, reduced if necessary to retain peak headroom in every file.
    target = min(10**(-24/20), *(10**(-1/20)*rms(windows[name][file])/max(abs(x) for x in samples)
                                  for name, bank in banks.items() for file, samples in bank.items()))
    output = Path(output); output.mkdir(parents=True, exist_ok=False)
    for name, bank in banks.items():
        (output/name).mkdir()
        for file, samples in bank.items(): write_probe(output/name/file, samples, target/rms(windows[name][file]))
    (output/"listen").mkdir()
    silence = array("f", [0]) * 48000  # Half a second of stereo silence between examples.
    for c, label in enumerate(("Diode", "Distortion", "Softclip", "Tape", "Transistor")):
        for title, sequence in (("0 then 50 then 100", [("candidate", a) for a in (0, 50, 100)]),
                                (f"{baseline_label} then {candidate_label} full", [(baseline_label, 100), ("candidate", 100)])):
            presentation = array("f")
            for i, (name, amount) in enumerate(sequence):
                file = f"character-{c}-rage-{amount}.wav"
                if i: presentation.extend(silence)
                gain = target/rms(windows[name][file])
                presentation.extend(x*gain for x in banks[name][file])
            write_probe(output/"listen"/(label + " - " + title + ".wav"), presentation, 1)
    report = {"format": "nitride-rage-level-matched-comparison", "version": 1,
              "baselineBuild": baseline_label, "candidateBuild": candidate_label,
              "note": "Identical native two-note source/MIDI/settings. Active-window RMS, not LUFS. Residual removes constant gain; differences do not establish perceived character quality.",
              "targetActiveRmsDbfs": db(target), "cleanMaximumError": clean_error, "amountComparisons": rows, "fullCharacterPairs": pairs}
    (output/"report.json").write_text(json.dumps(report, indent=2)+"\n")
    return report


def require_strong_range(report):
    if report["cleanMaximumError"] > 1e-7: raise ValueError("Zero-Rage audio changed")
    for row in report["amountComparisons"]:
        if row["build"] == "candidate" and (row["halfResidualDb"] < -20 or row["fullResidualDb"] < -12
                                                or row["fullResidualDb"] < row["halfResidualDb"] + 1):
            raise ValueError("Controlled Rage range regressed towards a subtle gain change")
    if any(p["residualDb"] < -34 for p in report["fullCharacterPairs"]):
        raise ValueError("Controlled characters collapsed after level alignment")


def study_metrics(baseline, candidate, live):
    banks = {label: {(c, a): read_probe(Path(folder)/f"character-{c}-rage-{a}.wav")
                     for c in range(5) for a in (0, 50, 100)}
             for label, folder in (("old", baseline), ("new", candidate), ("live", live))}
    def error(a, b): return max(abs(x-y) for x, y in zip(a, b))
    rows = []
    for c in range(5):
        old_clean, old_full = (active(banks["old"][c, a]) for a in (0, 100))
        clean, half, full = (active(banks["new"][c, a]) for a in (0, 50, 100))
        rows.append({"character": c, "oldFullResidualDb": residual(old_clean, old_full),
                     "halfResidualDb": residual(clean, half), "fullResidualDb": residual(clean, full),
                     "fullLevelDeltaDb": db(rms(full)/rms(clean))})
    return {"note": "Unchanged Diode Current r2; only character/amount vary. Live switch at 0.5s of a held note. Numerical goals, not perceived-quality approval.",
            "diodeMaximumError": max(error(banks["old"][0, a], banks["new"][0, a]) for a in (0, 50, 100)),
            "zeroRageMaximumError": max(error(banks["old"][c, 0], banks["new"][c, 0]) for c in range(5)),
            "preSwitchMaximumError": max(error(banks["live"][c, a][:48000], banks["new"][0, a][:48000]) for c in range(5) for a in (0, 50, 100)),
            "settledSwitchMaximumError": max(error(banks["live"][c, a][96000:120000], banks["new"][c, a][96000:120000]) for c in range(5) for a in (0, 50, 100)),
            "characters": rows}


def require_study_range(report, require_improvement=True):
    if any(report[key] != 0 for key in ("diodeMaximumError", "zeroRageMaximumError", "preSwitchMaximumError")):
        raise ValueError("Accepted Diode, clean audio or pre-switch audio changed")
    if report["settledSwitchMaximumError"] > 3e-6:
        raise ValueError("Held-note character switching did not settle to the selected model")
    for row in report["characters"]:
        if row["character"] == 0: continue  # Preserve the sound accepted by the user.
        if row["fullResidualDb"] < -20 or row["halfResidualDb"] < -28 or (require_improvement and row["fullResidualDb"] < row["oldFullResidualDb"] + 3):
            raise ValueError("Study character remains too weak in the unchanged quiet source")
        if row["fullLevelDeltaDb"] < -3:
            raise ValueError("Study character collapsed source energy")


@lru_cache(maxsize=2)
def _fft_plan(size):
    if size < 2 or size & (size-1): raise ValueError("FFT size must be a power of two")
    bits = size.bit_length()-1
    indices = [int(f"{n:0{bits}b}"[::-1], 2) for n in range(size)]
    stages = [(1 << p, [cmath.exp(-2j*math.pi*k/(1 << p)) for k in range(1 << (p-1))]) for p in range(1, bits+1)]
    return indices, stages


def _fft(samples):
    indices, stages = _fft_plan(len(samples))
    values = [complex(samples[n]) for n in indices]
    for length, roots in stages:
        half = length//2
        for base in range(0, len(values), length):
            for k, root in enumerate(roots):
                left = values[base+k]; right = root*values[base+k+half]
                values[base+k] = left+right; values[base+k+half] = left-right
    return values


def magnitude_profile(samples):
    """Welch magnitude profile; ignore phase, DC and frequencies above 20 kHz."""
    size = 4096; window = [.5-.5*math.cos(2*math.pi*n/(size-1)) for n in range(size)]
    power = [0.0]*(size//2+1); count = 0
    for start, end in ((2400, 60000), (98400, 156000)):
        for offset in range(start, end-size+1, size):
            bins = _fft([samples[2*(offset+n)]*window[n] for n in range(size)])
            for k in range(len(power)): power[k] += abs(bins[k])**2
            count += 1
    return [math.sqrt(power[k]/count) for k in range(4, int(20000*size/48000)+1)]


def character_contrast(baseline, candidate):
    banks = {label: [magnitude_profile(read_probe(Path(folder)/f"character-{c}-rage-100.wav")) for c in range(5)]
             for label, folder in (("old", baseline), ("new", candidate))}
    pairs = [{"characters": [a, b], "oldSpectralResidualDb": residual(banks["old"][a], banks["old"][b]),
              "spectralResidualDb": residual(banks["new"][a], banks["new"][b])} for a in range(5) for b in range(a)]
    revised = [p for p in pairs if 0 not in p["characters"]]
    return {"note": "Gain-aligned magnitude spectra, not waveform phase or a listening verdict. Same-source full Rage; revised-pair aggregate excludes unchanged Diode.",
            "pairs": pairs, "revisedMeanSeparationIncreaseDb": sum(p["spectralResidualDb"]-p["oldSpectralResidualDb"] for p in revised)/len(revised),
            "revisedMinimumSpectralResidualDb": min(p["spectralResidualDb"] for p in revised)}


def require_character_contrast(report):
    if report["revisedMeanSeparationIncreaseDb"] < 2 or report["revisedMinimumSpectralResidualDb"] < -28:
        raise ValueError("Character magnitude spectra remain insufficiently separated")


def character_sequences(output, baseline_label, candidate_label):
    output = Path(output); silence = array("f", [0])*48000
    for folder, label in ((baseline_label, baseline_label), ("candidate", candidate_label)):
        audio = array("f")
        for c in range(5):
            if c: audio.extend(silence)
            audio.extend(read_probe(output/folder/f"character-{c}-rage-100.wav"))
        write_probe(output/"listen"/f"All characters - {label}.wav", audio, 1)
    with (output/"listen/character-order.txt").open("x") as stream:
        stream.write("Diode -> Distortion -> Softclip -> Tape -> Transistor. Four seconds each, half-second gaps. Same-source full Rage, common RMS target. Not a perceptual-quality verdict.\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path); parser.add_argument("candidate", type=Path); parser.add_argument("output", type=Path)
    parser.add_argument("--require-strong-range", action="store_true", help="Check explicit numerical range goals, not perceptual approval")
    parser.add_argument("--baseline-label", default="eval-039"); parser.add_argument("--candidate-label", default="eval-040")
    parser.add_argument("--study-live", type=Path, help="Verify the unchanged Diode Current and held-note switching captures")
    parser.add_argument("--retain-audibility", action="store_true", help="Keep existing study audibility floors without demanding every character becomes stronger")
    parser.add_argument("--character-contrast", action="store_true", help="Check gain-aligned spectral separation between characters")
    parser.add_argument("--character-sequence", action="store_true", help="Also concatenate all five full-Rage characters at the matched level")
    args = parser.parse_args(); report = compare(args.baseline, args.candidate, args.output, args.baseline_label, args.candidate_label)
    if args.require_strong_range: require_strong_range(report)
    if args.study_live:
        study = study_metrics(args.baseline, args.candidate, args.study_live)
        with (args.output/"study-verification.json").open("x") as stream: stream.write(json.dumps(study, indent=2)+"\n")
        require_study_range(study, require_improvement=not args.retain_audibility)
    if args.character_contrast:
        contrast = character_contrast(args.baseline, args.candidate)
        with (args.output/"character-contrast.json").open("x") as stream: stream.write(json.dumps(contrast, indent=2)+"\n")
        require_character_contrast(contrast)
    if args.character_contrast or args.character_sequence:
        character_sequences(args.output, args.baseline_label, args.candidate_label)
    print(json.dumps(report, indent=2))


if __name__ == "__main__": main()
