"""The contract library against the IDL: the C++ enum mirrors (base type, count, ordinals; decision E6), and every
type name the library registers -- wire types and the request/reply pairs of the session registry."""
import re
import unittest
from pathlib import Path

from tools.idl.model import Enum, Struct
from tools.idl.parser import load_schema

ROOT = Path(__file__).resolve().parents[3]
INCLUDE = ROOT / "src" / "sqe" / "include" / "tcn" / "sqe"

# C++ mirror -> IDL enum.
MIRRORS = {
    "TriggerKind": "sqe_msgs::rpc::SRStreamTriggerKind",
    "ReferencePolicy": "sqe_msgs::rpc::SRReferencePolicy",
    "EvaluationMode": "sqe_msgs::rpc::SISEvaluationMode",
    "MarkerKind": "tcn_types::msg::MarkerType",
    "ReplyStatus": "sqe_msgs::rpc::SISStatus",
    "Reason": "sqe_msgs::rpc::SISReason",
    "RelationState": "sqe_msgs::rpc::SISRelationState",
    "ParkReason": "sqe_msgs::rpc::SISParkReason",
    "ReoptimisePolicy": "sqe_msgs::rpc::SISReoptimisePolicy",
    "FusionPolicy": "sqe_msgs::rpc::SISFusionPolicy",
    "NodeScope": "tcn_types::msg::SRNodeScope",
    "FrameSelector": "tcn_types::msg::SRFrameSelector",
}

# The mirrors whose IDL enum has UNKNOWN at 0 (E6); MarkerKind mirrors a tcn_types enum that predates the rule.
UNKNOWN_FIRST = [cpp for cpp in MIRRORS if cpp != "MarkerKind"]

# TCN_SQE_REGISTER_WIRE_TYPE(::ns::Type, "ns::Type") -- the leading `::` skips the macro's own #define.
WIRE_TYPE = re.compile(r'TCN_SQE_REGISTER_WIRE_TYPE\(\s*::([\w:]+)\s*,\s*"([^"]*)"\s*\)')
# TCN_SQE_REGISTER_REQUEST_KEY(::ns::Req, keys::kX, RequestMode::Y, ::ns::Reply | void)
REQUEST_KEY = re.compile(
    r"TCN_SQE_REGISTER_REQUEST_KEY\(\s*::([\w:]+)\s*,\s*[\w:]+\s*,\s*RequestMode::(\w+)\s*,\s*(void|::[\w:]+)\s*\)"
)


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
        for cpp in UNKNOWN_FIRST:
            with self.subTest(cpp):
                self.assertEqual(_cpp_enum(cpp)[1][0], ("Unknown", "0"))
                self.assertTrue(self.decls[MIRRORS[cpp]].enumerators[0][0].endswith("_UNKNOWN"), cpp)

    def test_every_registered_wire_type_exists_in_the_schema(self):
        regs = WIRE_TYPE.findall((INCLUDE / "wire_type.h").read_text(encoding="utf-8"))
        self.assertGreater(len(regs), 50)
        self.assertEqual(len({name for _, name in regs}), len(regs), "a wire name is registered twice")
        for cpp, name in regs:
            with self.subTest(name):
                self.assertEqual(cpp, name, "the C++ type and its wire name differ")
                self.assertTrue(name in self.decls, f"{name}: registered, but not a type in the schema")

    def test_every_session_registration_names_schema_types(self):
        regs = REQUEST_KEY.findall((INCLUDE / "session.h").read_text(encoding="utf-8"))
        self.assertGreater(len(regs), 10)
        wire = {name for _, name in WIRE_TYPE.findall((INCLUDE / "wire_type.h").read_text(encoding="utf-8"))}
        for req, mode, reply in regs:
            with self.subTest(req):
                self.assertTrue(req in self.decls, f"{req}: request type not in the schema")
                self.assertIn(req, wire, "request type has no wire registration")
                if mode == "Publish":
                    self.assertEqual(reply, "void")
                else:
                    self.assertEqual(mode, "Query")
                    self.assertTrue(reply.lstrip(":") in self.decls, f"{reply}: reply type not in the schema")
                    self.assertIn(reply.lstrip(":"), wire, "reply type has no wire registration")

    def test_every_reply_and_notification_opens_with_the_reply_header(self):
        # #35 R1: `reply_header(reply)` reads the first member, so every reply and notification of sqe_msgs, plus
        # SISTargetChange (published on targets/changed), must start with SISReplyHeader.
        header = "sqe_msgs::rpc::SISReplyHeader"
        carriers = sorted(
            fq
            for fq, d in self.decls.items()
            if fq.startswith("sqe_msgs::")
            and isinstance(d, Struct)
            and (fq.endswith("Reply") or fq.endswith("Notification") or fq == "sqe_msgs::rpc::SISTargetChange")
        )
        self.assertIn("sqe_msgs::rpc::SISTargetChange", carriers)
        self.assertGreater(len(carriers), 15)
        for fq in carriers:
            with self.subTest(fq):
                members = self.decls[fq].members
                self.assertTrue(members, f"{fq}: no members")
                self.assertEqual((members[0].name, members[0].type, members[0].optional), ("header", header, False),
                                 f"{fq}: the first member is not `SISReplyHeader header`")


if __name__ == "__main__":
    unittest.main()
