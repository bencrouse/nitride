# Filter qualification — manual external checks

This internal Apple Silicon package does **not** install or replace an AU, app,
preset or receipt. Its explicit-component probes run independently of whichever
Nitride is installed. Gatekeeper blocks are evidence: report them, do not disable
Gatekeeper globally. Do not use an unsigned public distribution workflow.

## Offline run

1. On the actual test Mac, close Logic/other audio apps and stop builds. Use power.
2. Run `Run Offline Checks.command`. No compiler/Python is required.
3. Keep the new results folder. It contains hardware/build identity, raw timings,
   logs, and a **generated diagnostic AU state**, not captured Logic settings.
4. Benchmark completion is not deadline qualification. The 30-minute **audio-duration**
   continuity run is offline and may finish faster/slower than 30 wall-clock minutes.
   Native process recreation is not a Logic project-reopen test.

## Logic — cannot be inferred from offline results

Use your own disposable **local** test project. Never return/upload projects,
settings snapshots, screenshots or audio. Do not close an unsaved existing project
or replace a component without an explicit backup/install decision.

- [ ] Confirm Logic actually loads the package's version and evaluation build ID.
      The offline package does not register/install it for Logic. If versions differ,
      stop: Logic checks remain pending until a separately authorized installation.
- [ ] At 48 kHz / 256 frames, load the generated probe or existing Diode Current r2.
      Compare all five characters at 0/50/100% on the same notes, starting quietly.
- [ ] Save the disposable project locally with nondefault family, character, amount,
      Mono/poly and modulation settings. Close/reopen it, then quit/relaunch Logic
      and reopen. Confirm recall and playback. Do not submit the project/settings.
- [ ] Exercise sustain/release, panic, live character/family changes, automation and
      instance duplication. Check for stuck notes, clicks, faults or unexpected changes.
- [ ] Loop playback for **30–60 actual wall-clock minutes** in Logic. Record CPU
      warnings, audio dropouts, memory trend and any reproduction steps as plain notes.
- [ ] Repeat a conservative 96 kHz / 1–4 voice case if available. Maximum 12-voice
      96 kHz support is not assumed; retain failures instead of reducing quality/voices.

## Return only these plain observations

- Actual CPU model, macOS and Logic version:
- Nitride version / evaluation build displayed in Logic:
- Sample rate, buffer size, voice/instance count:
- Project reopen/relaunch: PASS / FAIL / NOT RUN
- Logic soak wall-clock minutes, warnings/dropouts: PASS / FAIL / NOT RUN
- Parameter/automation/character recall problems:
- Plain-text failure steps (no project/settings/audio attachments):

Actual M2 execution and manual checks stay **pending** until evidence is returned.
The tool never uploads anything, captures a project, changes audio devices, or
silently reduces processing quality/polyphony. No factory promotion is implied.
