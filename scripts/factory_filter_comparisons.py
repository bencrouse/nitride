#!/usr/bin/env python3
"""Private full-MIDI Classic / clean Rage / chosen character comparisons, not listening approval."""
from array import array
import argparse
import json
import math
from pathlib import Path
import struct
import sys
import wave

MODES = ("classic", "clean", "rage")


def pcm(path):
    with wave.open(str(path), "rb") as reader:
        if (reader.getnchannels(), reader.getsampwidth(), reader.getframerate()) != (2, 3, 48000):
            raise ValueError("Expected stereo 48 kHz 24-bit review WAV")
        frames = reader.getnframes(); data = reader.readframes(frames)
    if len(data) != frames * 6: raise ValueError("Truncated review audio")
    return array("i", (int.from_bytes(data[n:n+3], "little", signed=True) for n in range(0, len(data), 3)))


def residual(a, b):
    if len(a) != len(b) or not a: raise ValueError("Comparison lengths differ or are empty")
    aa = sum(float(x)*x for x in a); bb = sum(float(x)*x for x in b)
    if min(aa, bb) <= 0: raise ValueError("Silent comparison")
    ab = sum(float(x)*y for x, y in zip(a, b))
    return 10 * math.log10(max(aa-ab*ab/bb, aa*1e-30) / aa)


def sequence(path, banks, gains):
    if len(banks) != 3 or len(gains) != 3: raise ValueError("Three comparison states required")
    gap = array("f", [0]) * 48000  # Half a second of stereo silence.
    size = (sum(len(bank) for bank in banks) + 2*len(gap))*4
    with Path(path).open("xb") as stream:
        stream.write(b"RIFF" + struct.pack("<I", size+36) + b"WAVEfmt "
                     + struct.pack("<IHHIIHH", 16, 3, 2, 48000, 384000, 8, 32) + b"data" + struct.pack("<I", size))
        for i, (bank, gain) in enumerate(zip(banks, gains)):
            if i: stream.write(gap.tobytes())
            values = array("f", (x/8388608*gain for x in bank))
            if sys.byteorder != "little": values.byteswap()
            stream.write(values.tobytes())


def compare(inputs, renders, output):
    inputs, renders, output = Path(inputs), Path(renders), Path(output)
    output.mkdir(parents=True, exist_ok=False)
    rows = []
    for revision in ("r2", "r3"):
        manifest = json.loads((inputs/"rage"/revision/"batch.json").read_text())
        reports = {mode: {r["name"]: r for r in json.loads((renders/mode/revision/"report.json").read_text())} for mode in MODES}
        for sketch in manifest["sketches"]:
            stem = sketch["file"]; metrics = {}; banks = []
            for mode in MODES:
                definition = json.loads((inputs/mode/revision/(stem+".nitridepreset")).read_text())
                metric = reports[mode][definition["name"]]
                if metric["faults"] != 0 or not metric["deterministic"] or metric["otherBlockMaximumError"] >= 1e-5:
                    raise ValueError("Failed deterministic/fault/block guard: " + stem)
                if any(not math.isfinite(metric[k]) for k in ("rawPeakDbfs", "rawActiveRmsDbfs", "p99Micros")):
                    raise ValueError("Nonfinite render evidence")
                metrics[mode] = metric
                banks.append(pcm(renders/mode/revision/(stem+"-raw.wav")))
            if len({len(bank) for bank in banks}) != 1: raise ValueError("MIDI performance lengths differ")
            target = min(-24, *(metric["rawActiveRmsDbfs"]-metric["rawPeakDbfs"]-3 for metric in metrics.values()))
            gains = [10**((target-metrics[mode]["rawActiveRmsDbfs"])/20) for mode in MODES]
            name = json.loads((inputs/"rage"/revision/(stem+".nitridepreset")).read_text())["name"].split(" / ")[-1]
            filename = name + " - Classic then clean Rage then " + sketch["character"] + ".wav"
            sequence(output/filename, banks, gains)
            rows.append({"preset": name, "parent": sketch["parent"], "character": sketch["character"], "amount": sketch["rage_amount"],
                "preserve": sketch["preserve"], "file": filename, "comparison_active_rms_target_dbfs": target,
                "classic_to_clean_gain_aligned_residual_db": residual(banks[0], banks[1]),
                "clean_to_character_gain_aligned_residual_db": residual(banks[1], banks[2]),
                "raw_level_change_db": metrics["rage"]["rawActiveRmsDbfs"]-metrics["classic"]["rawActiveRmsDbfs"],
                "metrics": metrics, "listening_approved": False})
    if len(rows) != 16: raise ValueError("Complete sixteen-preset comparison required")
    report = {"format": "nitride-factory-filter-comparisons", "version": 1, "listening_approved": False,
              "note": "Full identical original MIDI per state. Classic -> clean Rage -> proposed character, with half-second gaps. Common active-RMS target with >=3dB peak headroom, not LUFS. Residual includes routing/phase/response changes; not a quality or aliasing verdict. Raw levels, deadline misses and metrics retained.", "presets": rows}
    (output/"report.json").write_text(json.dumps(report, indent=2)+"\n")
    (output/"LISTEN.txt").write_text(report["note"]+"\nStart quietly. No factory defaults, projects or settings were changed.\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", type=Path); parser.add_argument("renders", type=Path); parser.add_argument("output", type=Path)
    args = parser.parse_args(); report = compare(args.inputs, args.renders, args.output)
    for row in report["presets"]:
        print(row["preset"], row["character"], row["amount"], "raw level delta", round(row["raw_level_change_db"], 2),
              "clean/character residual", round(row["clean_to_character_gain_aligned_residual_db"], 2))


if __name__ == "__main__": main()
