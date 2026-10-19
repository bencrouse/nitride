#!/usr/bin/env python3
"""Name-only Tine Keys continuation; every parent sound condition/MIDI byte stays intact."""
import argparse
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parent / "r1/cutoff-alloy-r1.nitridepreset"


def files():
    preset = json.loads(SOURCE.read_text())
    preset["id"] = uuid.uuid5(uuid.NAMESPACE_URL, "nitride:rename:tine-keys:r2").hex
    preset["name"] = "Review / Physical materials / Tine Keys r2"
    manifest = {"format": "nitride-review-batch", "version": 1, "revision": "r2", "sketches": [
        {"file": "tine-keys-r2", "category": "Physical materials", "seconds": 20,
         "intent": "Name-only continuation of liked Cutoff Alloy as Tine Keys after user hears an electric-piano role. Every parameter, mono/chord and frozen MIDI byte unchanged; internal cutoff-alloy lineage/old index retained. Not a specific commercial instrument model."}]}
    return {"tine-keys-r2.nitridepreset": (json.dumps(preset, indent=2) + "\n").encode(),
            "tine-keys-r2.mid": SOURCE.with_suffix(".mid").read_bytes(),
            "batch.json": (json.dumps(manifest, indent=2) + "\n").encode()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    content = files()
    if not args.verify and any((ROOT / name).exists() for name in content):
        raise SystemExit("Rename files already exist; use --verify, never replace frozen revisions")
    for name, data in content.items():
        path = ROOT / name
        if args.verify:
            if path.read_bytes() != data:
                raise SystemExit("Name-only continuation differs: " + name)
        else:
            with path.open("xb") as stream:
                stream.write(data)
        print(("Verified " if args.verify else "Created ") + name)


if __name__ == "__main__":
    main()
