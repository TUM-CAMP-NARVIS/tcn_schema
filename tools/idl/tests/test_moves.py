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


class RelocatedAndExtensibilityTest(unittest.TestCase):
    def test_relocated_header_is_reported(self):
        # Same name, new header (tcnart_msgs::msg::SensorType: Types.h -> MediaTypes.h).
        import json
        old = _schema({"p/msg/Types.idl": "module p { module msg { enum E { E_A }; }; };"})
        new = _schema({"p/msg/Media.idl": "module p { module msg { enum E { E_A }; }; };"})
        r = compare(old, new, {}, set())
        self.assertEqual(r.problems, [])
        self.assertEqual(r.relocated, [("p::msg::E", "p/msg/Types.h", "p/msg/Media.h")])
        from tools.idl.moves import write_reports
        with tempfile.TemporaryDirectory() as d:
            js, md = Path(d) / "m.json", Path(d) / "m.md"
            write_reports(r, old, new, js, md)
            data = json.loads(js.read_text())
            self.assertEqual(data["relocated"], [{"name": "p::msg::E", "old_header": "p/msg/Types.h",
                                                   "new_header": "p/msg/Media.h"}])
            self.assertIn("`p::msg::E` | `p/msg/Types.h` | `p/msg/Media.h`", md.read_text())

    def test_extensibility_change_is_a_problem(self):
        old = _schema({"p/msg/V.idl": "module p { module msg { @final struct V { double x; }; }; };"})
        new = _schema({"p/msg/V.idl": "module p { module msg { @appendable struct V { double x; }; }; };"})
        self.assertEqual(len(compare(old, new, {}, set()).problems), 1)

    def test_member_annotation_change_is_a_problem(self):
        old = _schema({"p/msg/V.idl": "module p { module msg { struct V { @key double x; }; }; };"})
        new = _schema({"p/msg/V.idl": "module p { module msg { struct V { double x; }; }; };"})
        self.assertEqual(len(compare(old, new, {}, set()).problems), 1)


def _merge(schema):
    # cross-file resolution as load_schema does it
    from tools.idl.parser import _build  # noqa: F401  (resolution already applied per file in this fixture)
    return schema


if __name__ == "__main__":
    unittest.main()


class ExpectTest(unittest.TestCase):
    OLD = {"p/msg/V.idl": """module p { module msg {
        struct V { double x; double y; };
        @final struct W { double z; };
        enum E { A_ONE, A_TWO };
        union U switch (E) { case A_ONE: double one; case A_TWO: long two; };
    }; };"""}

    def _cmp(self, new_text, expect):
        from tools.idl.moves import compare
        return compare(_schema(self.OLD), _schema({"p/msg/V.idl": new_text}), {}, set(), expect=expect)

    NEW = """module p { module msg {
        @appendable struct V { @optional double x; };
        @final struct W { double z; };
        @appendable enum E { E_UNKNOWN, E_ONE, E_TWO };
        @appendable union U switch (E) { case E_ONE: double one; case E_TWO: long two; };
    }; };"""
    EXPECT = {"extensibility": {"*": "appendable"},
              "removed_members": {"p::msg::V": ["y"]},
              "optional": {"p::msg::V": ["x"]},
              "enums": {"p::msg::E": [[None, "E_UNKNOWN"], ["A_ONE", "E_ONE"], ["A_TWO", "E_TWO"]]}}

    def test_expected_changes_pass(self):
        r = self._cmp(self.NEW, self.EXPECT)
        self.assertEqual(r.problems, [])

    def test_unexpected_change_still_fails(self):
        r = self._cmp(self.NEW.replace("long two", "double two"), self.EXPECT)
        self.assertEqual(len(r.problems), 1)
        r = self._cmp(self.NEW, dict(self.EXPECT, removed_members={}))
        self.assertEqual(len(r.problems), 1)

    def test_star_does_not_override_explicit_final(self):
        r = self._cmp(self.NEW.replace("@final struct W", "@appendable struct W"), self.EXPECT)
        self.assertEqual(len(r.problems), 1)


class ExpectTypoTest(unittest.TestCase):
    def test_expect_naming_a_missing_member_is_an_error(self):
        from tools.idl.moves import compare
        s = {"p/msg/V.idl": "module p { module msg { struct V { double x; }; }; };"}
        with self.assertRaises(ValueError):
            compare(_schema(s), _schema(s), {}, set(), expect={"removed_members": {"p::msg::V": ["typo"]}})
        with self.assertRaises(ValueError):
            compare(_schema(s), _schema(s), {}, set(), expect={"optional": {"p::msg::V": ["typo"]}})
