import importlib.util
import tempfile
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).parents[1] / "make_factory_hex.py"
SPEC = importlib.util.spec_from_file_location("make_factory_hex", MODULE_PATH)
factory_hex = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(factory_hex)


class FactoryHexTests(unittest.TestCase):
    def test_round_trip_and_metadata(self):
        boot = {0x0000: 0xEF, 0x0001: 0x10, 0x300006: 0x64}
        app = {0x2000: 0xEF, 0x2001: 0x20, 0x2010: 0x01}
        merged = factory_hex.build_factory(boot, app)

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "factory.hex"
            factory_hex.write_hex(output, merged)
            parsed = factory_hex.parse_hex(output)

        factory_hex.validate_factory(parsed, app)
        self.assertEqual(parsed[0x0000], 0xEF)
        self.assertEqual(parsed[0x2000], 0xEF)
        self.assertEqual(parsed[0x7FC0], ord("T"))

    def test_rejects_application_below_offset(self):
        with self.assertRaises(factory_hex.HexError):
            factory_hex.application_payload({0x0000: 0x00, 0x2000: 0x01})

    def test_rejects_bootloader_in_application_area(self):
        with self.assertRaises(factory_hex.HexError):
            factory_hex.build_factory({0x2000: 0x00}, {0x2000: 0x01})


if __name__ == "__main__":
    unittest.main()
