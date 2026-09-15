import copy
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def module(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

migration = module('migrate_six_routes_four_sends')
configuration = module('configure_send_midi')


def fixture():
    globals_ = [0] * 87
    globals_[0] = 2
    globals_[1:7] = [17, 0, 1, 0, 0, 0]
    globals_[49:53] = [13, 14, 30, 31]
    globals_[53:67] = [19, 20, 1, 7, 8, 1, 1, 37, 38, 1, 0, 0, 0, 0]
    globals_[67:] = [1, 1, 9, 98, 60, 459, 17, 4, 0, 70, 20, 10, 73, 2, 39, 40, 41, 42, 6, 60]
    row = [1, 22, 23, 0, 0, 0, 4, 3, 25, 0, 0, 4, 3, 0, 75]
    row[8] = {'v': 25, 'midi': {'channel': 3, 'cc': 9, 'enabled': 1, 'symmetric': 0, 'min': 0, 'max': 100}}
    slot = {'guid': 'WtbX', 'specs': [1, 0, 0], 'parameters': [0] + globals_ + row,
            'witchboardLatencyTrim': 0}
    return {'kind': 'disting NT preset', 'slots': [{'guid': 'usbf', 'parameters': [1, 2, 3]}, slot]}


class FourSendMigration(unittest.TestCase):
    def test_preserves_settings_and_fixed_fader(self):
        old = fixture()
        unchanged = copy.deepcopy(old)
        new = migration.migrate(old, 10)
        self.assertEqual(old, unchanged)
        self.assertEqual(new['slots'][0], old['slots'][0])
        slot = new['slots'][1]
        p = slot['parameters']
        self.assertEqual(len(p), 240)  # NT common parameter plus 239 plugin parameters.
        self.assertEqual(p[1:38], old['slots'][1]['parameters'][1:38])
        self.assertEqual(p[38:56], old['slots'][1]['parameters'][50:68])
        self.assertEqual(p[70:90], old['slots'][1]['parameters'][68:88])
        self.assertEqual(slot['witchboardSendLevels'][0], [25, 75, 0, 0])
        self.assertEqual(slot['witchboardSendMidi'][0][0], [3, 9, 0, 100, 0])
        self.assertEqual(p[90+8], 0)
        self.assertEqual(p[90+14], 25)
        for ch in range(1, 10):
            self.assertEqual(p[90+ch*15:93+ch*15], [0, 0, 0])
        self.assertEqual(slot['witchboardLatencyTrim'], 0)

    def test_refuses_unsafe_or_unknown_conversion(self):
        cases = []
        p = fixture(); p['slots'][1]['parameters'][38] = 17; cases.append(p)  # G output.
        p = fixture(); p['slots'][1]['parameters'][88+5] = 6; cases.append(p)
        p = fixture(); p['slots'][1]['parameters'][88+8]['cv'] = {'enabled': 1}; cases.append(p)
        cases.append(migration.migrate(fixture()))
        for p in cases:
            with self.assertRaises(ValueError): migration.migrate(p)

    def test_shared_mapping_does_not_replace_independent(self):
        new = migration.migrate(fixture())
        configuration.configure(new, 1, 4, 3, 10, pickup=True)
        mappings = new['slots'][1]['witchboardSendMidi'][0]
        self.assertEqual(mappings[0], [3, 9, 0, 100, 0])
        self.assertEqual(mappings[4], [3, 10, 0, 100, 1])
        with self.assertRaises(ValueError): configuration.configure(new, 1, 4, 17, 9)


if __name__ == '__main__':
    unittest.main()
