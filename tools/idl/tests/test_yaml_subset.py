"""The YAML subset reader (0.11 plan 6): enough YAML for the joint-set table, standard library only."""
import unittest
from pathlib import Path

from tools.idl.yaml_subset import YamlSubsetError, load

REPO = Path(__file__).resolve().parents[3]
TABLE = REPO / "src" / "dds" / "tcn" / "tcnart_msgs" / "msg" / "joint_sets.yaml"


class YamlSubsetTest(unittest.TestCase):
    def test_block_and_flow(self):
        self.assertEqual(load("a: 1\nb: [x, y]\nc:\n  - {n: p, q: null}\n"),
                         {"a": 1, "b": ["x", "y"], "c": [{"n": "p", "q": None}]})

    def test_comments(self):
        self.assertEqual(load("# head\na: 1   # c\nb: \"x # y\"\nc: x#y\n"), {"a": 1, "b": "x # y", "c": "x#y"})

    def test_continued_plain_scalar(self):
        self.assertEqual(load("a: one two\n  three\n    four\nb: 2\n"), {"a": "one two three four", "b": 2})
        self.assertEqual(load("- x: one\n    two\n  y: 3\n"), [{"x": "one two", "y": 3}])

    def test_scalars(self):
        self.assertEqual(load("a: -1\nb: 1.5\nc: 0-21\nd: true\ne: ~\nf: 'it''s'\ng: \"q\\\"\\n\"\nh: [1, -1.0, a b]\n"),
                         {"a": -1, "b": 1.5, "c": "0-21", "d": True, "e": None, "f": "it's", "g": "q\"\n",
                          "h": [1, -1.0, "a b"]})

    def test_nested_block(self):
        self.assertEqual(load("a:\n  b:\n    c: 1\n  d:\n  - 1\n  - 2\ne: {f: {g: [1, 2]}}\n"),
                         {"a": {"b": {"c": 1}, "d": [1, 2]}, "e": {"f": {"g": [1, 2]}}})

    def test_anchor_rejected_with_line(self):
        with self.assertRaises(YamlSubsetError) as cm:
            load("a: &x 1\n")
        self.assertEqual(cm.exception.line, 1)

    def test_unsupported_rejected(self):
        for text, line in (("a: 1\nb: *x\n", 2), ("a: !!str 1\n", 1), ("a: |\n  x\n", 1), ("a: >\n  x\n", 1),
                           ("a: 1\n---\nb: 2\n", 2), ("a: 1\na: 2\n", 2), ("a: yes\n", 1), ("a: [1, 2\n", 1)):
            with self.subTest(text=text):
                with self.assertRaises(YamlSubsetError) as cm:
                    load(text)
                self.assertEqual(cm.exception.line, line)

    def test_matches_pyyaml_on_the_joint_set_table(self):
        try:
            import yaml
        except ImportError:
            self.skipTest("PyYAML is not installed")
        text = TABLE.read_text(encoding="utf-8")
        self.assertEqual(load(text), yaml.safe_load(text))


if __name__ == "__main__":
    unittest.main()
