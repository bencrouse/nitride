#!/bin/zsh
# Offline/read-only handoff. Never installs presets/components or reads projects.
set -euo pipefail
umask 077
ROOT=${1:?package root}; ROOT=${ROOT:A}
cd "$ROOT"
/usr/bin/shasum -a 256 -c checksums.sha256
print 'Close Logic/other audio apps and stop builds before timing. Use mains power.'
print 'Nothing will be installed or uploaded. Allow roughly 15–30 minutes.'
read -r 'answer?Press Return to start, or Control-C to cancel: '
OUT=$(/usr/bin/mktemp -d "$ROOT/results-XXXXXX")
/bin/cp "$ROOT/candidate.json" "$OUT/candidate.json"
/bin/cp "$ROOT/MANUAL-CHECKLIST.md" "$OUT/MANUAL-CHECKLIST.md"
cpu=$(/usr/sbin/sysctl -n machdep.cpu.brand_string | /usr/bin/tr -cd '[:alnum:] ._-')
os=$(/usr/bin/sw_vers -productVersion)
build=$(/usr/bin/sw_vers -buildVersion)
arch=$(/usr/bin/uname -m)
printf '{"cpu":"%s","macos":"%s","macos_build":"%s","architecture":"%s","logic_reopen":"manual-pending","logic_soak":"manual-pending"}\n' "$cpu" "$os" "$build" "$arch" > "$OUT/machine.json"
"$ROOT/tools/nitride_perf" --identity > "$OUT/perf-identity.json"
"$ROOT/tools/nitride_au_presets" --identity > "$OUT/native-identity.json"
"$ROOT/tools/nitride_rage_spectral_tests" --identity > "$OUT/spectral-identity.json"
gui=(/bin/launchctl asuser "$(/usr/bin/id -u)")
component="$ROOT/payload/nitride.component"
if "${gui[@]}" "$ROOT/tools/nitride_au_presets" --component "$component" export 111 'Review / Basses / Diode Current r2' 'Qualification - generated probe' "$OUT/generated-probe.aupreset" > "$OUT/native-export.log" 2>&1; then
  print PASS > "$OUT/native-export.status"
  if "${gui[@]}" "$ROOT/tools/nitride_au_presets" --component "$component" verify "$OUT/generated-probe.aupreset" 111 'Review / Basses / Diode Current r2' > "$OUT/native-reopen.json" 2> "$OUT/native-reopen.log"; then
    print PASS > "$OUT/native-reopen.status"
  else
    print FAIL > "$OUT/native-reopen.status"
  fi
else
  print FAIL > "$OUT/native-export.status"
fi
if "$ROOT/tools/nitride_rage_spectral_tests" > "$OUT/spectral.json" 2> "$OUT/spectral.log"; then
  print MEASURED > "$OUT/spectral.status"
else
  print FAIL > "$OUT/spectral.status"
fi
if /usr/bin/caffeinate -i "$ROOT/tools/nitride_perf" --rage --passes 3 --seconds 2 --warmup .5 --label external-filter-qualification --output "$OUT/performance.json" > "$OUT/performance.log" 2>&1; then
  print MEASURED > "$OUT/performance.status"
else
  print FAIL > "$OUT/performance.status"
fi
if /usr/bin/caffeinate -i "$ROOT/tools/nitride_perf" --rage --case rage-diode4 --soak --passes 1 --seconds 1800 --warmup .5 --rate 48000 --block 256 --label external-offline-continuity --output "$OUT/continuity.json" > "$OUT/continuity.log" 2>&1; then
  print PASS-OFFLINE-ONLY > "$OUT/continuity.status"
else
  print FAIL > "$OUT/continuity.status"
fi
print -r -- "Results: $OUT"
print 'MEASURED does not mean deadlines passed. Offline continuity is NOT a Logic/audio-device soak.'
print 'Complete the manual checklist locally. Return metrics/checklist only, never projects/settings/audio.'
print 'No installation, persistent power setting change, telemetry or upload occurred.'
