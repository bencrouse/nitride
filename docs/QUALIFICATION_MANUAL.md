# Qualification001 — manual Logic and standalone checks

This is an internal Apple Silicon development candidate, not a signed/notarized public
release. Please record the exact Mac model/RAM, macOS/build, Logic version, audio output,
sample rate and buffer you actually use. The runner collects basic chip/OS/app-version
metadata, not projects, serial numbers, audio-device settings or a conversation history.
Nothing is uploaded. Share only results you choose to send.

## Install and automatic checks

1. Unzip the complete test folder on the M2. Keep it writable (e.g. Desktop).
2. This installer is **fresh-install only**. If Nitride already exists, it refuses to
   overwrite it; stop and ask for an explicit versioned update. Do not manually delete
   your existing sounds or rename components to work around the refusal.
3. Run `Install.command`. It installs the AU in your user Components folder, the standalone
   in `~/Applications/Nitride Qualification.app`, and53categorized factory presets.
4. These artifacts are ad-hoc signed for internal testing, **not notarized**. If macOS
   blocks them, report the exact message. Do not disable Gatekeeper or re-sign binaries;
   public-release signing/notarization is a separate step and the hashes must stay intact.
5. Quit Logic and other audio apps, stop builds, plug the Mac into power and run
   `Run Tests.command`. It needs no compiler or Python. Allow15–30minutes (hardware dependent).
   It produces no live audio and does not change device/system settings.
6. Do not label benchmark completion as a pass: overdue blocks and p99 headroom require
   review. Socket/native JSON, logs and per-preset status files identify actual results.

## Project-save/reopen check — about15minutes

Do this in a **new disposable project**, not existing music. Use moderate monitoring volume.
Choose48kHz/128frames first; if that is not your actual configuration, write down what you
used. We are not automatically changing Logic's defaults or the system output.

1. Add five Nitride tracks: Plastic Hero, Budget Heaven, Socket Dance, Reverse Veil and Tine Keys.
2. Set the project tempo to120BPM. The supplied `inputs/` folder includes original test MIDI.
   For Socket Dance use `socket-120.mid`. Keep the project tempo when importing other MIDI.
3. On the Tine Keys instance, set Tone cutoff to1750Hz, Output to−9dB and Release to450ms.
   Save a new host setting called `Qualification - My Tine Keys`; **do not overwrite factory files**.
4. Add/read a short cutoff automation lane on another track. Stop at a known bar with
   automation in Read mode; write down the expected sound/settings at that same playhead position.
5. Save the project. Close it, **quit Logic completely**, relaunch Logic and reopen the project.
6. Confirm all five sounds return, the edited values and user setting are retained,
   and automation works when played. Load a fresh factory Tine Keys instance and confirm
   it is unchanged. Confirm the53clean preset names and absence of an extra AU Presets submenu.
7. Record PASS/FAIL/NOT RUN and any exact steps needed to reproduce a problem. A preset
   reload alone is not the project-close/process-restart test.

Free-running modulation means uncontrolled bounces can differ. Check the restored state,
musical behavior and controlled transport; do not demand identical waveforms from an
uncontrolled free-running LFO. The automated tests handle controlled sample comparisons.

## Real Logic soak —30–60minutes

Run the disposable project with repeated MIDI and moderate playback level. During the run:

- Play live notes, short overlaps and sustain. Release all notes/pedal and check nothing sticks.
- Loop playback, change tempo, stop/seek/resume, switch presets, and repeatedly close/reopen editors.
- Use ordinary control/automation edits, not only a static held chord.
- Check real-time playback and an offline bounce; return to playback afterward.
- Note crashes, Logic overloads, audible dropouts, hanging notes and sustained memory growth.
  Distinguish an observed device/host dropout from an offline benchmark deadline overrun.

Record the actual duration, instance count, settings, activities and observations. Do not
mark a short run or offline benchmark as the completed soak.96kHzmaximum-polyphony is a
known risk, not guaranteed supported; do not infer a pass there from48kHz results.

## Standalone smoke check

Open `~/Applications/Nitride Qualification.app`. Choose an app-local output at a sensible
volume, play/release notes, browse presets and close/reopen the app. Note startup, MIDI,
device or sound problems. No system-wide audio-device change is required.

## Results to return

Copy this checklist into the generated `results-*` folder and append:

- Mac model/RAM, exact OS/Logic versions:
- Actual audio output/rate/buffer:
- Project-close/reopen: PASS / FAIL / NOT RUN; observations:
- Soak: actual minutes/instances/activity; PASS / FAIL / NOT RUN; observations:
- Standalone: PASS / FAIL / NOT RUN; observations:
- Any blocks/security prompts or reproduction steps:

Return that results folder for review. No project or recording is required. An explicit
failure or missing check is useful evidence; it must not be silently treated as passed.

## Remove the internal candidate

`Uninstall.command` first verifies every owned file. It removes only unchanged files from
this candidate, leaves user-created settings and app preferences alone, and refuses if an
owned file was edited. It is not a forced cleanup or a general updater.
