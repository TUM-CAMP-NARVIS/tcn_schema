import tempfile
import unittest
from pathlib import Path

from tools.fastddsgen_fixup import fix_file


class FixupTest(unittest.TestCase):
    def test_clamps_oversized_typesize(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "XCdrAux.hpp"
            p.write_text("constexpr uint32_t a_max_cdr_typesize {7267171100UL};\n"
                         "constexpr uint32_t a_max_key_cdr_typesize {0UL};\n"
                         "constexpr uint32_t b_max_cdr_typesize {352UL};\n")
            self.assertTrue(fix_file(p))
            text = p.read_text()
            self.assertIn("a_max_cdr_typesize {4294967295UL};", text)
            self.assertIn("a_max_key_cdr_typesize {0UL};", text)
            self.assertIn("b_max_cdr_typesize {352UL};", text)
            self.assertFalse(fix_file(p))  # idempotent


if __name__ == "__main__":
    unittest.main()
