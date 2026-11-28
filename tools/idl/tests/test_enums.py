import tempfile
import unittest
from pathlib import Path

from tools.idl.enums import apply_enum, migration_rows
from tools.idl.parser import parse_file

SRC = """module p { module msg {
    // About E.
    @appendable enum E {
        A_X,      // the x
        // about y
        A_Y,
        A_NONE    // last today
    };
    @appendable union U switch (E) {
        case A_X: long x;
        case A_Y: double y;
    };
}; };
"""
TABLE = [["A_NONE", "E_NONE"], ["A_X", "E_X"], [None, "E_NEW"], ["A_Y", "E_Y"]]


def _parse(text):
    with tempfile.TemporaryDirectory() as d:
        p = Path(d) / "x.idl"
        p.write_text(text)
        return parse_file(p, "p/msg/X.idl")


class EnumsTest(unittest.TestCase):
    def test_rename_insert_reorder_keep_comments(self):
        f = _parse(SRC)
        out = apply_enum(SRC, f.decls, "p::msg::E", TABLE)
        e = next(d for d in _parse(out).decls if d.name == "p::msg::E")
        self.assertEqual([n for n, _ in e.enumerators], ["E_NONE", "E_X", "E_NEW", "E_Y"])
        self.assertIn("E_X,      // the x", out)
        self.assertIn("// about y\n        E_Y", out)
        self.assertIn("E_NONE,    // last today", out)
        self.assertIn("// About E.", out)
        # union labels follow
        self.assertIn("case E_X: long x;", out)
        self.assertIn("case E_Y: double y;", out)
        self.assertNotIn("A_", out)

    def test_table_must_cover_old_enumerators(self):
        f = _parse(SRC)
        with self.assertRaises(ValueError):
            apply_enum(SRC, f.decls, "p::msg::E", [["A_X", "E_X"], ["A_Y", "E_Y"]])

    def test_migration_rows(self):
        f = _parse(SRC)
        e = next(d for d in f.decls if d.name == "p::msg::E")
        self.assertEqual(migration_rows(e, TABLE), [
            ("A_NONE", 2, "E_NONE", 0), ("A_X", 0, "E_X", 1), (None, None, "E_NEW", 2), ("A_Y", 1, "E_Y", 3)])


if __name__ == "__main__":
    unittest.main()
