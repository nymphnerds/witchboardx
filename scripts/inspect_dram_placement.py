#!/usr/bin/env python3
"""Enforce the conservative stage-1 cold-code section allowlist."""
import argparse
import re
import subprocess

COLD_FUNCTIONS = {
    "constructWitchboard", "calculateRequirements", "serialise", "deserialise",
    "parameterString", "parameterUiPrefix", "pluginEntry",
}


def check(object_path, objdump):
    output = subprocess.check_output([objdump, "-t", "-C", object_path], text=True)
    found = set()
    hot_count = 0
    for line in output.splitlines():
        match = re.match(r"^[0-9a-f]+\s+\w+\s+F\s+(\S+)\s+[0-9a-f]+\s+(.+)$", line)
        if not match:
            continue
        section, symbol = match.groups()
        name = symbol.removeprefix("(anonymous namespace)::").split("(", 1)[0]
        expected = "._nt_dram" if name in COLD_FUNCTIONS else ".text"
        if section != expected:
            raise SystemExit(f"{object_path}: {symbol}: expected {expected}, found {section}")
        if name in COLD_FUNCTIONS:
            found.add(name)
        else:
            hot_count += 1
    missing = COLD_FUNCTIONS - found
    if missing:
        raise SystemExit(f"{object_path}: missing cold functions: {sorted(missing)}")
    print(f"{object_path}: all 7 intended cold functions in ._nt_dram; "
          f"{hot_count} remaining emitted functions in .text")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object")
    parser.add_argument("--objdump", default="arm-none-eabi-objdump")
    args = parser.parse_args()
    check(args.object, args.objdump)
