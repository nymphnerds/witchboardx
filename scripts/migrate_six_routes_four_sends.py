#!/usr/bin/env python3
"""Migrate the personal 8-route/2-send preset to the 6-route/4-send editor build."""
import argparse
import copy
import json
from pathlib import Path

DISABLED = [0, 0, 0, 100, 0]


def value(parameter):
    return parameter['v'] if isinstance(parameter, dict) else parameter


def send_mapping(parameter):
    if not isinstance(parameter, dict):
        return DISABLED.copy()
    if set(parameter) - {'v', 'midi'}:
        raise ValueError('Send mix has non-MIDI mappings; migrate these manually')
    midi = parameter.get('midi', {})
    if not midi.get('enabled'):
        return DISABLED.copy()
    if set(midi) - {'channel', 'cc', 'enabled', 'symmetric', 'min', 'max'} or midi.get('symmetric'):
        raise ValueError('Unsupported send MIDI mapping; refusing to discard settings')
    result = [midi['channel'], midi['cc'], midi['min'], midi['max'], 0]
    if not (1 <= result[0] <= 16 and 0 <= result[1] <= 119
            and 0 <= result[2] <= 100 and 0 <= result[3] <= 100):
        raise ValueError('Send MIDI mapping is outside supported ranges')
    return result


def migrate(preset, channels=None):
    result = copy.deepcopy(preset)
    count = 0
    for slot in result.get('slots', []):
        if slot.get('guid') != 'WtbX':
            continue
        old_channels = slot['specs'][0]
        new_channels = old_channels if channels is None else channels
        if not 1 <= old_channels <= new_channels <= 10:
            raise ValueError('Supports 1..10 channels; cannot discard source channels')
        params = slot['parameters']
        if len(params) != 1 + 87 + 15 * old_channels or 'witchboardSendLevels' in slot:
            raise ValueError('Expected the personal 87-global, 8-route/2-send schema')
        old = params[1:88]
        # G/H may only be removed when their endpoints and mappings are unused.
        for route in (6, 7):
            fields = old[1 + route * 6:1 + (route + 1) * 6]
            if any(isinstance(p, dict) for p in fields) or any(value(p) for p in fields[:4]):
                raise ValueError('Route G/H is configured; move it to A-F before migration')
        globals_ = old[:37] + old[49:67] + [0, 0, 1, 0, 0, 1, 0] * 2 + old[67:]
        assert len(globals_) == 89
        rows, levels, mappings = [], [], []
        for ch in range(old_channels):
            row = params[88 + 15 * ch:88 + 15 * (ch + 1)]
            for field in (5, 6, 7, 10, 11, 12):
                p = row[field]
                if not 0 <= value(p) < 6:
                    raise ValueError(f'Channel {ch + 1} assigns a slot to removed route G/H')
                if isinstance(p, dict):
                    raise ValueError('Mapped insert slot assignment needs manual range review')
            levels.append([value(row[8]), value(row[14]), 0, 0])
            mappings.append([send_mapping(row[8]), send_mapping(row[14]),
                             DISABLED.copy(), DISABLED.copy(), DISABLED.copy()])
            row[14], row[8] = value(row[8]), 0
            rows.extend(row)
        for ch in range(old_channels, new_channels):
            # Added channels start disabled with no inputs or sends.
            rows.extend([0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 1, 2, 0, 0])
            levels.append([0] * 4)
            mappings.append([DISABLED.copy() for _ in range(5)])
        slot['parameters'] = params[:1] + globals_ + rows
        slot['specs'][0] = new_channels
        slot['witchboardSendLevels'] = levels
        slot['witchboardSendMidi'] = mappings
        names = slot.setdefault('witchboardNames', {})
        names['routes'] = names.get('routes', [f'Route {c}' for c in 'ABCDEF'])[:6]
        names['fx'] = names.get('fx', ['FX Send 1', 'FX Send 2'])[:2] + ['FX Send 3', 'FX Send 4']
        names.setdefault('channels', [f'Channel {i+1}' for i in range(old_channels)])
        names['channels'] += [f'Channel {i+1}' for i in range(old_channels, new_channels)]
        if 'ui' in slot:
            slot['ui']['page'] = 0
            slot['ui']['items'] = [0] * len(slot['ui'].get('items', []))
        count += 1
    if not count:
        raise ValueError('No WitchboardX slot found')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--channels', type=int)
    args = parser.parse_args()
    result = migrate(json.loads(args.source.read_text()), args.channels)
    with args.destination.open('x') as output:
        json.dump(result, output, indent=2, ensure_ascii=False)
        output.write('\n')
    print(args.destination)


if __name__ == '__main__':
    main()
