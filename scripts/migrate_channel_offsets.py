#!/usr/bin/env python3
"""Append the Offset editor to a six-route/four-send WitchboardX preset."""
import argparse
import copy
import json
from pathlib import Path


def migrate(preset):
    result = copy.deepcopy(preset)
    slots = [slot for slot in result.get('slots', []) if slot.get('guid') == 'WtbX']
    if not slots:
        raise ValueError('No WitchboardX slot found')
    for slot in slots:
        channels = slot['specs'][0]
        if not 1 <= channels <= 10:
            raise ValueError('Expected 1–10 Witchboard channels')
        params = slot['parameters']
        old_count = 1 + 89 + channels * 15  # Includes NT common parameter.
        if len(params) == old_count:
            params.extend([1, 0])
        elif len(params) != old_count + 2:
            raise ValueError('Expected a flat six-route/four-send preset; migrate older layouts first')
        slot.setdefault('witchboardChannelOffsets', [0] * channels)
        offsets = slot['witchboardChannelOffsets']
        if (len(offsets) != channels or
                any(type(v) is not int or not -300 <= v <= 0 for v in offsets)):
            raise ValueError('Invalid channel offsets (expected tenths of ms, -300..0)')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    result = migrate(json.loads(args.source.read_text()))
    with args.destination.open('x') as stream:
        json.dump(result, stream, indent=2)
        stream.write('\n')


if __name__ == '__main__':
    main()
