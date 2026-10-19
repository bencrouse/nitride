#!/usr/bin/env python3
"""Generate curated metadata and a narrowly adapted COPY of the pinned JUCE AU wrapper."""
import argparse
import hashlib
import json
from pathlib import Path

import factory_presets as fp
import evaluation as ev

WRAPPER_SHA256 = "d6b4d9016335df4d92b829bba4d7ca99f60588a0a00b145033befeb66956d095"


def header(selection=fp.SELECTION):
    # Build archives intentionally omit the personal evaluation journal. Building
    # validates frozen source identities; publishing additionally requires feedback.
    _, entries = fp.validate_selection(selection, require_feedback=False)
    registry = ev.validate_builtins()["entries"]
    legacy = ["01 / Soft pad", "02 / Glass keys", "03 / Contact lead", "04 / Hard hit"]
    legacy += [json.loads((fp.ROOT / "presets/review" / e["preset"]).read_text())["name"] for e in registry]
    quote = lambda value: json.dumps(value, ensure_ascii=True)
    lines = ["// Generated from the separate release selection; historical numeric programs never move.",
             "#pragma once", "#include <array>", "namespace Nitride", "{",
             "struct FactoryProgram { int program; const char* id; const char* name; const char* category; };",
             f"inline constexpr std::array<FactoryProgram, {len(entries)}> factoryPrograms {{{{"]
    for entry in entries:
        lines.append("    {" + str(entry["program"]) + ", " + ", ".join(quote(entry[k]) for k in ("id", "name", "category")) + "},")
    lines += ["}};", f"inline constexpr std::array<const char*, {len(legacy)}> legacyProgramNames {{{{"]
    lines += ["    " + quote(name) + "," for name in legacy]
    lines += ["}};", "inline const char* factoryProgramName(int program) noexcept", "{",
              "    for(const auto& entry:factoryPrograms)if(entry.program==program)return entry.name;",
              "    return nullptr;", "}", "}", ""]
    return "\n".join(lines)


def adapt_wrapper(data):
    if hashlib.sha256(data).hexdigest() != WRAPPER_SHA256:
        raise ValueError("AU wrapper differs from pinned JUCE 8.0.15; review adapter explicitly, never patch an unknown dependency")
    text = data.decode()
    # Keep the vendor checkout/SDK/DSP untouched. Preserve original license in the copy.
    text = text.replace("#include <set>", '#include <set>\n#include "FactoryPrograms.h"', 1)
    first = text.index("    ComponentResult GetPresets (CFArrayRef* outData) const override")
    last = text.index("    OSStatus NewFactoryPresetSet", first)
    text = text[:first] + """    // Nitride: categorized .aupreset files are the host browser. Advertising a
    // second factory list makes Logic add a duplicate "AU Presets" submenu.
    // PresentPreset still accepts historical numbers independently of this API.
    ComponentResult GetPresets (CFArrayRef* outData) const override
    {
        if (outData != nullptr)
            *outData = nullptr;
        return kAudioUnitErr_InvalidProperty;
    }

""" + text[last:]
    text = text.replace("if (chosenPresetNumber >= numPrograms)", "if (chosenPresetNumber < 0 || chosenPresetNumber >= numPrograms)", 1)
    text = text.replace("chosenPreset.presetName = juceFilter->getProgramName (chosenPresetNumber).toCFString();", """const auto* displayName = Nitride::factoryProgramName (chosenPresetNumber);
        chosenPreset.presetName = (displayName != nullptr ? String (displayName) : juceFilter->getProgramName (chosenPresetNumber)).toCFString();""", 1)
    text = text.replace("const String currentProgramName = juceFilter->getProgramName (currentProgramNumber);", """const auto* displayName = Nitride::factoryProgramName (currentProgramNumber);
        const String currentProgramName = displayName != nullptr ? String (displayName) : juceFilter->getProgramName (currentProgramNumber);""", 1)
    return "// Nitride generated AU visibility/name adapter; original source checksum verified.\n" + text


def write_changed(path, text):
    path = Path(path)
    data = text.encode()
    # Vendor sources use CRLF. Text-mode reads normalize them and would rewrite
    # an identical generated wrapper on every configure, needlessly relinking AU.
    if not path.exists() or path.read_bytes() != data:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wrapper", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    write_changed(args.output / "FactoryPrograms.h", header())
    write_changed(args.output / "NitrideAuWrapper.mm", adapt_wrapper(args.wrapper.read_bytes()))


if __name__ == "__main__": main()
