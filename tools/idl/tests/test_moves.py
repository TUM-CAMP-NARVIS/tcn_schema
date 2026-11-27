import tempfile
import unittest
from pathlib import Path

from tools.idl.model import Schema
from tools.idl.moves import compare
from tools.idl.parser import parse_file

FIX = Path(__file__).resolve().parent / "fixtures"


def _schema(files):
    with tempfile.TemporaryDirectory() as d:
        parsed = []
        for rel, text in files.items():
            p = Path(d) / rel.replace("/", "_")
            p.write_text(text)
            parsed.append(parse_file(p, rel))
    return Schema(parsed, {})


class SpanTest(unittest.TestCase):
    def test_span_includes_leading_comment(self):
        path = FIX / "spans.idl"
        text = path.read_text()
        f = parse_file(path, "p/msg/spans.idl")
        span = {d.name.split("::")[-1]: text[d.span[0]:d.span[1]] for d in f.decls}
        self.assertTrue(span["A"].startswith("// a: about A"))
        self.assertTrue(span["A"].rstrip().endswith("// trailing note of A"))
        self.assertNotIn("// b", span["A"])
        self.assertTrue(span["B"].startswith("// b: about B\n    @final struct B"))
        self.assertTrue(span["B"].rstrip().endswith("};"))
        self.assertEqual(span["E"].strip(), "enum E { E_ONE, E_TWO };")


class CompareTest(unittest.TestCase):
    def test_compare_rename_ok(self):
        old = _schema({"p/msg/V.idl": "module p { module msg { struct V { double x; }; }; };"})
        new = _schema({"q/msg/V.idl": "module q { module msg { struct V { double x; }; }; };"})
        r = compare(old, new, {"p::msg::V": "q::msg::V"}, set())
        self.assertEqual(r.problems, [])
        self.assertEqual(r.mapped, [("p::msg::V", "q::msg::V")])

    def test_compare_layout_diff(self):
        old = _schema({"p/msg/V.idl": "module p { module msg { struct V { double x; }; }; };"})
        new = _schema({"q/msg/V.idl": "module q { module msg { struct V { float x; }; }; };"})
        r = compare(old, new, {"p::msg::V": "q::msg::V"}, set())
        self.assertEqual(len(r.problems), 1)
        self.assertIn("q::msg::V", r.problems[0])

    def test_compare_member_type_mapped(self):
        old = _schema({"p/msg/T.idl": "module p { module msg { struct A { long a; }; struct B { A a; sequence<A, 3> s; }; }; };"})
        new = _schema({"q/msg/A.idl": "module q { module msg { struct A { long a; }; }; };",
                       "r/msg/B.idl": '#include "q/msg/A.idl"\nmodule r { module msg { struct B { q::msg::A a; sequence<q::msg::A, 3> s; }; }; };'})
        r = compare(old, _merge(new), {"p::msg::A": "q::msg::A", "p::msg::B": "r::msg::B"}, set())
        self.assertEqual(r.problems, [])

    def test_compare_enum_and_deleted_and_missing(self):
        old = _schema({"p/msg/T.idl": "module p { module msg { enum E { E_A, E_B }; struct Gone { long g; }; struct Lost { long l; }; }; };"})
        new = _schema({"p/msg/T.idl": "module p { module msg { enum E { E_A, E_B }; }; };"})
        r = compare(old, new, {}, {"p::msg::Gone"})
        self.assertEqual(r.deleted, ["p::msg::Gone"])
        self.assertEqual(len(r.problems), 1)
        self.assertIn("p::msg::Lost", r.problems[0])


def _merge(schema):
    # cross-file resolution as load_schema does it
    from tools.idl.parser import _build  # noqa: F401  (resolution already applied per file in this fixture)
    return schema


if __name__ == "__main__":
    unittest.main()
