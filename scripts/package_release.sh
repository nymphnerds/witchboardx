#!/usr/bin/env bash

set -euo pipefail

unset CDPATH
repo_root="$(cd -- "$(dirname -- "$0")/.." && pwd)"
object="${OBJECT:-$repo_root/plugins/Witchboard-EQ.o}"
release_dir="${RELEASE_DIR:-$repo_root/release}"

case "$object" in
    /*) ;;
    *) object="$repo_root/$object" ;;
esac

case "$release_dir" in
    /*) ;;
    *) release_dir="$repo_root/$release_dir" ;;
esac

staging_dir="$release_dir/staging/witchboard"

test -f "$object"

rm -rf -- "$release_dir"
mkdir -p "$staging_dir/programs/plug-ins"

cp "$object" "$staging_dir/programs/plug-ins/Witchboard-EQ.o"
printf '%s\n' \
    "Copy programs/plug-ins/Witchboard-EQ.o to the same path on the disting NT MicroSD card." \
    "The plug-in appears as 'Witchboard EQ' and uses GUID WtEQ." \
    "Requires a disting NT firmware version compatible with C++ plugin API v13." \
    >"$staging_dir/INSTALL.txt"

# Include the copied DSP's attribution in the distributed binary package.
sed -n '/^MIT License$/,/^\*\//p' "$repo_root/plugins/Witchboard/Witchboard.cpp" \
    | sed '$d' >"$staging_dir/SIGNALSMITH-LICENSE.txt"
test -s "$staging_dir/SIGNALSMITH-LICENSE.txt"

(
    cd "$staging_dir"
    zip -q -r "$release_dir/Witchboard-EQ.zip" programs INSTALL.txt SIGNALSMITH-LICENSE.txt
)

cp "$object" "$release_dir/Witchboard-EQ.o"
rm -rf -- "$release_dir/staging"

unzip -l "$release_dir/Witchboard-EQ.zip"
