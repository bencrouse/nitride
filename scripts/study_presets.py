#!/usr/bin/env python3
"""Publish explicit native audition files separately from the curated factory catalog."""
import argparse
import json
import os
from pathlib import Path
import shutil
import tempfile

import evaluation as ev
import factory_presets as fp


def export(output, batch, trial_id, study_id):
    ev.identifier(study_id)
    batch = Path(batch).resolve(); output = Path(output).absolute()
    state = ev.project_state(ev.read_events(fp.ROOT / "evaluation/events.jsonl"))
    trial = state["trials"][trial_id]; build = state["builds"][trial["build"]]
    for relative, expected in build["bundle_hashes"].items():
        if ev.digest(Path(build["installed_path"]) / relative) != expected: raise ValueError("Installed AU differs from trial build")
    metadata = json.loads((batch / "batch.json").read_text())
    if metadata.get("format") != "nitride-review-batch" or metadata.get("version") != 1 or not 1 <= len(metadata["sketches"]) <= 12:
        raise ValueError("Invalid study batch")
    registry = {e["id"]: e for e in ev.validate_builtins()["entries"]}
    selection, selected = fp.validate_selection(); selected = {e["id"] for e in selected}
    categories = set(selection["categories"])
    requests = []
    for sketch in metadata["sketches"]:
        source = batch / (fp.component_name(sketch["file"]) + ".nitridepreset")
        matches = [e for e in registry.values() if (fp.ROOT / "presets/review" / e["preset"]).resolve() == source]
        if len(matches) != 1: raise ValueError("Study source is not registered")
        entry = matches[0]
        if entry["id"] not in trial["revisions"] or entry["id"] not in state["revisions"] or entry["id"] in selected:
            raise ValueError("Study must be in the explicit trial and outside the selected factory catalog")
        if sketch["host_category"] not in categories: raise ValueError("Unknown host category")
        requests.append((sketch, entry, source))
    output.mkdir(parents=True, exist_ok=False)
    manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": "study",
                "status": "awaiting-logic-category-confirmation", "study": study_id, "trial": trial_id,
                "installed_build": build["id"], "selection": "not-factory-selected", "entries": [], "files": {}}
    for sketch, entry, source in requests:
        definition = json.loads(source.read_text())
        label = definition["name"].split(" / ")[-1]; suffix = " " + entry["revision"]
        if not label.endswith(suffix): raise ValueError("Source name/revision differs from registry")
        name = "Study - " + fp.component_name(label.removesuffix(suffix))
        relative = sketch["host_category"] + "/" + name + ".aupreset"; path = output / relative
        path.parent.mkdir(exist_ok=True)
        frozen = "sources/" + entry["family"] + ".nitridepreset"
        (output / "sources").mkdir(exist_ok=True); shutil.copy2(source, output / frozen)
        fp.native(["export", str(entry["program"]), definition["name"], name, str(path)], fp.TOOL)
        native_entry = {"id": entry["id"], "program": entry["program"], "name": name, "category": sketch["host_category"], "file": relative,
                        "source": str(source.relative_to(fp.ROOT)), "source_sha256": ev.digest(source), "frozen_source": frozen}
        fp.validate_native(path, native_entry, definition)
        metrics = json.loads(fp.native(["verify", str(path), str(entry["program"]), definition["name"]], fp.TOOL))
        manifest["entries"].append({**native_entry, "native_verification": metrics})
        manifest["files"][relative] = ev.digest(path); manifest["files"][frozen] = ev.digest(output / frozen)
    with (output / "manifest.json").open("x") as stream: stream.write(json.dumps(manifest, indent=2) + "\n")
    fp.verify_package(output)
    return manifest


def install(package, destination=fp.FACTORY_DESTINATION):
    package, destination = Path(package), Path(destination).absolute()
    manifest = fp.verify_package(package)
    if manifest.get("scope") != "study" or manifest.get("selection") != "not-factory-selected": raise ValueError("Not an unapproved study package")
    study = ev.identifier(manifest["study"])
    if any(p.is_symlink() for p in (destination, *destination.parents)): raise ValueError("Refuse linked destination")
    files = {e["file"]: manifest["files"][e["file"]] for e in manifest["entries"]}
    if any(not Path(p).name.startswith("Study - ") for p in files): raise ValueError("Study labels must remain explicit")
    receipt = {"format": "nitride-study-preset-install", "version": 1, "study": study, "trial": manifest["trial"],
               "installed_build": manifest["installed_build"], "status": "awaiting-feedback-not-factory-selected",
               "package_sha256": ev.digest(package / "manifest.json"), "files": files}
    receipt_path = destination / (".nitride-study-" + study + ".json")
    if receipt_path.exists() or receipt_path.is_symlink():
        previous = json.loads(fp.package_file(destination, receipt_path.name).read_text())
        if previous == receipt and all(ev.digest(fp.package_file(destination, p)) == h for p, h in files.items()): return receipt
        raise ValueError("Study receipt/files already exist or were edited; never overwrite")
    for relative in files:
        path = destination / relative
        if path.exists() or path.is_symlink(): raise ValueError("User preset collision: " + relative)
        if not path.parent.is_dir() or path.parent.is_symlink(): raise ValueError("Expected an existing safe category")
    created = []
    try:
        with tempfile.TemporaryDirectory(prefix=".nitride-study-stage-", dir=destination) as temporary:
            stage = Path(temporary)
            for i, (relative, expected) in enumerate(files.items()):
                path = stage / str(i); shutil.copy2(fp.package_file(package, relative), path); ev.durable(path)
                if ev.digest(path) != expected: raise ValueError("Staged checksum mismatch")
            staged_receipt = stage / "receipt.json"; staged_receipt.write_text(json.dumps(receipt, indent=2) + "\n"); ev.durable(staged_receipt)
            for i, (relative, expected) in enumerate(files.items()):
                path = destination / relative; os.link(stage / str(i), path); created.append((path, expected))
            os.link(staged_receipt, receipt_path)
    except Exception:
        for path, expected in reversed(created):
            if path.is_file() and not path.is_symlink() and ev.digest(path) == expected: path.unlink()
        raise
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path); parser.add_argument("--batch", type=Path, required=True)
    parser.add_argument("--trial", required=True); parser.add_argument("--study", required=True); parser.add_argument("--install", action="store_true")
    args = parser.parse_args(); manifest = export(args.output, args.batch, args.trial, args.study)
    if args.install: install(args.output)
    print("Prepared", len(manifest["entries"]), "explicit Study files; selected factory catalog unchanged.")


if __name__ == "__main__": main()
