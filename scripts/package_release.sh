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
staging_dir="$(mktemp -d "$release_dir/.v105-package.XXXXXX")"
trap 'rm -rf -- "$staging_dir"' EXIT
mkdir -p "$staging_dir/programs/plug-ins"
cp "$object" "$staging_dir/programs/plug-ins/WitchboardX.o"
printf '%s\n' \
    "WitchboardX v1.0.5 shared insert returns and route latency plugin object." \
    "Copy WitchboardX.o to programs/plug-ins/ on the disting NT MicroSD card." \
    >"$staging_dir/INSTALL.txt"
(
    cd "$staging_dir"
    zip -q -r package.zip programs INSTALL.txt
)
mv "$staging_dir/package.zip" "$release_dir/WitchboardX.zip"
cp "$object" "$release_dir/WitchboardX.o"
cp "$release_dir/WitchboardX.zip" "$release_dir/release.zip"
unzip -l "$release_dir/WitchboardX.zip"
