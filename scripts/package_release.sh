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
staging_dir="$(mktemp -d "$release_dir/.v100-package.XXXXXX")"
trap 'rm -rf -- "$staging_dir"' EXIT
mkdir -p "$staging_dir/programs/plug-ins"
cp "$object" "$staging_dir/programs/plug-ins/WitchboardX.o"
cp "$repo_root/scripts/migrate_v134_preset.py" "$staging_dir/"
mkdir -p "$staging_dir/presets"
cp "$repo_root/presets/WitchboardX.json" "$staging_dir/presets/"
printf '%s\n' \
    "WitchboardX personal 10-channel/8-route build - trigger ducker, master filter, output latency alignment and JSON-backed names." \
    "Copy programs/plug-ins/*.o to the same path on the disting NT MicroSD card." \
    "The plug-in appears as 'WitchboardX' and uses GUID WtbX." \
    "Built as a no-DRAM firmware-safe release for disting NT v1.18 or later." \
    "The optional presets/WitchboardX.json example is aligned for 10 channels and 8 routes, and demonstrates channel/route/FX names; its sample files and external hardware are not included." \
    "Migrate older Witchboard presets before loading: python3 migrate_v134_preset.py old.json new.json" \
    "Migration preserves channel/routing mappings, removes EQ/old ducker mappings and switches Sidechain off." \
    "Automated host tests do not certify on-device audio or CPU use." \
    >"$staging_dir/INSTALL.txt"
(
    cd "$staging_dir"
    zip -q -r package.zip programs presets INSTALL.txt migrate_v134_preset.py
)
mv "$staging_dir/package.zip" "$release_dir/WitchboardX.zip"
cp "$object" "$release_dir/WitchboardX.o"
cp "$release_dir/WitchboardX.zip" "$release_dir/release.zip"
unzip -l "$release_dir/WitchboardX.zip"
