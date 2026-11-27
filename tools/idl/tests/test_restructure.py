import tempfile
import unittest
from pathlib import Path

from tools.idl.parser import load_schema
from tools.idl.restructure import SpecError, restructure

OLD = {
    "p/msg/T.idl": """// File comment of T.
#ifndef P_MSG_T_IDL
#define P_MSG_T_IDL
module p {
  module msg {
    // A is small.
    struct A {
      double x;
    };

    // B uses A.
    struct B {
      A a;                 // the a
      sequence<p::msg::A, 2> two;
    };

    struct Gone { long g; };
  };
};
#endif
""",
}


class RestructureTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.old = root / "old"
        for rel, text in OLD.items():
            (self.old / "custom" / rel).parent.mkdir(parents=True, exist_ok=True)
            (self.old / "custom" / rel).write_text(text)
        self.new = root / "new"
        self.spec = {"source": "custom", "target": "tcn",
                     "files": [{"path": "q/msg/A.idl", "module": "q::msg", "decls": ["p::msg::A"]},
                               {"path": "r/msg/B.idl", "module": "r::msg", "decls": ["p::msg::B"],
                                "preamble_from": "p/msg/T.idl"}],
                     "deleted": ["p::msg::Gone"]}

    def tearDown(self):
        self.tmp.cleanup()

    def test_split_and_requalify(self):
        renames = restructure(self.old, self.spec, self.new)
        self.assertEqual(renames, {"p::msg::A": "q::msg::A", "p::msg::B": "r::msg::B"})
        a = (self.new / "tcn" / "q" / "msg" / "A.idl").read_text()
        b = (self.new / "tcn" / "r" / "msg" / "B.idl").read_text()
        self.assertIn("#ifndef Q_MSG_A_IDL", a)
        self.assertIn("// A is small.", a)
        self.assertIn("module q {\n  module msg {", a)
        self.assertIn('#include "q/msg/A.idl"', b)
        self.assertIn("q::msg::A a;                 // the a", b)
        self.assertIn("sequence<q::msg::A, 2> two;", b)
        self.assertIn("// File comment of T.", b)
        self.assertNotIn("Gone", a + b)
        # the result parses and resolves
        decls = load_schema(self.new).decls()
        self.assertEqual(decls["r::msg::B"].members[0].type, "q::msg::A")

    def test_every_decl_placed_once(self):
        self.spec["deleted"] = []
        with self.assertRaises(SpecError):
            restructure(self.old, self.spec, self.new)  # p::msg::Gone is neither placed nor deleted
        self.spec["deleted"] = ["p::msg::Gone"]
        self.spec["files"][1]["decls"].append("p::msg::A")
        with self.assertRaises(SpecError):
            restructure(self.old, self.spec, self.new)  # p::msg::A placed twice

    def test_forward_declared_use_before_definition(self):
        (self.old / "custom" / "p" / "rpc").mkdir(parents=True)
        (self.old / "custom" / "p" / "rpc" / "R.idl").write_text(
            "module p { module rpc {\n  struct L;\n  struct P { sequence<L> l; };\n  struct L { long v; };\n}; };\n")
        self.spec["files"].append({"path": "q/rpc/R.idl", "module": "q::rpc", "decls": ["p::rpc::P", "p::rpc::L"]})
        restructure(self.old, self.spec, self.new)
        text = (self.new / "tcn" / "q" / "rpc" / "R.idl").read_text()
        self.assertIn("    struct L;", text)
        self.assertLess(text.index("struct L;"), text.index("struct P {"))

    def test_include_cycle_rejected(self):
        self.spec["files"] = [{"path": "q/msg/X.idl", "module": "q::msg", "decls": ["p::msg::B"]},
                              {"path": "q/msg/Y.idl", "module": "q::msg", "decls": ["p::msg::A"]}]
        # B before A across files is fine (Y included by X); a declaration used before its file is not:
        self.spec["files"][0]["decls"] = ["p::msg::B", "p::msg::A"]
        self.spec["files"][1]["decls"] = []
        with self.assertRaises(SpecError):
            restructure(self.old, self.spec, self.new)  # B uses A, which comes later in the same file


if __name__ == "__main__":
    unittest.main()
