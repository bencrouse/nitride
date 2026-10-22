#!/bin/zsh
# Internal fresh-install qualification only. No overwrite, sudo or global audio settings.
set -euo pipefail
umask 077
ACTION=${1:?install, verify or uninstall}
ROOT=${2:?qualification package root}
ROOT=${ROOT:A}
AU="$HOME/Library/Audio/Plug-Ins/Components/nitride.component"
APP="$HOME/Applications/Nitride Qualification.app"
PRESETS="$HOME/Library/Audio/Presets/nitride/nitride"
OWNER="$HOME/Library/Application Support/nitride/Qualification/qualification-001"
au_created=0
app_created=0
created_presets=()

die() { print -u2 -- "$*"; exit 1; }
digest() { local line; line=$(/usr/bin/shasum -a 256 "$1"); print -r -- "${line%% *}"; }
safe_path() {
  local path="$1"
  while [[ "$path" != / ]]; do
    [[ ! -L "$path" ]] || die "Refusing symbolic link: $path"
    path=${path:h}
  done
}
source_for() {
  case "$1" in au) print -r -- "$ROOT/payload/nitride.component/$2";; app) print -r -- "$ROOT/payload/nitride.app/$2";; preset) print -r -- "$ROOT/payload/presets/$2";; *) die 'Unknown installation kind';; esac
}
target_for() {
  case "$1" in au) print -r -- "$AU/$2";; app) print -r -- "$APP/$2";; preset) print -r -- "$PRESETS/$2";; *) die 'Unknown installation kind';; esac
}
valid_relative() { [[ -n "$1" && "$1" != /* && "$1" != .. && "$1" != ../* && "$1" != */../* && "$1" != */.. ]] || die 'Unsafe manifest path'; }
safe_path "$AU"; safe_path "$APP"; safe_path "$PRESETS"; safe_path "$OWNER"
[[ "$ACTION" == install || "$ACTION" == verify || "$ACTION" == uninstall ]] || die 'Unknown action'
(cd "$ROOT" && /usr/bin/shasum -a 256 -c checksums.sha256) || die 'Package checksum failure'

if [[ "$ACTION" != install ]]; then
  [[ -f "$OWNER/candidate.sha256" && "$(<"$OWNER/candidate.sha256")" == "$(digest "$ROOT/candidate.json")" ]] || die 'No matching qualification ownership record'
fi
while IFS=$'\t' read -r kind relative expected; do
  valid_relative "$relative"
  source=$(source_for "$kind" "$relative"); target=$(target_for "$kind" "$relative")
  safe_path "$target"
  [[ "$(digest "$source")" == "$expected" ]] || die "Source checksum mismatch: $relative"
  if [[ "$ACTION" == install ]]; then
    [[ ! -e "$target" && ! -L "$target" ]] || die "Existing file will NOT be overwritten: $target"
  else
    [[ -f "$target" && "$(digest "$target")" == "$expected" ]] || die "Owned file changed/missing; nothing removed: $target"
  fi
done < "$ROOT/install.tsv"
[[ "$ACTION" != verify ]] || { print 'Installed qualification files verify; user additions are ignored.'; exit 0; }

remove_known_files() {
  local mode=${1:-all} kind relative expected target
  while IFS=$'\t' read -r kind relative expected; do
    if [[ "$mode" == rollback ]]; then
      [[ "$kind" != au || "$au_created" == 1 ]] || continue
      [[ "$kind" != app || "$app_created" == 1 ]] || continue
      if [[ "$kind" == preset ]] && (( ! ${created_presets[(Ie)$relative]} )); then continue; fi
    fi
    target=$(target_for "$kind" "$relative")
    if [[ -f "$target" && ! -L "$target" && "$(digest "$target")" == "$expected" ]]; then /bin/rm -f -- "$target"; fi
  done < "$ROOT/install.tsv"
  local trees=()
  [[ "$mode" != rollback || "$au_created" == 1 ]] && trees+=("$AU")
  [[ "$mode" != rollback || "$app_created" == 1 ]] && trees+=("$APP")
  for tree in "${trees[@]}"; do
    if [[ -d "$tree" && ! -L "$tree" ]]; then /usr/bin/find "$tree" -depth -type d -exec /bin/rmdir {} \; 2>/dev/null || true; fi
  done
}
if [[ "$ACTION" == uninstall ]]; then
  remove_known_files
  /bin/rm -f -- "$OWNER/candidate.sha256"
  /bin/rmdir -- "$OWNER" 2>/dev/null || true
  print 'Removed only unchanged owned qualification files. User presets/preferences remain.'
  exit 0
fi
[[ ! -e "$AU" && ! -e "$APP" && ! -e "$OWNER" ]] || die 'Fresh install required: preserve existing Nitride and ask for a versioned update. No replacement is attempted.'
created=0
cleanup() {
  local result=$?
  if (( result != 0 && created )); then remove_known_files rollback; fi
  return $result
}
trap cleanup EXIT
/bin/mkdir -p -- "${AU:h}" "${APP:h}" "$PRESETS" "${OWNER:h}"
/bin/mkdir -- "$AU"
created=1
au_created=1
/usr/bin/ditto "$ROOT/payload/nitride.component" "$AU"
/bin/mkdir -- "$APP"
app_created=1
/usr/bin/ditto "$ROOT/payload/nitride.app" "$APP"
while IFS=$'\t' read -r kind relative expected; do
  [[ "$kind" == preset ]] || continue
  source=$(source_for "$kind" "$relative"); target=$(target_for "$kind" "$relative")
  /bin/mkdir -p -- "${target:h}"
  # noclobber creates exclusively, refusing a path that appeared after preflight.
  set -o noclobber
  exec {fd}> "$target"
  unsetopt noclobber
  created_presets+=("$relative")
  /bin/cat "$source" >&$fd
  exec {fd}>&-
done < "$ROOT/install.tsv"
/usr/bin/codesign --verify --strict "$AU"
/usr/bin/codesign --verify --strict "$APP"
/bin/mkdir -- "$OWNER"
digest "$ROOT/candidate.json" > "$OWNER/candidate.sha256"
/usr/bin/touch "$AU"
print 'Installed internal qualification AU, standalone and53factory files. Restart Logic before testing.'
