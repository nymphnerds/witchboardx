#!/usr/bin/env python3
"""Copy a WtSF preset (68 globals, 15/channel) to WtEQ with flat EQ defaults."""
import argparse
import copy
import json
from pathlib import Path

EQ_DEFAULTS = [259, 0, 358, 566, 0, 358, 867, 0, 358]


def migrate(preset):
    if preset.get("kind") != "disting NT preset":
        raise ValueError("Expected a disting NT preset")
    result = copy.deepcopy(preset)
    count = 0
    for slot in result.get("slots", []):
        if slot.get("guid") != "WtSF":
            continue
        channels = slot["specs"][0]
        if not isinstance(channels, int) or not 1 <= channels <= 11:
            raise ValueError("WtEQ supports 1–11 channels; channel removal is not automatic")
        parameters = slot["parameters"]
        # The first saved value is NT common bypass, outside the plugin's indices.
        if len(parameters) != 1 + 68 + channels * 15:
            raise ValueError("Unrecognised WtSF parameter layout; refusing to guess")
        if any(not isinstance(p, (int, dict)) for p in parameters):
            raise ValueError("Expected flat parameter values or mapping objects")
        slot["parameters"] = parameters[:69] + EQ_DEFAULTS[:] + parameters[69:]
        # Preserve selection of a master output control moved down by the EQ rows.
        ui = slot.get("ui", {})
        items = ui.get("items", [])
        if len(items) > 4 and 13 <= items[4] < 18:
            items[4] += 9
        slot["guid"] = "WtEQ"
        if "name" in slot:
            slot["name"] = slot["name"].replace("WtSF", "WtEQ")
        count += 1
    if not count:
        raise ValueError("No WtSF slot found")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    result = migrate(json.loads(args.source.read_text()))
    # Never overwrite the source or an existing destination.
    with args.destination.open("x") as output:
        json.dump(result, output, ensure_ascii=False, separators=(",", ":"))
        output.write("\n")
    print(args.destination)


if __name__ == "__main__":
    main()
