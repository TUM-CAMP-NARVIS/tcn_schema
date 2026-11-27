import tempfile
import unittest
from pathlib import Path

from tools.fastddsgen_fixup import CLAMP, fix_dir, fix_file


class FixupTest(unittest.TestCase):
    def test_clamps_oversized_typesize(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "XCdrAux.hpp"
            p.write_text("constexpr uint32_t a_max_cdr_typesize {7267171100UL};\n"
                         "constexpr uint32_t a_max_key_cdr_typesize {0UL};\n"
                         "constexpr uint32_t b_max_cdr_typesize {352UL};\n")
            self.assertTrue(fix_file(p))
            text = p.read_text()
            self.assertIn(f"a_max_cdr_typesize {{{CLAMP}UL}};", text)
            self.assertIn("a_max_key_cdr_typesize {0UL};", text)
            self.assertIn("b_max_cdr_typesize {352UL};", text)
            self.assertFalse(fix_file(p))  # idempotent

    def test_clamp_does_not_wrap_in_pubsubtype(self):
        # The generated PubSubType does: size += alignment(size, 4); m_typeSize = size + 4 (uint32_t).
        size = CLAMP + (4 - CLAMP % 4) % 4 + 4
        self.assertLessEqual(size, 2 ** 32 - 1)
        self.assertGreater(CLAMP, 2 ** 31)

    def test_fix_dir(self):
        # The build passes the output directory, not every header (Windows command-line length).
        with tempfile.TemporaryDirectory() as d:
            sub = Path(d) / "msg"
            sub.mkdir()
            (sub / "ACdrAux.hpp").write_text("constexpr uint32_t a_max_cdr_typesize {9999999999UL};\n")
            (sub / "A.h").write_text("constexpr uint32_t a_max_cdr_typesize {9999999999UL};\n")
            self.assertEqual(fix_dir(sub), 1)
            self.assertIn(str(CLAMP), (sub / "ACdrAux.hpp").read_text())
            self.assertIn("9999999999", (sub / "A.h").read_text())


if __name__ == "__main__":
    unittest.main()
