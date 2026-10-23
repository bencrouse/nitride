#!/usr/bin/env python3
"""Local evaluation journal. No plugin telemetry, Git writes, or cloud access."""
import argparse
import contextlib
import datetime as dt
import fcntl
import hashlib
import json
import os
from pathlib import Path
import platform
import plistlib
import re
import shutil
import subprocess
import tempfile
import uuid
import zipfile

PROJECT = Path(__file__).resolve().parents[1]
CATEGORIES = ["Pads", "Basses", "Leads", "Sequence elements", "Physical materials", "Experimental"]
STATUSES = ["awaiting-trial", "in-trial", "needs-revision", "direction-kept", "parked", "approved"]


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def identifier(value):
    if not re.fullmatch(r"[a-zA-Z0-9_-]{1,80}", value):
        raise ValueError("Identity must use letters, numbers, underscore or hyphen")
    return value


def atomic_write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix="." + path.name, dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        directory = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def write_json(path, value):
    atomic_write(path, (json.dumps(value, indent=2, ensure_ascii=False) + "\n").encode())


def durable(path):
    with Path(path).open("rb") as stream:
        os.fsync(stream.fileno())


def read_events(path):
    path = Path(path)
    if not path.exists():
        return []
    data = path.read_bytes()
    if data and not data.endswith(b"\n"):
        raise ValueError("Incomplete journal tail; use recover-tail explicitly before writing")
    events, previous = [], "0" * 64
    for line in data.splitlines():
        event = json.loads(line)
        checksum = event.pop("hash")
        if event.get("version") != 1 or event.get("sequence") != len(events) + 1 or event.get("previous") != previous:
            raise ValueError("Invalid journal sequence/version/chain")
        if hashlib.sha256(canonical(event)).hexdigest() != checksum:
            raise ValueError("Journal checksum mismatch; preserve and inspect the original")
        event["hash"] = checksum
        previous = checksum
        events.append(event)
    return events


def project_state(events):
    state = {"version": 1, "categories": CATEGORIES, "revisions": {}, "builds": {}, "renders": {}, "trials": {}, "feedback": [], "guidance": [], "next": "Initialize review candidates"}
    superseded = {e["payload"]["supersedes"] for e in events if e["payload"].get("supersedes")}
    for event in events:
        if event["id"] in superseded:
            continue
        p, kind = event["payload"], event["type"]
        if kind == "revision":
            state["revisions"][p["id"]] = {**p, "status": "awaiting-trial"}
        elif kind == "build":
            state["builds"][p["id"]] = p
            state["installed_build"] = p["id"]
        elif kind == "trial":
            state["trials"][p["id"]] = p
            state["active_trial"] = p["id"]
        elif kind == "render":
            state["renders"][p["id"]] = p
        elif kind == "decision":
            state["revisions"][p["revision"]]["status"] = p["status"]
        elif kind == "feedback":
            state["feedback"].append({**p, "event": event["id"], "timestamp": event["timestamp"], "author": event["author"]})
        elif kind == "guidance":
            state["guidance"].append({**p, "event": event["id"], "author": event["author"]})
        if p.get("next"):
            state["next"] = p["next"]
    return state


def validate_builtins(project=PROJECT, *, check_history=True):
    """Always validate checked-in identities/hashes; optionally cross-check private history."""
    root = Path(project) / "presets/review"
    catalog = json.loads((root / "builtins.json").read_text())
    if catalog.get("format") != "nitride-review-builtins" or catalog.get("version") != 1 or not catalog.get("entries"):
        raise ValueError("Invalid builtin catalog")
    ids, filenames = set(), set()
    for i, entry in enumerate(catalog["entries"]):
        identifier(entry["family"])
        identifier(entry["revision"])
        if entry["program"] != i + 4 or entry["id"] != f'review:{entry["family"]}:{entry["revision"]}' or entry["id"] in ids:
            raise ValueError("Review program IDs must be unique and appended after the four originals")
        path = (root / entry["preset"]).resolve()
        if not path.is_relative_to(root.resolve()) or path.name in filenames or digest(path) != entry["sha256"]:
            raise ValueError("Missing, duplicate-resource or changed builtin preset: " + entry["id"])
        data = json.loads(path.read_text())
        if data.get("format") != "nitride-preset" or data.get("version") != 6:
            raise ValueError("Unsupported embedded preset schema")
        ids.add(entry["id"])
        filenames.add(path.name)
    events = read_events(Path(project) / "evaluation/events.jsonl") if check_history else []
    for event in events:
        if event["type"] == "revision":
            old = event["payload"]
            matching = [e for e in catalog["entries"] if e["id"] == old["id"]]
            if matching and (matching[0]["sha256"] != old["preset_sha256"] or matching[0]["program"] != old["program"]):
                raise ValueError("Registered builtin revisions cannot change or move")
        if event["type"] == "build":
            old = event["payload"]["registry"]
            if catalog["entries"][:len(old)] != old:
                raise ValueError("Built-in review catalog is append-only; old programs/revisions must remain")
    return catalog


class Ledger:
    def __init__(self, root, project=PROJECT):
        self.root, self.project = Path(root), Path(project)
        self.root.mkdir(parents=True, exist_ok=True)

    @contextlib.contextmanager
    def locked(self):
        with (self.root / ".lock").open("a") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            yield

    def events(self):
        return read_events(self.root / "events.jsonl")

    def append(self, kind, payload, author="assistant"):
        events = self.events()
        if payload.get("supersedes") and not any(e["id"] == payload["supersedes"] and e["type"] == kind for e in events):
            raise ValueError("Superseded event must exist and have the same type")
        event = {"version": 1, "sequence": len(events) + 1, "id": uuid.uuid4().hex, "timestamp": dt.datetime.now(dt.timezone.utc).isoformat(),
                 "author": author, "type": kind, "payload": payload, "previous": events[-1]["hash"] if events else "0" * 64}
        event["hash"] = hashlib.sha256(canonical(event)).hexdigest()
        with (self.root / "events.jsonl").open("ab") as stream:
            stream.write(canonical(event) + b"\n")
            stream.flush()
            os.fsync(stream.fileno())
        directory = os.open(self.root, os.O_RDONLY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
        self.rebuild()
        return event

    def rebuild(self):
        events = self.events()
        state = project_state(events)
        state["journal_tip"] = events[-1]["hash"] if events else None
        write_json(self.root / "catalog.json", state)
        lines = ["# Current Nitride evaluation", "", "Generated from events.jsonl; add notes through the journal, not this file.", "", "## Resume", "",
                 "- Installed evaluation build: " + state.get("installed_build", "not recorded"), "- Active trial: " + state.get("active_trial", "none"), "- Next: " + state["next"], "",
                 "## Candidates", "", "| Revision | Category | Status | Host program |", "| --- | --- | --- | ---: |"]
        for revision in state["revisions"].values():
            lines.append(f'| {revision["id"]} | {revision["category"]} | {revision["status"]} | {revision["program"]} (zero-based) |')
        if state["guidance"]:
            lines += ["", "## Collection guidance", ""]
            for item in state["guidance"]:
                lines.append(f'- {item["author"]}: {item["text"]}')
                if item.get("interpretation"):
                    lines.append("  - Assistant interpretation: " + item["interpretation"])
        trial = state["trials"].get(state.get("active_trial"))
        if trial:
            lines += ["", "## Trial context", "", "```json", json.dumps(trial, indent=2), "```"]
        lines += ["", "## Latest feedback", ""]
        for feedback in state["feedback"][-8:]:
            lines += [f'- {feedback["timestamp"]} / {feedback.get("revision", "batch")} / {feedback["author"]}: {feedback["text"]}']
            if feedback.get("interpretation"):
                lines.append("  - Assistant interpretation: " + feedback["interpretation"])
        unsketched = [c for c in CATEGORIES if not any(r["category"] == c for r in state["revisions"].values())]
        lines += ["", "## Unsketched categories", "", ", ".join(unsketched) if unsketched else "None — all six categories have sketches; trial feedback and final library approval remain separate.", "",
                  "## Limits", "", "- Logic listening feedback is not inferred from offline tests.", "- 96 kHz maximum-polyphony F1 qualification remains open.",
                  "- No commits are requested. Persistence is not off-device backup.", ""]
        atomic_write(self.root / "CURRENT.md", "\n".join(lines).encode())
        return state

    def register(self, entry, renders):
        state = project_state(self.events())
        if entry["id"] in state["revisions"]:
            if state["revisions"][entry["id"]]["preset_sha256"] != entry["sha256"]:
                raise ValueError("Revision identity already has different sound data")
            old = state["revisions"][entry["id"]]
            source = self.project / "presets/review" / entry["preset"]
            frozen_midi = f'revisions/{entry["family"]}/{entry["revision"]}/performance.mid'
            if digest(source.with_suffix(".mid")) != old["files"][frozen_midi]:
                raise ValueError("Registered MIDI performance changed; create a new revision")
            return
        source = self.project / "presets/review" / entry["preset"]
        stem = source.stem
        if digest(source) != entry["sha256"]:
            raise ValueError("Preset differs from registered builtin source")
        files = {"preset.nitridepreset": source, "performance.mid": source.with_suffix(".mid")}
        artifacts = {"preview.wav": Path(renders) / (stem + ".wav"), "raw.wav": Path(renders) / (stem + "-raw.wav"), "report.json": Path(renders) / "report.json"}
        for path in [*files.values(), *artifacts.values()]:
            if not path.is_file():
                raise ValueError("Missing evidence: " + str(path))
        report = json.loads(artifacts["report.json"].read_text())
        name = json.loads(source.read_text())["name"]
        metrics = next((r for r in report if r["name"] == name), None)
        if metrics is None:
            raise ValueError("Report does not contain the candidate revision")
        relative = Path("revisions") / entry["family"] / entry["revision"]
        evidence = Path("artifacts") / entry["family"] / entry["revision"]
        if (self.root / relative).exists() or (self.root / evidence).exists():
            raise ValueError("Unregistered revision directory exists; inspect it, never overwrite it")
        ancestors = [r for r in state["revisions"].values() if r["family"] == entry["family"]]
        parent = entry.get("parent", ancestors[-1]["id"] if ancestors else None)
        if parent and (parent not in state["revisions"] or state["revisions"][parent]["family"] != entry["family"]):
            raise ValueError("Revision parent must be a registered member of the same family")
        contrast = entry.get("contrast_with")
        if contrast and (contrast not in state["revisions"] or state["revisions"][contrast]["category"] != entry["category"] or state["revisions"][contrast]["family"] == entry["family"]):
            raise ValueError("Contrast must reference a registered different family in the same category")
        changes = {}
        if parent:
            old = state["revisions"][parent]
            baseline = json.loads((self.root / "revisions" / old["family"] / old["revision"] / "preset.nitridepreset").read_text())
            candidate = json.loads(source.read_text())
            for key in sorted(set(baseline["values"]) | set(candidate["values"])):
                before, after = baseline["values"].get(key), candidate["values"].get(key)
                if before != after:
                    changes[key] = {"before": before, "after": after}
            for key in ("mono", "chord"):
                if baseline[key] != candidate[key]:
                    changes[key] = {"before": baseline[key], "after": candidate[key]}
        kind = entry.get("revision_kind", "voicing")
        if kind == "rename-only":
            if not parent or changes:
                raise ValueError("Name-only revisions must preserve all parent sound conditions")
            parent_midi = self.root / "revisions" / old["family"] / old["revision"] / "performance.mid"
            if digest(files["performance.mid"]) != digest(parent_midi):
                raise ValueError("Name-only revisions must preserve the frozen parent MIDI")
        hashes = {}
        for directory, items in ((relative, files), (evidence, artifacts)):
            (self.root / directory).mkdir(parents=True)
            for name, path in items.items():
                target = self.root / directory / name
                shutil.copy2(path, target)
                durable(target)
                hashes[str(directory / name)] = digest(target)
        payload = {"id": entry["id"], "family": entry["family"], "revision": entry["revision"], "category": entry["category"], "program": entry["program"],
                   "source": str(source.relative_to(self.project)), "preset_sha256": entry["sha256"], "files": hashes, "parent": parent, "contrast_with": contrast, "parameter_changes": changes,
                   "revision_kind": kind, "display_family": entry.get("display_family", entry["family"]),
                   "render_provenance": {"evaluation_build": metrics.get("evaluationBuild"), "synth_version": metrics.get("synthVersion"),
                                         "note": "Renderer-reported identity; installed build association is recorded separately"},
                    "next": "Install the identifiable evaluation AU, then compare candidate revisions in Logic"}
        if entry.get("design_lens"):
            payload["design_lens"] = entry["design_lens"]
        if entry.get("study_axis"):
            payload["study_axis"] = entry["study_axis"]
        self.append("revision", payload)

    def verify(self):
        for event in self.events():
            if "files" in event["payload"]:
                for relative, expected in event["payload"]["files"].items():
                    if digest(self.root / relative) != expected:
                        raise ValueError("Evidence checksum mismatch: " + relative)
        return self.rebuild()

    def record_render(self, identity, revision, build, folder):
        identifier(identity)
        state = project_state(self.events())
        if identity in state["renders"] or revision not in state["revisions"] or build not in state["builds"]:
            raise ValueError("Render requires a new ID and registered revision/build")
        item = state["revisions"][revision]
        stem = Path(item["source"]).stem
        source = Path(folder)
        report = json.loads((source / "report.json").read_text())
        filename = self.project / item["source"]
        name = json.loads(filename.read_text())["name"]
        metrics = next((r for r in report if r["name"] == name), None)
        if digest(filename) != item["preset_sha256"]:
            raise ValueError("Registered preset source changed")
        # Export formatting/line endings may differ; sound data and identity may not.
        if metrics is None or json.loads((source / (stem + ".nitridepreset")).read_text()) != json.loads(filename.read_text()):
            raise ValueError("Render does not match the registered preset")
        if metrics.get("evaluationBuild") != build or metrics.get("synthVersion") != state["builds"][build]["version"]:
            raise ValueError("Render build/version differs from the recorded AU")
        expected_midi = item["files"][f'revisions/{item["family"]}/{item["revision"]}/performance.mid']
        if digest(source / (stem + ".mid")) != expected_midi:
            raise ValueError("Render MIDI differs from the frozen performance")
        relative = Path("artifacts") / item["family"] / item["revision"] / identity
        destination = self.root / relative
        if destination.exists():
            raise ValueError("Render evidence directory already exists")
        destination.mkdir(parents=True)
        files = {}
        for basename, path in {"preview.wav": source / (stem + ".wav"), "raw.wav": source / (stem + "-raw.wav"), "report.json": source / "report.json"}.items():
            shutil.copy2(path, destination / basename)
            durable(destination / basename)
            files[str(relative / basename)] = digest(destination / basename)
        self.append("render", {"id": identity, "revision": revision, "build": build, "metrics": metrics, "files": files}, "tool")

    def record_build(self, identity, component, build_dir):
        identifier(identity)
        if identity in project_state(self.events())["builds"]:
            raise ValueError("Build identity already recorded; use a new tag, never replace its binary")
        component, build_dir = Path(component), Path(build_dir)
        cache = (build_dir / "CMakeCache.txt").read_text()
        if f"NITRIDE_EVALUATION_BUILD:STRING={identity}\n" not in cache:
            raise ValueError("Build tag differs from configured evaluation identity")
        binary = component / "Contents/MacOS/nitride"
        info = plistlib.loads((component / "Contents/Info.plist").read_bytes())
        inputs = [p for directory in ("Source", "cmake", "presets", "scripts") for p in (self.project / directory).rglob("*")
                  if p.is_file() and not any(part.startswith(".") or part == "__pycache__" for part in p.relative_to(self.project).parts)]
        inputs += [self.project / name for name in ("CMakeLists.txt", "CMakePresets.json", "Makefile")]
        compile_inputs = [p for p in inputs if (
            str(p.relative_to(self.project)).startswith(("Source/DSP/", "Source/Instrument/", "Source/Plugin", "cmake/"))
            or p.suffix == ".nitridepreset" or p.name in ("CMakeLists.txt", "builtins.json"))]
        if any(p.stat().st_mtime > binary.stat().st_mtime for p in compile_inputs):
            raise ValueError("Source inputs changed after the AU was linked; rebuild before recording it")
        relative = Path("builds") / identity
        destination = self.root / relative
        if destination.exists():
            raise ValueError("Build evidence directory already exists")
        destination.mkdir(parents=True)
        source_zip, au_zip = destination / "source.zip", destination / "installed-au.zip"
        with zipfile.ZipFile(source_zip, "w", zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(inputs):
                archive.write(path, str(path.relative_to(self.project)))
        bundle_hashes = {}
        with zipfile.ZipFile(au_zip, "w", zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(component.rglob("*")):
                if path.is_file():
                    relative_file = str(path.relative_to(component))
                    archive.write(path, component.name + "/" + relative_file)
                    bundle_hashes[relative_file] = digest(path)
        durable(source_zip)
        durable(au_zip)
        git = lambda *args: subprocess.check_output(["git", *args], cwd=self.project, text=True).strip()
        payload = {"id": identity, "version": info["CFBundleShortVersionString"], "bundle": info["CFBundleIdentifier"], "installed_path": str(component),
                   "binary_sha256": digest(binary), "bundle_hashes": bundle_hashes, "git_head": git("rev-parse", "HEAD"), "working_tree": git("status", "--short", "-uall"),
                   "source_files": {str(p.relative_to(self.project)): digest(p) for p in inputs}, "machine": platform.platform(),
                   "registry": validate_builtins(self.project)["entries"], "files": {str(relative / p.name): digest(p) for p in (source_zip, au_zip)},
                   "next": "Compare the latest candidate revisions against their parents in Logic; capture individual feedback"}
        write_json(destination / "manifest.json", payload)
        payload["files"][str(relative / "manifest.json")] = digest(destination / "manifest.json")
        self.append("build", payload, "tool")

    def backup(self, output):
        output = Path(output).resolve()
        if output.is_relative_to(self.root.resolve()) or output.exists():
            raise ValueError("Backup must be a new file outside evaluation/")
        self.verify()
        output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output, "x", zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(self.root.rglob("*")):
                if path.is_file() and path.name != ".lock":
                    archive.write(path, "evaluation/" + str(path.relative_to(self.root)))
            for relative in ("scripts/evaluation.py", "scripts/test_evaluation.py", "docs/EVALUATION.md"):
                path = self.project / relative
                if path.exists():
                    archive.write(path, "tools/" + relative)
        durable(output)
        return digest(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=PROJECT / "evaluation")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("verify-builtins")
    init = sub.add_parser("init")
    init.add_argument("--renders", type=Path, default=PROJECT / "build/preset-review/batch-01/r1")
    for command in ("status", "verify", "rebuild", "recover-tail"):
        sub.add_parser(command)
    build = sub.add_parser("record-build")
    build.add_argument("--id", required=True)
    build.add_argument("--component", type=Path, default=Path.home() / "Library/Audio/Plug-Ins/Components/nitride.component")
    build.add_argument("--build-dir", type=Path, default=PROJECT / "build/local-release")
    rendered = sub.add_parser("record-render")
    rendered.add_argument("--id", required=True)
    rendered.add_argument("--revision", required=True)
    rendered.add_argument("--build", required=True)
    rendered.add_argument("--folder", type=Path, required=True)
    trial = sub.add_parser("trial")
    trial.add_argument("--id", required=True)
    trial.add_argument("--build", required=True)
    trial.add_argument("--host", default="Logic Pro")
    trial.add_argument("--revision", action="append", required=True)
    trial.add_argument("--context", default="{}", help="JSON; unknown values remain absent")
    trial.add_argument("--planned", action="store_true")
    feedback = sub.add_parser("feedback")
    feedback.add_argument("--trial")
    feedback.add_argument("--revision")
    feedback.add_argument("--text", required=True)
    feedback.add_argument("--interpretation")
    feedback.add_argument("--supersedes")
    decision = sub.add_parser("decision")
    decision.add_argument("--revision", required=True)
    decision.add_argument("--status", choices=STATUSES, required=True)
    decision.add_argument("--reason", required=True)
    decision.add_argument("--supersedes")
    note = sub.add_parser("note")
    note.add_argument("--text", required=True)
    for command in (feedback, decision, note):
        command.add_argument("--author", choices=("user", "assistant", "tool"), default="user")
        command.add_argument("--next")
    backup = sub.add_parser("backup")
    backup.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "verify-builtins":
        print("Validated", len(validate_builtins()["entries"]), "immutable embedded review revisions")
        return
    ledger = Ledger(args.root)
    with ledger.locked():
        if args.command == "recover-tail":
            path = ledger.root / "events.jsonl"
            data = path.read_bytes()
            if data.endswith(b"\n"):
                ledger.verify()
                return
            complete = data[:data.rfind(b"\n") + 1]
            diagnostic = ledger.root / "diagnostics" / (uuid.uuid4().hex + "-original-journal.bin")
            atomic_write(diagnostic, data)
            atomic_write(path, complete)
            ledger.append("note", {"text": "Explicit recovery preserved original journal at " + str(diagnostic.relative_to(ledger.root))}, "tool")
        elif args.command == "init":
            for entry in validate_builtins()["entries"]:
                ledger.register(entry, args.renders)
        elif args.command == "record-build":
            ledger.record_build(args.id, args.component, args.build_dir)
        elif args.command == "record-render":
            ledger.record_render(args.id, args.revision, args.build, args.folder)
        elif args.command == "backup":
            print("Backup SHA256:", ledger.backup(args.output))
        elif args.command in ("verify", "status", "rebuild"):
            ledger.verify() if args.command == "verify" else ledger.rebuild()
            print((ledger.root / "CURRENT.md").read_text())
        else:
            state = project_state(ledger.events())
            if args.command == "trial":
                identifier(args.id)
                if args.id in state["trials"] or args.build not in state["builds"] or not all(r in state["revisions"] for r in args.revision):
                    raise ValueError("Trial requires a new ID and recorded build/revisions")
                payload = {"id": args.id, "build": args.build, "revisions": args.revision, "host": args.host, "context": json.loads(args.context),
                           "status": "ready-for-user" if args.planned else "started", "next": "Collect individual Logic feedback for the registered trial revisions"}
                if not isinstance(payload["context"], dict):
                    raise ValueError("Trial context must be a JSON object")
                destination = ledger.root / "trials" / args.id
                if destination.exists():
                    raise ValueError("Trial directory already exists")
                write_json(destination / "context.json", payload)
                ledger.append("trial", payload)
            else:
                payload = {key: value for key, value in vars(args).items() if key not in ("root", "command", "author") and value is not None}
                if payload.get("revision") and payload["revision"] not in state["revisions"]:
                    raise ValueError("Unknown revision")
                if payload.get("trial") and payload["trial"] not in state["trials"]:
                    raise ValueError("Unknown trial")
                if payload.get("trial") and payload.get("revision") and payload["revision"] not in state["trials"][payload["trial"]]["revisions"]:
                    raise ValueError("Feedback revision is not part of the selected trial")
                if args.command == "decision" and args.status == "approved" and args.author != "user":
                    raise ValueError("Library approval must be explicitly authored by the user")
                ledger.append(args.command, payload, args.author)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, json.JSONDecodeError) as error:
        raise SystemExit(str(error)) from error
