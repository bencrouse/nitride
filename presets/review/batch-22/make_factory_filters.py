#!/usr/bin/env python3
"""Selective factory-filter auditions; never rewrite a frozen source or promote a sound."""
import argparse
import copy
import json
from pathlib import Path
import sys
import uuid

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import factory_presets as fp

# The sixteen first-pass recommendations, not the eight conditional experiments.
# Amounts are listening hypotheses inside the reviewed ranges, not approvals.
PLAN = (
    ("rubber-relay", "Diode", .55, "Chewing resonant bass", "rounded low body, held FM motion and short-note bite"),
    ("tension-core", "Distortion", .40, "Focused dirty bass body", "settled pitch, legato glide and absence of the removed onset burp"),
    ("brown-console", "Tape", .40, "Rounded sustained organ body", "relative beating, tremolo and readable held clusters"),
    ("low-battery-piano", "Tape", .35, "Worn key with independent Tone articulation", "brief hammer, detuned ring and quiet detail"),
    ("copper-lift", "Softclip", .40, "Rounded independently articulated brass", "bright entrance, solid body and staggered chord entrances"),
    ("bent-aerial", "Transistor", .45, "Narrow off-center lead edge", "readable beating partials, drift and legato"),
    ("reed-snap", "Softclip", .45, "Independent resonant reed stabs", "snap, detached dyads and the existing triangle-shape refinement"),
    ("melted-ensemble", "Tape", .30, "Restrained dark sustained ensemble color", "slow beating motion, darkness and chord clarity"),
    ("reel-clav", "Transistor", .35, "Controlled woody rhythmic edge", "short dry body and empty rhythmic gaps"),
    ("taffy-coil", "Diode", .45, "Elastic independently filtered impacts", "opposed pitch bend, punch and readable overlaps"),
    ("oxide-rim", "Transistor", .30, "Short papery asymmetric clack", "dry transient and no added long ringing"),
    ("pulse-lattice", "Diode", .50, "Individually resonant sequence plucks", "short notes, original clock and stepped FM colors"),
    ("undertow-grid", "Diode", .45, "Resonant held-note rhythmic bed", "low weight, sustained bed and pulse gaps"),
    ("paper-motor", "Transistor", .40, "Small tactile papery motor", "changing ticks and modest stepped network strain"),
    ("pressure-lift", "Distortion", .50, "Strained opening pressure riser", "quiet start, rising trajectory and clean handoff"),
    ("shred-relay", "Distortion", .60, "Progressive signal breakup", "audible progression, steady AM clock and dry endpoint"),
)
CHARACTERS = ("Diode", "Distortion", "Softclip", "Tape", "Transistor")
# Freeze parent identities independently of whichever revision is factory-selected.
# Promotion must not make --verify generate the next revision from its own output.
PARENTS = {
    "rubber-relay": ("review:rubber-relay:r1", "Bass"),
    "tension-core": ("review:tension-core:r2", "Bass"),
    "brown-console": ("review:brown-console:r2", "Keys & Organs"),
    "low-battery-piano": ("review:low-battery-piano:r2", "Keys & Organs"),
    "copper-lift": ("review:copper-lift:r1", "Leads & Brass"),
    "bent-aerial": ("review:bent-aerial:r2", "Leads & Brass"),
    "reed-snap": ("review:reed-snap:r2", "Leads & Brass"),
    "melted-ensemble": ("review:melted-ensemble:r2", "Pads"),
    "reel-clav": ("review:reel-clav:r2", "Plucks & Mallets"),
    "taffy-coil": ("review:taffy-coil:r1", "Plucks & Mallets"),
    "oxide-rim": ("review:oxide-rim:r2", "Drums & Percussion"),
    "pulse-lattice": ("review:pulse-lattice:r2", "Sequences"),
    "undertow-grid": ("review:undertow-grid:r1", "Sequences"),
    "paper-motor": ("review:paper-motor:r1", "Textures"),
    "pressure-lift": ("review:pressure-lift:r1", "FX & Transitions"),
    "shred-relay": ("review:shred-relay:r1", "FX & Transitions"),
}


def encoded(data):
    return (json.dumps(data, indent=2) + "\n").encode()


def recipes():
    registry = fp.source_registry()
    result = []
    for family, character, amount, axis, preserve in PLAN:
        identity, host_category = PARENTS[family]
        parent = {**registry[identity], "category": host_category}; source = ROOT / parent["source"]
        original = json.loads(source.read_text())
        metadata = json.loads((source.parent / "batch.json").read_text())
        sketch = next(s for s in metadata["sketches"] if s["file"] == source.stem)
        old_revision = parent["id"].rsplit(":", 1)[1]
        revision = "r" + str(int(old_revision[1:]) + 1)
        definition = copy.deepcopy(original)
        definition["version"] = 7
        definition["id"] = uuid.uuid5(uuid.NAMESPACE_URL, "nitride:factory-filter-audition:" + family + ":" + revision).hex
        definition["name"] = original["name"].removesuffix(" " + old_revision) + " " + revision
        definition["values"].update(filter_family=1.0, rage_character=float(CHARACTERS.index(character)), rage_amount=amount)
        intent = (axis + ". Selected factory parent " + parent["id"] + "; change only filter family, character and amount. "
                  "Preserve " + preserve + ". Same frozen parent MIDI, source/FM/network/envelopes/resonance/pitch/Space/Output. "
                  "Audition hypothesis, not a factory replacement or listening approval. Tape is voice-source color, not a master tape processor.")
        result.append({"family": family, "revision": revision, "stem": family + "-" + revision,
                       "parent": parent, "source": source, "original": original, "definition": definition,
                       "sketch": {"file": family + "-" + revision, "category": sketch["category"],
                                  "host_category": parent["category"], "seconds": sketch["seconds"],
                                  "study_axis": axis, "intent": intent, "parent": parent["id"],
                                  "source_sha256": parent["source_sha256"], "character": character,
                                  "rage_amount": amount, "preserve": preserve}})
    # Two normal-size batches: seven r1->r2 and nine r2->r3 revisions.
    return sorted(result, key=lambda r: int(r["revision"][1:]))


def files(mode="rage"):
    if mode not in ("classic", "clean", "rage"):
        raise ValueError("Unknown comparison mode")
    result, batches = {}, {}
    for recipe in recipes():
        revision, stem = recipe["revision"], recipe["stem"]
        definition = copy.deepcopy(recipe["original"] if mode == "classic" else recipe["definition"])
        if mode == "clean": definition["values"]["rage_amount"] = 0.0
        result[revision + "/" + stem + ".nitridepreset"] = encoded(definition)
        result[revision + "/" + stem + ".mid"] = recipe["source"].with_suffix(".mid").read_bytes()
        batches.setdefault(revision, []).append(recipe["sketch"])
    for revision, sketches in batches.items():
        result[revision + "/batch.json"] = encoded({"format": "nitride-review-batch", "version": 1,
            "revision": revision if mode != "classic" else "r" + str(int(revision[1:]) - 1), "sketches": sketches})
    return result


def write_new(root, content):
    root = Path(root)
    if any((root / name).exists() or (root / name).is_symlink() for name in content):
        raise ValueError("Auditions exist; verify instead of overwriting frozen evidence")
    for name, data in content.items():
        target = root / name; target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream: stream.write(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--stage", type=Path, help="New private A/B/C comparison input folder")
    action.add_argument("--create", action="store_true", help="Freeze the proposed r2/r3 source files once")
    action.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    if args.stage:
        args.stage.mkdir(parents=True, exist_ok=False)
        for mode in ("classic", "clean", "rage"): write_new(args.stage / mode, files(mode))
    elif args.create:
        write_new(HERE, files())
    else:
        for name, data in files().items():
            if (HERE / name).read_bytes() != data: raise ValueError("Frozen factory-filter audition differs: " + name)
            print("Verified " + name)


if __name__ == "__main__": main()
