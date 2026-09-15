import copy
import importlib.util
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('offsets', ROOT/'scripts/migrate_channel_offsets.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class OffsetMigration(unittest.TestCase):
    def test_preserves_every_existing_parameter_and_other_slots(self):
        for channels in range(1, 11):
            parameters = [0] * (90 + channels * 15)
            parameters[93] = {'v': -6, 'midi': {'channel': 3, 'cc': 9}}
            original = {'slots': [{'guid': 'usbf', 'parameters': [0, 28, 29]},
                {'guid': 'WtbX', 'specs': [channels, 0, 0], 'parameters': parameters,
                 'witchboardNames': {'channels': ['Kick']}, 'ui': {'page': 6}}]}
            snapshot = copy.deepcopy(original)
            result = module.migrate(original)
            self.assertEqual(original, snapshot)
            self.assertEqual(result['slots'][0], original['slots'][0])
            slot = result['slots'][1]
            self.assertEqual(slot['parameters'][:-2], parameters)
            self.assertEqual(slot['parameters'][-2:], [1, 0])
            self.assertEqual(slot['witchboardChannelOffsets'], [0]*channels)
            self.assertEqual(slot['ui'], original['slots'][1]['ui'])
            self.assertEqual(module.migrate(result), result)

    def test_current_preset_matches_build(self):
        preset = json.loads((ROOT/'presets/WitchboardX.json').read_text())
        for slot in preset['slots']:
            if slot['guid'] != 'WtbX':
                continue
            channels = slot['specs'][0]
            self.assertEqual(len(slot['parameters']), 92 + channels * 15)
            self.assertEqual(slot['witchboardChannelOffsets'], [0] * channels)
            self.assertEqual(slot['parameters'][-2:], [1, 0])
        self.assertEqual(module.migrate(preset), preset)

    def test_rejects_other_layout(self):
        with self.assertRaises(ValueError):
            module.migrate({'slots': [{'guid': 'WtbX', 'specs': [10], 'parameters': [0]*238}]})


if __name__ == '__main__':
    unittest.main()
