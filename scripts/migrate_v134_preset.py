#!/usr/bin/env python3
"""Convert legacy Witchboard presets to the v1.34 no-EQ trigger-ducker layout.

Preserve channel, route, filter, master-gain and compatible input/depth mappings.
The old sidechain has different semantics: disable it and initialise the new
controls for audition. Removed sidechain/EQ mappings are intentionally dropped.
"""
import argparse
import copy
import json
from pathlib import Path


def migrate(preset):
    if preset.get("kind") != "disting NT preset":
        raise ValueError("Expected a disting NT preset")
    result = copy.deepcopy(preset)
    count = 0
    for slot in result.get("slots", []):
        guid = slot.get("guid")
        if guid not in ("WtSF", "WtEQ", "WtbX"):
            continue
        channels = slot["specs"][0]
        if type(channels) is not int or not 1 <= channels <= 11:
            raise ValueError("WitchboardX supports 1–11 channels; channel removal is not automatic")
        parameters = slot["parameters"]
        if any(type(p) not in (int, dict) for p in parameters):
            raise ValueError("Expected flat parameter values or mapping objects")
        globals_count = len(parameters) - 1 - channels * 15
        expected = {"WtSF": (68,), "WtEQ": (77,), "WtbX": (69, 77, 78, 79)}[guid]
        if globals_count not in expected:
            raise ValueError("Unrecognised parameter layout; refusing to guess")
        if globals_count != 69:
            # Saved index 0 is the NT common bypass; plugin indices start at 1.
            old = parameters[1:1 + globals_count]
            new = old[:50]
            new += [0, old[51], old[53], 60, 486, 20, 4]
            new += old[58:68]  # Five filter and five master routing parameters.
            new += [old[77] if globals_count >= 78 else 0, 0]
            assert len(new) == 69
            slot["parameters"] = parameters[:1] + new + parameters[1 + globals_count:]
            # Master-page rows moved. Keep the selected page and channel rows,
            # but select the first master control instead of a stale EQ row.
            items = slot.get("ui", {}).get("items", [])
            if len(items) > 4:
                items[4] = 0
            slot.pop("witchboardLatencyTrim", None)
        slot["guid"] = "WtbX"
        if guid != "WtbX":
            slot["name"] = "WitchboardX"
        count += 1
    if not count:
        raise ValueError("No Witchboard slot found")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    result = migrate(json.loads(args.source.read_text()))
    with args.destination.open("x") as output:
        json.dump(result, output, ensure_ascii=False, separators=(",", ":"))
        output.write("\n")
    print(args.destination)


if __name__ == "__main__":
    main()
