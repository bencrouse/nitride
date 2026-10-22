#!/bin/zsh
# No Python/compiler required on the test Mac. Explicit offline measurements only.
set -euo pipefail
umask 077
ROOT=${1:?qualification package root}; ROOT=${ROOT:A}
print 'Close Logic/other audio apps and stop builds before measuring. Plug the Mac into power.'
print 'This collects offline results, not audio output or projects. Allow roughly15–30minutes.'
read -r "answer?Press Return to begin, or Control-C to cancel: "
/bin/zsh "$ROOT/tools/install.zsh" verify "$ROOT"
OUT=$(/usr/bin/mktemp -d "$ROOT/results-XXXXXX")
/bin/cp "$ROOT/matrix.tsv" "$OUT/matrix.tsv"
/bin/cp "$ROOT/candidate.json" "$OUT/candidate.json"
cpu=$(/usr/sbin/sysctl -n machdep.cpu.brand_string | /usr/bin/tr -cd '[:alnum:] ._-')
version=$(/usr/bin/sw_vers -productVersion)
build=$(/usr/bin/sw_vers -buildVersion)
arch=$(/usr/bin/uname -m)
memory=$(/usr/sbin/sysctl -n hw.memsize)
logic=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' '/Applications/Logic Pro.app/Contents/Info.plist' 2>/dev/null || print 'not-found')
printf '{"cpu":"%s","macos":"%s","macos_build":"%s","architecture":"%s","ram_bytes":%s,"logic_version":"%s","project_reopen":"manual-pending","logic_soak":"manual-pending"}\n' "$cpu" "$version" "$build" "$arch" "$memory" "$logic" > "$OUT/machine.json"
gui=(/bin/launchctl asuser "$(/usr/bin/id -u)")
if "${gui[@]}" /usr/bin/auval -v aumu NT01 NTRD > "$OUT/auval.log" 2>&1; then print PASS > "$OUT/auval.status"; else print FAIL > "$OUT/auval.status"; fi
if "${gui[@]}" "$ROOT/tools/nitride_au_component_tests" > "$OUT/native.log" 2>&1; then print PASS > "$OUT/native.status"; else print FAIL > "$OUT/native.status"; fi
while IFS=$'\t' read -r relative program expected; do
  if "${gui[@]}" "$ROOT/tools/nitride_au_presets" verify "$HOME/Library/Audio/Presets/nitride/nitride/$relative" "$program" "$expected" > "$OUT/native-preset-$program.json" 2> "$OUT/native-preset-$program.log"; then
    print PASS > "$OUT/native-preset-$program.status"
  else
    print FAIL > "$OUT/native-preset-$program.status"
  fi
done < "$ROOT/native.tsv"
if "$ROOT/tools/nitride_socket_tests" "$ROOT/inputs/socket-120.mid" "$ROOT/inputs/socket-118.mid" "$OUT/socket.json" > "$OUT/socket.log" 2>&1; then print PASS > "$OUT/socket.status"; else print FAIL > "$OUT/socket.status"; fi
while IFS=$'\t' read -r group rate block name; do
  [[ -n "$group" && "$group" != \#* ]] || continue
  stem="$group-$rate-$block-$name"
  args=(--passes 3 --seconds 10 --warmup 2 --rate "$rate" --block "$block" --case "$name" --label qualification-001 --output "$OUT/$stem.json")
  [[ "$group" == ordinary ]] || args+=("--$group")
  print -r -- "Measuring $stem (completion does not mean the timing gate passed)"
  if "$ROOT/tools/nitride_perf" "${args[@]}" > "$OUT/$stem.log" 2>&1; then print MEASURED > "$OUT/$stem.status"; else print FAILED > "$OUT/$stem.status"; fi
done < "$ROOT/matrix.tsv"
/bin/cp "$ROOT/MANUAL-CHECKLIST.md" "$OUT/MANUAL-CHECKLIST.md"
print -r -- "Results: $OUT"
print 'Complete MANUAL-CHECKLIST.md in Logic, then return only this results folder for review.'
print 'Nothing was uploaded. Offline completion does NOT qualify Logic reopening, device dropouts or the30–60minute soak.'
