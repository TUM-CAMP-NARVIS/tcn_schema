import tempfile
import unittest
from pathlib import Path

from tools.idl.lint import compare_baseline, lint
from tools.idl.model import Schema
from tools.idl.parser import parse_file

GUARD = "#ifndef FX_MSG_{stem}_IDL\n#define FX_MSG_{stem}_IDL\n{body}\n#endif\n"
GOOD_BODY = ("module fx { module msg {\n"
             "  @appendable enum ColorKind { COLOR_KIND_UNKNOWN, COLOR_KIND_RED };\n"
             "  @appendable struct Paint { ColorKind kind; };\n"
             "}; };")


def _schema(files, family="tcn"):
    """files: {rel_path: text} -> (Schema, {rel: text})"""
    with tempfile.TemporaryDirectory() as d:
        parsed = []
        for rel, text in files.items():
            p = Path(d) / rel.replace("/", "_")
            p.write_text(text)
            parsed.append(parse_file(p, rel, family))
    return Schema(parsed, {"fx": family}), files


def _good(stem="Paint", body=GOOD_BODY):
    return GUARD.format(stem=stem.upper(), body=body)


class LintRulesTest(unittest.TestCase):
    def rules(self, files, family="tcn"):
        return [f.rule for f in lint(*_schema(files, family))]

    def test_conforming_file_has_no_findings(self):
        self.assertEqual(self.rules({"fx/msg/Paint.idl": _good()}), [])

    def test_basename_unique(self):
        r = self.rules({"fx/msg/Paint.idl": _good(),
                        "fx/rpc/Paint.idl": _good().replace("FX_MSG_", "FX_RPC_").replace("Paint {", "Paint2 {")
                                                  .replace("ColorKind", "ToneKind").replace("COLOR_KIND", "TONE_KIND")})
        self.assertEqual(r, ["basename-unique"])

    def test_file_pascalcase(self):
        self.assertEqual(self.rules({"fx/msg/paint.idl": _good("paint")}), ["file-pascalcase"])

    def test_include_guard(self):
        self.assertEqual(self.rules({"fx/msg/Paint.idl": GOOD_BODY}), ["include-guard"])

    def test_enum_first_unknown(self):
        body = GOOD_BODY.replace("COLOR_KIND_UNKNOWN, COLOR_KIND_RED", "COLOR_KIND_RED, COLOR_KIND_BLUE")
        self.assertEqual(self.rules({"fx/msg/Paint.idl": _good(body=body)}), ["enum-first-unknown"])

    def test_enum_prefix(self):
        body = GOOD_BODY.replace("COLOR_KIND_RED", "RED")
        self.assertEqual(self.rules({"fx/msg/Paint.idl": _good(body=body)}), ["enum-prefix"])

    def test_extensibility_explicit_tcn(self):
        body = GOOD_BODY.replace("@appendable struct", "struct")
        self.assertEqual(self.rules({"fx/msg/Paint.idl": _good(body=body)}), ["extensibility-explicit"])

    def test_extensibility_ros2_must_be_final(self):
        body = "module fx { module msg { @appendable struct Pose { double x; }; }; };"
        self.assertEqual(self.rules({"fx/msg/Pose.idl": _good("Pose", body)}, family="ros2"),
                         ["extensibility-explicit"])
        body = "module fx { module msg { @final struct Pose { double x; }; }; };"
        self.assertEqual(self.rules({"fx/msg/Pose.idl": _good("Pose", body)}, family="ros2"), [])

    def test_no_enums_module(self):
        body = GOOD_BODY.replace("module msg {", "module msg { module enums { };", 1)
        self.assertEqual(self.rules({"fx/msg/Paint.idl": _good(body=body)}), ["no-enums-module"])


class BaselineTest(unittest.TestCase):
    def test_new_and_stale(self):
        findings = lint(*_schema({"fx/msg/Paint.idl": GOOD_BODY}))  # one include-guard finding
        key = findings[0].key()
        new, stale = compare_baseline(findings, {key})
        self.assertEqual((new, stale), ([], []))
        new, stale = compare_baseline(findings, set())
        self.assertEqual(len(new), 1)
        new, stale = compare_baseline([], {key})
        self.assertEqual(stale, [key])


class RepoBaselineTest(unittest.TestCase):
    def test_repo_matches_baseline(self):
        import subprocess, sys
        repo = Path(__file__).resolve().parents[3]
        r = subprocess.run([sys.executable, "-I", str(repo / "tools" / "idl_cli.py"), "lint", "--dds-root",
                            str(repo / "src" / "dds"), "--baseline", str(repo / "tools" / "idl" / "lint_baseline.txt")],
                           capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout)


if __name__ == "__main__":
    unittest.main()
