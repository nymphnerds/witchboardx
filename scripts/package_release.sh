#!/usr/bin/env bash
set -euo pipefail
unset CDPATH
repo_root="$(cd -- "$(dirname -- "$0")/.." && pwd)"
object="${OBJECT:-$repo_root/plugins/WitchboardX.o}"
release_dir="${RELEASE_DIR:-$repo_root/release}"
case "$object" in /*) ;; *) object="$repo_root/$object" ;; esac
case "$release_dir" in /*) ;; *) release_dir="$repo_root/$release_dir" ;; esac
test -f "$object"
mkdir -p "$release_dir"
staging_dir="$(mktemp -d "$release_dir/.v103-package.XXXXXX")"
trap 'rm -rf -- "$staging_dir"' EXIT
mkdir -p "$staging_dir/programs/plug-ins"
cp "$object" "$staging_dir/programs/plug-ins/WitchboardX.o"
mkdir -p "$staging_dir/presets"
cp "$repo_root/presets/WitchboardX.json" "$staging_dir/presets/"
mkdir -p "$staging_dir/scripts" "$staging_dir/docs"
cp "$repo_root/scripts/configure_send_midi.py" \
    "$repo_root/scripts/migrate_six_routes_four_sends.py" \
    "$repo_root/scripts/migrate_channel_offsets.py" "$staging_dir/scripts/"
cp "$repo_root/docs/six-routes-four-sends.md" \
    "$repo_root/docs/channel-offsets.md" "$staging_dir/docs/"
cp "$repo_root/README.md" "$repo_root/CHANGELOG.md" "$staging_dir/"
printf '%s\n' \
    "WitchboardX v1.0.3 - up to 10 channels, six insert routes, four shared FX sends, per-channel timing offsets, trigger ducking and master filtering." \
    "Copy programs/plug-ins/*.o to the same path on the disting NT MicroSD card." \
    "The plug-in appears as 'WitchboardX' and uses GUID WtbX." \
    "Built as a no-DRAM firmware-safe release for disting NT v1.18 or later." \
    "Load the included presets/WitchboardX.json example, or read README.md before loading a preset saved for another Witchboard layout." \
    "The migration helpers cover the personal 8-route/2-send layout documented in docs/six-routes-four-sends.md; they do not migrate every historical preset." \
    "Use scripts/migrate_six_routes_four_sends.py first, then scripts/migrate_channel_offsets.py. Back up presets before migration." \
    "Automated host tests do not certify on-device audio or CPU use." \
    >"$staging_dir/INSTALL.txt"
(
    cd "$staging_dir"
    zip -q -r package.zip programs presets scripts docs README.md CHANGELOG.md INSTALL.txt
)
mv "$staging_dir/package.zip" "$release_dir/WitchboardX.zip"
cp "$object" "$release_dir/WitchboardX.o"
cp "$release_dir/WitchboardX.zip" "$release_dir/release.zip"
unzip -l "$release_dir/WitchboardX.zip"
