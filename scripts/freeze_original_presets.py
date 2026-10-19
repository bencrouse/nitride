#!/usr/bin/env python3
"""Freeze the four existing AU programs as inspectable definitions; never change DSP."""
import json
from pathlib import Path

import evaluation as ev
import factory_presets as fp


def main():
    destination = fp.ROOT / "presets/factory/originals"
    manifest = fp.ROOT / "presets/factory/originals.json"
    if destination.exists() or manifest.exists():
        raise SystemExit("Original definitions already frozen; never replace")
    scratch = fp.ROOT / "build/original-preset-freeze"
    scratch.mkdir(parents=True, exist_ok=False)
    definitions = []
    for program, (family, name) in enumerate((("soft-pad", "01 / Soft pad"), ("glass-keys", "02 / Glass keys"),
                                            ("contact-lead", "03 / Contact lead"), ("hard-hit", "04 / Hard hit"))):
        path = scratch / (family + ".aupreset")
        fp.native(["export", str(program), name, family, str(path)], fp.TOOL)
        import plistlib
        source = fp.document_from_native(plistlib.loads(path.read_bytes()))["current"]["patch"]
        source["id"] = "factory:" + str(program)
        assert source["name"] == name
        definitions.append((family, source, program))
    destination.mkdir()
    entries = []
    for family, source, program in definitions:
        path = destination / (family + ".nitridepreset")
        with path.open("x") as stream: stream.write(json.dumps(source, indent=2) + "\n")
        entries.append({"id": source["id"], "family": "original-" + family, "program": program,
                        "source": str(path.relative_to(fp.ROOT)), "source_sha256": ev.digest(path)})
    with manifest.open("x") as stream:
        stream.write(json.dumps({"format": "nitride-original-program-definitions", "version": 1, "entries": entries}, indent=2) + "\n")
    print("Frozen original programs0–3 from unchanged native AU state.")


if __name__ == "__main__": main()
