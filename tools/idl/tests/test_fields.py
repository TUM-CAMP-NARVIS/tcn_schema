import tempfile
import unittest
from pathlib import Path

from tools.idl.fields import apply_fields
from tools.idl.parser import parse_file

SRC = """module p { module msg {
    // About M.
    @appendable struct M {
        long z;
    };

    @appendable struct S {
        long a; // A
        // leading comment of b
        long b; // B
                //   continued B
        long c; // C
        sequence<M> ms;           // the Ms
        double vals[3];
        @optional long o;
        M kept;
    };
}; };
"""


def _decls(text):
    with tempfile.TemporaryDirectory() as d:
        p = Path(d) / "x.idl"
        p.write_text(text)
        return parse_file(p, "p/msg/X.idl").decls


class FieldsTest(unittest.TestCase):
    def test_remove_members_keep_neighbours(self):
        out = apply_fields(SRC, _decls(SRC), {"remove": {"p::msg::S": ["b", "ms", "vals", "o"]}})
        self.assertIn("long a; // A\n        long c; // C\n        M kept;", out)
        for gone in ("leading comment of b", "continued B", "the Ms", "vals", "long o"):
            self.assertNotIn(gone, out)

    def test_no_double_blank_line(self):
        src = "module p { module msg { struct S {\n      long a;\n\n      long b;\n\n      long c;\n}; }; };\n"
        out = apply_fields(src, _decls(src), {"remove": {"p::msg::S": ["b"]}})
        self.assertIn("long a;\n\n      long c;", out)

    def test_adjacent_removals_leave_one_blank_line(self):
        src = ("module p { module msg { struct S {\n      long a;\n\n      // b and c\n      long b;\n      long c;\n\n"
               "      long d;\n}; }; };\n")
        out = apply_fields(src, _decls(src), {"remove": {"p::msg::S": ["b", "c"]}})
        self.assertIn("long a;\n\n      long d;", out)

    def test_delete_type(self):
        out = apply_fields(SRC, _decls(SRC), {"delete": ["p::msg::M"], "remove": {"p::msg::S": ["ms", "kept"]}})
        self.assertNotIn("About M.", out)
        self.assertNotIn("struct M", out)
        self.assertIn("@appendable struct S {", out)

    def test_optional(self):
        out = apply_fields(SRC, _decls(SRC), {"optional": {"p::msg::S": ["a", "o"]}})
        self.assertIn("@optional long a; // A", out)
        self.assertEqual(out.count("@optional long o;"), 1)   # already optional: unchanged

    def test_unknown_member_is_an_error(self):
        with self.assertRaises(ValueError):
            apply_fields(SRC, _decls(SRC), {"remove": {"p::msg::S": ["nope"]}})


if __name__ == "__main__":
    unittest.main()
