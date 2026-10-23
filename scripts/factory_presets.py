#!/usr/bin/env python3
"""Local categorized AU-preset checkpoint. Never changes the plugin, program table or user sounds."""
import argparse
import json
import os
from pathlib import Path
import plistlib
import shutil
import struct
import subprocess
import tempfile
import unicodedata
import xml.etree.ElementTree as ET

import evaluation as ev

ROOT = Path(__file__).resolve().parents[1]
SELECTION = ROOT / "presets/factory/candidate-001.json"
TOOL = ROOT / "build/local-release/nitride_au_presets"
DESTINATION = Path.home() / "Library/Audio/Presets/nitride/nitride/Nitride Preview"
RECEIPT = ".nitride-preview.json"
FACTORY_DESTINATION = DESTINATION.parent
FACTORY_RECEIPT = ".nitride-factory.json"


def source_registry(project=ROOT, check_history=True):
    """Release metadata can select originals without moving the append-only review table."""
    project = Path(project)
    registry = {e["id"]: {**e, "source": "presets/review/" + e["preset"], "source_sha256": e["sha256"]}
                for e in ev.validate_builtins(project, check_history=check_history)["entries"]}
    originals = json.loads((project / "presets/factory/originals.json").read_text())
    if originals.get("format") != "nitride-original-program-definitions" or originals.get("version") != 1:
        raise ValueError("Invalid original program definitions")
    if [e["id"] for e in originals["entries"]] != ["factory:" + str(i) for i in range(4)]:
        raise ValueError("Original program identities must remain0–3")
    for program, entry in enumerate(originals["entries"]):
        if entry["program"] != program or ev.digest(package_file(project, entry["source"])) != entry["source_sha256"]:
            raise ValueError("Original program definition changed")
        registry[entry["id"]] = entry
    return registry


def acceptance(project=ROOT):
    data = json.loads((Path(project) / "presets/factory/acceptance.json").read_text())
    if (data.get("format"), data.get("version")) != ("nitride-factory-acceptance", 1):
        raise ValueError("Invalid portable factory acceptance")
    if not data.get("user_feedback") or len(data["retained"]) != len(set(data["retained"])):
        raise ValueError("Missing explicit inclusion feedback or duplicate accepted identities")
    return data


def component_name(value):
    if not isinstance(value, str) or not value or value != value.strip() or value in (".", ".."):
        raise ValueError("Empty or unsafe preset/category name")
    if any(c in value for c in '/\\:\0') or any(ord(c) < 32 for c in value) or len(value) > 100:
        raise ValueError("Unsafe preset/category name: " + value)
    return value


def validate_selection(path=SELECTION, project=ROOT, require_feedback=True, check_history=True):
    selection = json.loads(Path(path).read_text())
    if (selection.get("format"), selection.get("version"), selection.get("status")) != (
            "nitride-release-selection", 1, "proposed-not-release-approved"):
        raise ValueError("Not a proposed release selection")
    ev.identifier(selection["id"])
    categories = [component_name(c) for c in selection["categories"]]
    if len(set(categories)) != len(categories):
        raise ValueError("Duplicate categories")
    registry = source_registry(project, check_history=check_history)
    retained = set(acceptance(project)["retained"]) if require_feedback else set()
    ids, families, names, entries = set(), set(), set(), []
    for item in selection["entries"]:
        source = registry.get(item["id"])
        if not source or (require_feedback and item["id"] not in retained):
            raise ValueError("Selection requires explicit retained-direction feedback: " + item["id"])
        if item["id"] in ids or source["family"] in families:
            raise ValueError("Only one preferred revision per family is browsable")
        name = component_name(item["name"])
        if item["category"] not in categories:
            raise ValueError("Unknown release category")
        key = unicodedata.normalize("NFC", name).casefold()
        if key in names:
            raise ValueError("Duplicate release name")
        ids.add(item["id"]); families.add(source["family"]); names.add(key)
        entries.append({**item, "program": source["program"], "family": source["family"],
                         "source": source["source"], "source_sha256": source["source_sha256"]})
    if not entries or not selection["pilot"] or len(set(selection["pilot"])) != len(selection["pilot"]) or not set(selection["pilot"]) <= ids:
        raise ValueError("Pilot must select unique retained entries")
    pending = selection["pending_feedback"]
    if len(set(pending)) != len(pending) or set(pending) & ids or any(p not in registry for p in pending):
        raise ValueError("Pending voices must be known, unique and excluded from the proposed release")
    return selection, entries


def document_from_native(preset):
    identity = {"type": int.from_bytes(b"aumu", "big"), "subtype": int.from_bytes(b"NT01", "big"),
                "manufacturer": int.from_bytes(b"NTRD", "big"), "version": 0}
    if not isinstance(preset, dict) or any(preset.get(k) != v for k, v in identity.items()):
        raise ValueError("Not a Nitride AU ClassInfo preset")
    data = preset.get("jucePluginState")
    if not isinstance(data, bytes) or len(data) < 10:
        raise ValueError("Missing full Nitride plugin state")
    magic, length = struct.unpack("<II", data[:8])
    if magic != 0x21324356 or length != len(data) - 9 or data[-1] != 0:
        raise ValueError("Invalid JUCE XML state envelope")
    root = ET.fromstring(data[8:8 + length])
    if root.tag != "NITRIDE_INSTRUMENT" or root.get("version") != "7":
        raise ValueError("Unsupported Nitride session XML")
    document = json.loads(root.findtext("DOCUMENT", ""))
    if document.get("format") != "nitride-session" or document.get("version") != 7:
        raise ValueError("Unsupported Nitride session document")
    if any(document.get(field) != [] for field in ("undo", "redo", "kept")):
        raise ValueError("Factory presets must not contain personal sounds or edit history")
    return document


def validate_native(path, entry, source):
    preset = plistlib.loads(Path(path).read_bytes())
    document = document_from_native(preset)
    current = document["current"]
    if preset.get("name") != entry["name"] or current["selected"] != entry["id"] or current["program"] != entry["program"]:
        raise ValueError("Native label or historical preset identity changed")
    if current.get("wholePatch") is not True or current.get("parameters") != []:
        raise ValueError("Factory state must be a clean whole-patch reference")
    for field in ("patch", "reference"):
        expected = {**source, "id": "snapshot" if field == "patch" else "reference"}
        if current[field] != expected:
            raise ValueError("Native preset differs from its exact source conditions/Compare reference")
    return document


def native(command, tool, component=None):
    # AudioComponent discovery needs the GUI user's bootstrap namespace, like auval.
    explicit = ["--component", str(Path(component).resolve())] if component is not None else []
    return subprocess.run(["launchctl", "asuser", str(os.getuid()), str(Path(tool).resolve()), *explicit, *command],
                           check=True, capture_output=True, text=True).stdout


def component_build(component):
    """Identify a source install from the bundle itself, without the private journal."""
    component = Path(component).absolute()
    info = plistlib.loads(package_file(component, "Contents/Info.plist").read_bytes())
    if info.get("CFBundleIdentifier") != "com.nitride.audio.nitride":
        raise ValueError("Not a Nitride component bundle")
    executable = component_name(info["CFBundleExecutable"])
    binary = package_file(component, "Contents/MacOS/" + executable)
    binary_hash = ev.digest(binary)
    return {"id": "source-" + binary_hash[:16], "version": info["CFBundleShortVersionString"],
            "installed_path": str(component), "binary_sha256": binary_hash,
            "bundle_hashes": {str(p.relative_to(component)): ev.digest(package_file(component, p.relative_to(component)))
                              for p in sorted(component.rglob("*")) if p.is_file()}}


def export_package(output, scope="pilot", tool=TOOL, selection_path=SELECTION, component=None):
    selection, entries = validate_selection(selection_path, check_history=component is None)
    if scope == "pilot":
        entries = [e for identity in selection["pilot"] for e in entries if e["id"] == identity]
    elif scope != "candidate":
        raise ValueError("Unknown export scope")
    explicit_component = component
    if component is None:
        state = ev.project_state(ev.read_events(ROOT / "evaluation/events.jsonl"))
        build = state["builds"][state["installed_build"]]
    else:
        build = component_build(component)
    component = Path(build["installed_path"])
    for relative, expected in build["bundle_hashes"].items():
        if ev.digest(component / relative) != expected:
            raise ValueError("Installed AU differs from the recorded build")
    tool = Path(tool).resolve()
    if not tool.is_file():
        raise ValueError("Build nitride_au_presets first")
    output = Path(output).absolute()
    output.mkdir(parents=True, exist_ok=False)
    (output / "sources").mkdir()
    manifest = {"format": "nitride-native-preset-checkpoint", "version": 1, "scope": scope,
                "status": "awaiting-logic-category-confirmation", "selection": selection["id"],
                "selection_sha256": ev.digest(selection_path), "installed_build": build["id"], "plugin_version": build["version"],
                "component_binary_sha256": build["binary_sha256"], "native_tool_sha256": ev.digest(tool),
                "qualification": "Native AU ClassInfo/full-document/89-parameter/48kHz64frame audio recall, not Logic UI discovery or release approval. Only outer host labels cleaned; archived plugin names/IDs/conditions unchanged. No renewed performance/hardware/maximum-polyphony qualification.",
                "entries": [], "files": {}}
    shutil.copy2(selection_path, output / "selection.json")
    for entry in entries:
        relative = entry["category"] + "/" + entry["name"] + ".aupreset"
        path = output / relative; path.parent.mkdir(exist_ok=True)
        original = ROOT / entry["source"]
        source = json.loads(original.read_text())
        source_relative = "sources/" + entry["family"] + ".nitridepreset"
        shutil.copy2(original, output / source_relative)
        first = native(["export", str(entry["program"]), source["name"], entry["name"], str(path)], tool, explicit_component)
        validate_native(path, entry, source)
        verification = native(["verify", str(path), str(entry["program"]), source["name"]], tool, explicit_component)
        metrics = json.loads(verification)
        if not metrics["full_document_exact"] or not metrics["audio_exact"] or metrics["parameters"] != 89:
            raise ValueError("Incomplete native recall verification")
        manifest["entries"].append({**entry, "file": relative, "frozen_source": source_relative,
                                     "export_stdout": first, "native_verification": metrics})
        print("Verified", relative, "(full document, 89 parameters, exact audio)", flush=True)
    for relative, expected in build["bundle_hashes"].items():
        if ev.digest(package_file(component, relative)) != expected:
            raise ValueError("Component changed during preset export; no presets installed")
    for path in sorted(output.rglob("*")):
        if path.is_file():
            ev.durable(path); manifest["files"][str(path.relative_to(output))] = ev.digest(path)
    with (output / "manifest.json").open("x") as stream:
        stream.write(json.dumps(manifest, indent=2) + "\n")
    ev.durable(output / "manifest.json")
    return manifest


def package_file(root, relative):
    path = Path(relative)
    if path.is_absolute() or not path.parts or any(part in (".", "..") for part in path.parts):
        raise ValueError("Unsafe package path")
    result = Path(root) / path
    if any(p.is_symlink() for p in (result, *result.parents)) or not result.is_file():
        raise ValueError("Package files must be regular files, not links")
    if not result.resolve().is_relative_to(Path(root).resolve()):
        raise ValueError("Package path escapes its root")
    return result


def verify_package(folder):
    folder = Path(folder)
    manifest = json.loads(package_file(folder, "manifest.json").read_text())
    if (manifest.get("format"), manifest.get("version"), manifest.get("status")) != (
            "nitride-native-preset-checkpoint", 1, "awaiting-logic-category-confirmation"):
        raise ValueError("Not a native-preset checkpoint package")
    for relative, expected in manifest["files"].items():
        if ev.digest(package_file(folder, relative)) != expected:
            raise ValueError("Package checksum mismatch: " + relative)
    ids, paths = set(), set()
    if not manifest["entries"]:
        raise ValueError("Empty native preset package")
    for entry in manifest["entries"]:
        expected_path = component_name(entry["category"]) + "/" + component_name(entry["name"]) + ".aupreset"
        if entry["file"] != expected_path or entry["file"] in paths or entry["id"] in ids:
            raise ValueError("Invalid/duplicate categorized preset path or identity")
        if entry["file"] not in manifest["files"] or entry["frozen_source"] not in manifest["files"]:
            raise ValueError("Unhashed native preset/source")
        source = package_file(folder, entry["frozen_source"])
        if ev.digest(source) != entry["source_sha256"]:
            raise ValueError("Source definition checksum mismatch")
        validate_native(package_file(folder, entry["file"]), entry, json.loads(source.read_text()))
        metrics = entry["native_verification"]
        if metrics.get("full_document_exact") is not True or metrics.get("audio_exact") is not True or metrics.get("parameters") != 89:
            raise ValueError("Missing recorded native verification")
        ids.add(entry["id"]); paths.add(entry["file"])
    return manifest


def install_preview(folder, destination=DESTINATION):
    manifest = verify_package(folder)
    if manifest["scope"] != "pilot" or len(manifest["entries"]) != 3 or len({e["category"] for e in manifest["entries"]}) != 2:
        raise ValueError("Only the three-preset/two-category pilot can be installed before Logic confirmation")
    destination = Path(destination).absolute()
    if destination.exists() or destination.is_symlink():
        raise ValueError("Preview destination already exists; no user files will be overwritten")
    if any(parent.is_symlink() for parent in destination.parents):
        raise ValueError("Preview destination must not traverse symbolic links")
    receipt = {"format": "nitride-preset-preview-install", "version": 1,
               "package_sha256": ev.digest(Path(folder) / "manifest.json"), "installed_build": manifest["installed_build"],
               "status": "awaiting-logic-category-confirmation", "files": {e["file"]: manifest["files"][e["file"]] for e in manifest["entries"]}}
    destination.mkdir(parents=True, exist_ok=False)
    for relative, expected in receipt["files"].items():
        target = destination / relative; target.parent.mkdir(exist_ok=True)
        with package_file(folder, relative).open("rb") as source, target.open("xb") as output:
            shutil.copyfileobj(source, output); output.flush(); os.fsync(output.fileno())
        if ev.digest(target) != expected:
            raise ValueError("Installed preview checksum mismatch")
    with (destination / RECEIPT).open("x") as stream:
        stream.write(json.dumps(receipt, indent=2) + "\n"); stream.flush(); os.fsync(stream.fileno())
    return receipt


def uninstall_preview(destination=DESTINATION):
    destination = Path(destination).absolute()
    receipt = json.loads(package_file(destination, RECEIPT).read_text())
    if (receipt.get("format"), receipt.get("version")) != ("nitride-preset-preview-install", 1) or not receipt.get("files"):
        raise ValueError("No valid Nitride preview ownership receipt")
    # Preflight EVERY owned file before removing anything. User additions are never touched.
    paths = [package_file(destination, relative) for relative in receipt["files"]]
    for path, expected in zip(paths, receipt["files"].values()):
        if path.suffix != ".aupreset" or ev.digest(path) != expected:
            raise ValueError("Preview was modified; preserve user changes instead of uninstalling")
    for path in paths:
        path.unlink()
    (destination / RECEIPT).unlink()
    for directory in sorted({p.parent for p in paths}, key=lambda p: len(p.parts), reverse=True):
        try: directory.rmdir()
        except OSError: pass  # A user-saved addition remains in place.
    try: destination.rmdir()
    except OSError: pass


def host_confirmation(events=None):
    if events is None:
        approved = acceptance()
        if approved.get("host_category_confirmation") == "confirmed-by-user":
            return approved["host_category_feedback_event"]
        events = ev.read_events(ROOT / "evaluation/events.jsonl")
    for event in reversed(events):
        confirmation = event["payload"].get("host_category_confirmation", {})
        if event["type"] != "note" or confirmation.get("status") != "confirmed-by-user":
            continue
        feedback = next((e for e in events if e["id"] == confirmation.get("feedback_event")), None)
        if feedback and feedback["type"] == "feedback" and feedback["author"] == "user" and feedback["payload"].get("scope") == "preset-finalization-native-host-pilot":
            return feedback["id"]
    raise ValueError("Actual Logic category discovery must be confirmed by the user before publishing the factory catalog")


def install_factory(folder, destination=FACTORY_DESTINATION):
    confirmation = host_confirmation()
    manifest = verify_package(folder)
    if manifest["scope"] != "candidate":
        raise ValueError("Top-level install requires the full candidate package, not the pilot")
    destination = Path(destination).absolute()
    if any(path.is_symlink() for path in (destination, *destination.parents)):
        raise ValueError("Factory destination must not traverse symbolic links")
    files = {e["file"]: manifest["files"][e["file"]] for e in manifest["entries"]}
    receipt = {"format": "nitride-factory-preset-install", "version": 1,
               "status": "organized-candidate-not-release-approved", "selection": manifest["selection"],
               "host_confirmation_feedback": confirmation, "package_sha256": ev.digest(Path(folder) / "manifest.json"),
               "installed_build": manifest["installed_build"], "files": files}
    receipt_path = destination / FACTORY_RECEIPT
    if receipt_path.exists():
        previous = json.loads(package_file(destination, FACTORY_RECEIPT).read_text())
        if previous == receipt and all(ev.digest(package_file(destination, p)) == h for p, h in files.items()):
            return receipt  # Identical reinstall is a no-op, never an overwrite.
        raise ValueError("Managed factory files already exist; preserve edits and use an explicit versioned update, not automatic replacement")
    for relative in files:
        target = destination / relative
        if target.exists() or target.is_symlink():
            raise ValueError("Factory path already exists; no user preset will be overwritten: " + relative)
        if target.parent.is_symlink() or (target.parent.exists() and not target.parent.is_dir()):
            raise ValueError("Unsafe category directory: " + relative)
    destination.mkdir(parents=True, exist_ok=True)
    created, directories = [], []
    try:
        # Stage complete files first. Hard-link publication is atomic and fails if
        # anything appeared at the destination since preflight; no partial preset writes.
        with tempfile.TemporaryDirectory(prefix=".nitride-factory-stage-", dir=destination) as temporary:
            stage = Path(temporary)
            for index, relative in enumerate(files):
                staged = stage / str(index)
                with package_file(folder, relative).open("rb") as source, staged.open("xb") as output:
                    shutil.copyfileobj(source, output); output.flush(); os.fsync(output.fileno())
                if ev.digest(staged) != files[relative]:
                    raise ValueError("Staged factory checksum mismatch")
            staged_receipt = stage / "receipt.json"
            staged_receipt.write_text(json.dumps(receipt, indent=2) + "\n"); ev.durable(staged_receipt)
            for index, (relative, expected) in enumerate(files.items()):
                target = destination / relative
                if not target.parent.exists(): target.parent.mkdir(); directories.append(target.parent)
                os.link(stage / str(index), target); created.append((target, expected))
            os.link(staged_receipt, receipt_path)
    except Exception:
        for path, expected in reversed(created):
            if path.is_file() and not path.is_symlink() and ev.digest(path) == expected: path.unlink()
        for directory in reversed(directories):
            try: directory.rmdir()
            except OSError: pass
        raise
    return receipt


def install_missing(folder, destination=FACTORY_DESTINATION):
    """Source-install path: add missing factory files, never replace or claim existing files."""
    host_confirmation()  # Checked-in acceptance suffices; no personal journal is needed.
    manifest = verify_package(folder)
    selection, entries = validate_selection(check_history=False)
    if (manifest["scope"] != "candidate" or manifest["selection"] != selection["id"]
            or len(manifest["entries"]) != len(entries)
            or any(any(item.get(key) != value for key, value in entry.items())
                   for entry, item in zip(entries, manifest["entries"]))):
        raise ValueError("Source install requires the complete current factory catalog")
    destination = Path(destination).absolute()
    if any(p.is_symlink() for p in (destination, *destination.parents)):
        raise ValueError("Factory destination must not traverse symbolic links")
    files = {e["file"]: manifest["files"][e["file"]] for e in manifest["entries"]}
    missing, preserved = {}, []
    for relative, expected in files.items():
        target = destination / relative
        if target.parent.is_symlink() or (target.parent.exists() and not target.parent.is_dir()):
            raise ValueError("Unsafe category directory: " + relative)
        if target.exists() or target.is_symlink():
            preserved.append(relative)
        else:
            missing[relative] = expected
    if not missing:
        return {"installed": [], "preserved": preserved}
    destination.mkdir(parents=True, exist_ok=True)
    created, directories = [], []
    try:
        with tempfile.TemporaryDirectory(prefix=".nitride-factory-stage-", dir=destination) as temporary:
            stage = Path(temporary)
            for index, (relative, expected) in enumerate(missing.items()):
                staged = stage / str(index)
                shutil.copyfile(package_file(folder, relative), staged)
                ev.durable(staged)
                if ev.digest(staged) != expected:
                    raise ValueError("Staged factory checksum mismatch")
            for index, (relative, expected) in enumerate(missing.items()):
                target = destination / relative
                if not target.parent.exists():
                    target.parent.mkdir()
                    directories.append(target.parent)
                try:
                    os.link(stage / str(index), target)
                except FileExistsError:
                    preserved.append(relative)  # A concurrent save wins; never overwrite it.
                else:
                    created.append((target, expected))
    except BaseException:
        for path, expected in reversed(created):
            if path.is_file() and not path.is_symlink() and ev.digest(path) == expected:
                path.unlink()
        for directory in reversed(directories):
            try: directory.rmdir()
            except OSError: pass
        raise
    # Existing ownership receipts remain untouched. This additive path never removes,
    # migrates or updates sounds, so it does not claim ownership of pre-existing saves.
    return {"installed": [str(path.relative_to(destination)) for path, _ in created], "preserved": preserved}


def deploy(component, tool=TOOL, destination=FACTORY_DESTINATION):
    with tempfile.TemporaryDirectory(prefix="nitride-native-presets-") as temporary:
        package = Path(temporary).resolve() / "package"
        export_package(package, scope="candidate", tool=tool, component=component)
        return install_missing(package, destination)


def extend_factory(folder, archive, destination=FACTORY_DESTINATION, retire_studies=False, study_ids=()):
    """Explicit additive update only: existing managed sounds must remain byte-identical."""
    confirmation = host_confirmation()
    manifest = verify_package(folder)
    if manifest["scope"] != "candidate": raise ValueError("Extension requires the full candidate")
    destination, archive = Path(destination).absolute(), Path(archive).absolute()
    if any(p.is_symlink() for path in (destination, archive) for p in (path, *path.parents)):
        raise ValueError("Extension paths must not traverse symbolic links")
    if archive.is_relative_to(destination) or destination.is_relative_to(archive):
        raise ValueError("Archive must be outside the host preset tree")
    previous_path = package_file(destination, FACTORY_RECEIPT)
    previous_bytes = previous_path.read_bytes(); previous = json.loads(previous_bytes)
    if (previous.get("format"), previous.get("version")) != ("nitride-factory-preset-install", 1) or not previous.get("files"):
        raise ValueError("No valid factory ownership receipt")
    files = {e["file"]: manifest["files"][e["file"]] for e in manifest["entries"]}
    for relative, expected in previous["files"].items():
        if ev.digest(package_file(destination, relative)) != expected or files.get(relative) != expected:
            raise ValueError("Extension cannot remove, edit or replace an existing managed sound: " + relative)
    additions = {p: h for p, h in files.items() if p not in previous["files"]}
    for relative in additions:
        path = destination / relative
        if path.exists() or path.is_symlink(): raise ValueError("User preset collision: " + relative)
        if path.parent.exists() and (not path.parent.is_dir() or path.parent.is_symlink()): raise ValueError("Unsafe category directory")
    retire, requests = {}, []
    legacy_receipt = ".nitride-fs1r-study-001.json"
    if retire_studies and ((destination / legacy_receipt).exists() or (destination / legacy_receipt).is_symlink()):
        requests.append((legacy_receipt, None))
    if len(study_ids) != len(set(study_ids)): raise ValueError("Duplicate study retirement request")
    for identity in study_ids:
        ev.identifier(identity); requests.append((".nitride-study-" + identity + ".json", identity))
    documents = [document_from_native(plistlib.loads(package_file(folder, p).read_bytes())) for p in files] if requests else []
    for study_receipt, identity in requests:
        study = json.loads(package_file(destination, study_receipt).read_text())
        expected_format = "nitride-study-preset-install" if identity else "nitride-fs1r-study-install"
        owned = study.get("files", {})
        if (study.get("format"), study.get("version")) != (expected_format, 1) or not isinstance(owned, dict) or not 1 <= len(owned) <= 12:
            raise ValueError("Invalid study ownership receipt")
        if identity:
            if study.get("study") != identity or study.get("status") != "awaiting-feedback-not-factory-selected":
                raise ValueError("Study identity/status differs from explicit retirement request")
        elif len(owned) != 3: raise ValueError("Legacy study receipt must own exactly three files")
        for relative, expected in study["files"].items():
            path = package_file(destination, relative)
            if relative in files or relative in retire: raise ValueError("Study file has conflicting ownership")
            if path.suffix != ".aupreset" or not path.name.startswith("Study - ") or ev.digest(path) != expected:
                raise ValueError("Study file edited; preserve user changes: " + relative)
            old_document = document_from_native(plistlib.loads(path.read_bytes()))
            if old_document not in documents: raise ValueError("Study sound has no exact clean-label replacement")
            retire[relative] = expected
        retire[study_receipt] = ev.digest(destination / study_receipt)
    receipt = {**previous, "installed_build": manifest["installed_build"], "selection": manifest["selection"],
               "package_sha256": ev.digest(Path(folder) / "manifest.json"), "host_confirmation_feedback": confirmation, "files": files}
    archive.mkdir(parents=True, exist_ok=False)
    # Preserve prior receipt and exact retired files before any host-tree mutation.
    (archive / FACTORY_RECEIPT).write_bytes(previous_bytes); ev.durable(archive / FACTORY_RECEIPT)
    for relative in retire:
        path = archive / relative; path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(package_file(destination, relative), path); ev.durable(path)
    created, directories, moved = [], [], []
    published = False
    try:
        with tempfile.TemporaryDirectory(prefix=".nitride-extend-", dir=destination) as temporary:
            stage = Path(temporary)
            for i, (relative, expected) in enumerate(additions.items()):
                path = stage / str(i); shutil.copy2(package_file(folder, relative), path); ev.durable(path)
                if ev.digest(path) != expected: raise ValueError("Staged extension checksum mismatch")
            new_receipt = stage / "receipt.json"; new_receipt.write_text(json.dumps(receipt, indent=2) + "\n"); ev.durable(new_receipt)
            for i, (relative, expected) in enumerate(additions.items()):
                path = destination / relative
                if not path.parent.exists(): path.parent.mkdir(); directories.append(path.parent)
                os.link(stage / str(i), path); created.append((path, expected))
            for i, (relative, expected) in enumerate(retire.items()):
                path = package_file(destination, relative)
                if ev.digest(path) != expected: raise ValueError("Study changed during extension")
                saved = stage / ("retired-" + str(i)); os.rename(path, saved); moved.append((path, saved))
            if previous_path.read_bytes() != previous_bytes: raise ValueError("Factory receipt changed during extension")
            try:
                os.replace(new_receipt, previous_path)
                published = True
            except BaseException:
                for path, saved in reversed(moved): os.link(saved, path)
                moved.clear()
                raise
    except BaseException:
        if published:
            raise RuntimeError("Catalog extension was published; temporary cleanup failed. Do not roll back managed files.")
        # Temporary moved files may be gone; durable archive is the rollback source.
        for path, _ in reversed(moved):
            if not path.exists():
                with path.open("xb") as stream: stream.write((archive / path.relative_to(destination)).read_bytes())
        for path, expected in reversed(created):
            if path.is_file() and not path.is_symlink() and ev.digest(path) == expected: path.unlink()
        for path in reversed(directories):
            try: path.rmdir()
            except OSError: pass
        raise
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("selection")
    export = commands.add_parser("export")
    export.add_argument("output", type=Path)
    export.add_argument("--scope", choices=("pilot", "candidate"), default="pilot")
    export.add_argument("--tool", type=Path, default=TOOL)
    export.add_argument("--component", type=Path, help="Export directly from this bundle, without the private evaluation journal")
    source_install = commands.add_parser("deploy", help="Export, verify and install only missing factory presets from a source-built AU")
    source_install.add_argument("--component", type=Path, required=True)
    source_install.add_argument("--tool", type=Path, default=TOOL)
    source_install.add_argument("--destination", type=Path, default=FACTORY_DESTINATION)
    verify = commands.add_parser("verify"); verify.add_argument("package", type=Path)
    install = commands.add_parser("install-preview"); install.add_argument("package", type=Path)
    install.add_argument("--destination", type=Path, default=DESTINATION)
    uninstall = commands.add_parser("uninstall-preview")
    uninstall.add_argument("--destination", type=Path, default=DESTINATION)
    factory = commands.add_parser("install-factory"); factory.add_argument("package", type=Path)
    factory.add_argument("--destination", type=Path, default=FACTORY_DESTINATION)
    extend = commands.add_parser("extend-factory"); extend.add_argument("package", type=Path)
    extend.add_argument("--archive", type=Path, required=True)
    extend.add_argument("--destination", type=Path, default=FACTORY_DESTINATION)
    extend.add_argument("--retire-studies", action="store_true")
    extend.add_argument("--retire-study", action="append", default=[], help="Explicit study identity; archive only exact owned duplicates")
    args = parser.parse_args()
    if args.command == "selection":
        _, entries = validate_selection(); print("Proposed:", len(entries), "retained families; pending voices and archived iterations excluded")
    elif args.command == "export": export_package(args.output, args.scope, args.tool, component=args.component)
    elif args.command == "deploy":
        result = deploy(args.component, args.tool, args.destination)
        print("Installed", len(result["installed"]), "missing factory presets at:", args.destination)
        print("Preserved", len(result["preserved"]), "existing paths unchanged (including any user edits).")
        print("Restart Logic and use the instrument's Settings menu to browse the category folders.")
    elif args.command == "verify":
        manifest = verify_package(args.package); print("Verified native preset package:", len(manifest["entries"]), "voices; host confirmation is recorded separately")
    elif args.command == "install-preview":
        install_preview(args.package, args.destination); print("Installed pilot:", args.destination)
    elif args.command == "uninstall-preview":
        uninstall_preview(args.destination); print("Removed only unchanged owned preview files; any user additions preserved")
    elif args.command == "install-factory":
        install_factory(args.package, args.destination); print("Installed categorized factory candidate directly at:", args.destination)
    elif args.command == "extend-factory":
        extend_factory(args.package, args.archive, args.destination, args.retire_studies, args.retire_study)
        print("Extended catalog without changing existing managed sounds; prior receipts/studies archived at:", args.archive)


if __name__ == "__main__":
    try: main()
    except subprocess.CalledProcessError as error:
        raise SystemExit((error.stderr or error.stdout or str(error)).strip()) from error
    except (ValueError, KeyError, OSError, ET.ParseError, json.JSONDecodeError, plistlib.InvalidFileException) as error:
        raise SystemExit(str(error)) from error
