import tempfile
import unittest
from pathlib import Path

from tools.idl.annotate import annotate
from tools.idl.parser import parse_file

SRC = """module p { module msg {
    // About S. Mentions struct S in a comment.
    @verbatim(language="comment", text="x")
    struct S {
        long a;
    };

    enum E {
        E_UNKNOWN
    };

    @final
    struct F {
        long b;
    };

    struct V { long c; };
}; };
"""


def _run(text, final):
    with tempfile.TemporaryDirectory() as d:
        p = Path(d) / "x.idl"
        p.write_text(text)
        decls = parse_file(p, "p/msg/X.idl").decls
    return annotate(text, decls, final)


class AnnotateTest(unittest.TestCase):
    def test_appendable_and_final(self):
        out = _run(SRC, {"p::msg::V"})
        self.assertIn('@verbatim(language="comment", text="x")\n    @appendable struct S {', out)
        self.assertIn("// About S. Mentions struct S in a comment.", out)
        self.assertIn("    @appendable enum E {", out)
        self.assertIn("    @final\n    struct F {", out)       # already annotated: unchanged
        self.assertIn("    @final struct V { long c; };", out)

    def test_idempotent(self):
        once = _run(SRC, {"p::msg::V"})
        self.assertEqual(_run(once, {"p::msg::V"}), once)


if __name__ == "__main__":
    unittest.main()
