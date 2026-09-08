import copy
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

script = Path(__file__).resolve().parents[1] / "scripts/migrate_v134_preset.py"
spec = importlib.util.spec_from_file_location("migration", script)
migration = importlib.util.module_from_spec(spec)
spec.loader.exec_module(migration)


class MigrationTest(unittest.TestCase):
    def test_preserves_values_mappings_and_other_slots(self):
        for guid, globals_count in (("WtSF", 68), ("WtEQ", 77), ("WtbX", 77), ("WtbX", 78), ("WtbX", 79)):
            for channels in (1, 9, 10, 11):
                params = [{"v": i, "midi": {"cc": i % 128, "min": -60, "max": 0}, "cv": {"input": 7}} for i in range(1+globals_count+15*channels)]
                old = {"kind": "disting NT preset", "slots": [{"guid": "abcd", "parameters": [123]}, {"guid": guid, "specs": [channels, 0, 0], "parameters": params, "ui": {"items": [0, 1, 2, 3, 13, 7]}, "witchboardNames": {"routes": ["my route"]}}]}
                original = copy.deepcopy(old)
                new = migration.migrate(old)
                self.assertEqual(old, original)
                self.assertEqual(new["slots"][0], old["slots"][0])
                slot = new["slots"][1]
                values = slot["parameters"]
                self.assertEqual(values[70:], params[1+globals_count:])
                self.assertEqual(values[:51], params[:51])
                self.assertEqual(values[51:58], [0, params[52], params[54], 60, 486, 20, 4])
                self.assertEqual(values[58:68], params[59:69])
                self.assertEqual(values[68], params[78] if globals_count >= 78 else 0)
                self.assertEqual(values[69], 0)
                self.assertEqual(slot["witchboardNames"], old["slots"][1]["witchboardNames"])
                self.assertEqual(slot["ui"]["items"], [0, 1, 2, 3, 0, 7])
                self.assertEqual(len(values), 70+15*channels)
                self.assertEqual(migration.migrate(new), new)

    def test_refuses_unsupported_layout_or_channel_removal(self):
        for channels, count in ((12, 250), (10, 42), (True, 95)):
            with self.assertRaises(ValueError):
                migration.migrate({"kind": "disting NT preset", "slots": [{"guid": "WtbX", "specs": [channels], "parameters": [0]*count}]})

    def test_current_preset_preserves_clamped_trim(self):
        current = {"kind": "disting NT preset", "slots": [{"guid": "WtbX", "specs": [1], "parameters": [0]*85, "witchboardLatencyTrim": -60}]}
        self.assertEqual(migration.migrate(current), current)

    def test_cli_never_overwrites(self):
        with tempfile.TemporaryDirectory() as root:
            source, dest = Path(root)/"old.json", Path(root)/"new.json"
            old = {"kind": "disting NT preset", "slots": [{"guid": "WtbX", "specs": [1], "parameters": [0]*95}]}
            source.write_text(json.dumps(old))
            subprocess.run([sys.executable, str(script), str(source), str(dest)], check=True, capture_output=True)
            expected = dest.read_bytes()
            self.assertEqual(json.loads(expected), migration.migrate(old))
            result = subprocess.run([sys.executable, str(script), str(source), str(dest)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(dest.read_bytes(), expected)
            self.assertEqual(json.loads(source.read_text()), old)


if __name__ == "__main__":
    unittest.main()
