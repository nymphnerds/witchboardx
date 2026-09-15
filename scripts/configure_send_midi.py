#!/usr/bin/env python3
"""Configure one independent send or shared-fader CC in a four-send preset copy."""
import argparse
import json
from pathlib import Path


def configure(preset, channel, target, midi_channel, cc, minimum=0, maximum=100, pickup=False):
    slots = [s for s in preset.get('slots', []) if s.get('guid') == 'WtbX']
    if len(slots) != 1:
        raise ValueError('Expected exactly one WitchboardX slot')
    slot = slots[0]
    if not 1 <= channel <= slot['specs'][0] or not 0 <= target <= 4:
        raise ValueError('Invalid channel or target')
    if not 0 <= midi_channel <= 16 or not 0 <= cc <= 119:
        raise ValueError('MIDI channel is 0 (disabled) or 1..16; CC is 0..119')
    if not 0 <= minimum <= 100 or not 0 <= maximum <= 100:
        raise ValueError('Send limits must be 0..100')
    slot['witchboardSendMidi'][channel - 1][target] = [midi_channel, cc, minimum, maximum, int(pickup)]
    return preset


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--channel', type=int, required=True, help='Witchboard channel 1..10')
    parser.add_argument('--target', choices=['1', '2', '3', '4', 'shared'], required=True)
    parser.add_argument('--midi-channel', type=int, required=True, help='1..16; 0 disables')
    parser.add_argument('--cc', type=int, required=True)
    parser.add_argument('--minimum', type=int, default=0)
    parser.add_argument('--maximum', type=int, default=100)
    parser.add_argument('--pickup', action='store_true')
    args = parser.parse_args()
    result = configure(json.loads(args.source.read_text()), args.channel,
                       4 if args.target == 'shared' else int(args.target) - 1,
                       args.midi_channel, args.cc, args.minimum, args.maximum, args.pickup)
    with args.destination.open('x') as output:
        json.dump(result, output, indent=2, ensure_ascii=False)
        output.write('\n')
    print(args.destination)


if __name__ == '__main__':
    main()
