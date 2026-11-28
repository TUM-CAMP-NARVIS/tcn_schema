"""The contract library's C++ enum mirrors against the IDL: base type, count and ordinals (decision E6)."""
import re
import unittest
from pathlib import Path

from tools.idl.model import Enum
from tools.idl.parser import load_schema

ROOT = Path(__file__).resolve().parents[3]
INCLUDE = ROOT / "src" / "sqe" / "include" / "tcn" / "sqe"

# C++ mirror -> IDL enum. Later tasks add rows.
MIRRORS = {
    "TriggerKind": "sqe_msgs::rpc::SRStreamTriggerKind",
    "ReferencePolicy": "sqe_msgs::rpc::SRReferencePolicy",
    "EvaluationMode": "sqe_msgs::rpc::SISEvaluationMode",
    "MarkerKind": "tcn_types::msg::MarkerType",
}


def _cpp_enum(name):
    for h in sorted(INCLUDE.glob("*.h")):
        m = re.search(rf"enum\s+class\s+{name}\s*:\s*([\w:]+)\s*\{{(.*?)\}};", h.read_text(encoding="utf-8"), re.S)
        if m:
            body = re.sub(r"///?[^\n]*|/\*.*?\*/", "", m.group(2), flags=re.S)
            items = [x.strip() for x in body.split(",") if x.strip()]
            return m.group(1), [tuple(s.strip() for s in i.split("=")) for i in items]
    raise AssertionError(f"enum class {name} not found in {INCLUDE}")


class ContractEnumsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.decls = load_schema(ROOT / "src" / "dds").decls()

    def test_mirrors_match_the_idl(self):
        for cpp, fq in MIRRORS.items():
            with self.subTest(cpp):
                idl = self.decls[fq]
                self.assertIsInstance(idl, Enum)
                base, items = _cpp_enum(cpp)
                self.assertEqual(base, "std::int32_t")
                self.assertEqual(len(items), len(idl.enumerators))
                for i, item in enumerate(items):
                    # every value is spelled out, so a reordering is a visible diff
                    self.assertEqual(item[1:], (str(i),), f"{cpp}::{item[0]}")

    def test_unknown_comes_first_in_the_sqe_mirrors(self):
        for cpp in ("TriggerKind", "ReferencePolicy", "EvaluationMode"):
            with self.subTest(cpp):
                self.assertEqual(_cpp_enum(cpp)[1][0], ("Unknown", "0"))


if __name__ == "__main__":
    unittest.main()
