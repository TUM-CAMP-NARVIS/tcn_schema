import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from tools.idl.model import Schema
from tools.idl.parser import load_schema, parse_file
from tools.idl.tables import build_enums, build_types

REPO = Path(__file__).resolve().parents[3]
DDS = REPO / "src" / "dds"
FIX = Path(__file__).resolve().parent / "fixtures"


class TablesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        schema = load_schema(DDS)
        cls.types = build_types(schema, "9.9.9")
        cls.enums = build_enums(schema, "9.9.9")

    def _type(self, name):
        return next(t for t in self.types["types"] if t["name"] == name)

    def test_enums_json_has_stream_kind(self):
        e = next(e for e in self.enums["enums"] if e["name"] == "tcnart_msgs::msg::StreamKind")
        self.assertEqual(e["enumerators"][:2], [{"name": "STREAM_KIND_UNKNOWN", "value": 0},
                                                {"name": "STREAM_KIND_VIDEO", "value": 1}])
        self.assertEqual((e["package"], e["file"]), ("tcnart_msgs", "tcnart_msgs/msg/MediaTypes.idl"))
        self.assertEqual(self.enums["schema_version"], "9.9.9")

    def test_types_json_family(self):
        self.assertEqual(self._type("std_msgs::msg::Header")["family"], "ros2")
        self.assertEqual(self._type("tcnart_msgs::msg::TargetTrackingMessage")["family"], "tcn")

    def test_struct_members(self):
        h = self._type("std_msgs::msg::Header")
        self.assertEqual(h["kind"], "struct")
        self.assertEqual(h["members"], [
            {"name": "stamp", "type": "builtin_interfaces::msg::Time", "optional": False, "array": None},
            {"name": "frame_id", "type": "string", "optional": False, "array": None}])

    def test_union_cases(self):
        u = self._type("sqe_msgs::rpc::SRStreamTrigger")
        self.assertEqual(u["kind"], "union")
        self.assertEqual(u["discriminator"], "sqe_msgs::rpc::SRStreamTriggerKind")
        self.assertTrue(all("labels" in c and "member" in c for c in u["cases"]))

    def test_parameters_not_forward_declared(self):
        # 0.11 plan 6 task 5 (T13): ParameterValue is declared before its users, so fastddsgen stores it by value
        # and the generated operator== compares values (it compared pointers before).
        self.assertFalse(self._type("pcpd_msgs::rpc::ParameterValue")["forward_declared"])
        for name in ("pcpd_msgs::rpc::Parameter", "pcpd_msgs::rpc::ParameterList", "pcpd_msgs::rpc::ParameterValue",
                     "pcpd_msgs::rpc::ParameterDescriptor", "pcpd_msgs::rpc::ServiceInfoReply"):
            self.assertEqual(self._type(name)["pointer_members"], [], name)
            self.assertFalse(self._type(name)["eq_by_pointer"], name)
        self.assertEqual([t["name"] for t in self.types["types"] if t.get("eq_by_pointer")], [])

    def test_deterministic_cli(self):
        with tempfile.TemporaryDirectory() as d:
            outs = []
            for i in range(2):
                out = Path(d) / str(i)
                subprocess.run([sys.executable, "-I", str(REPO / "tools" / "idl_cli.py"), "tables", "--dds-root", str(DDS),
                                "--version", "1.2.3", "--out", str(out)], check=True, cwd=REPO)
                outs.append(((out / "types.json").read_bytes(), (out / "enums.json").read_bytes()))
            self.assertEqual(outs[0], outs[1])
            self.assertEqual(json.loads(outs[0][0])["schema_version"], "1.2.3")


class ForwardDeclTablesTest(unittest.TestCase):
    """fixtures/forward.idl: a forward-declared, recursive union (Parameters.idl before 0.11 plan 6 task 5)."""

    @classmethod
    def setUpClass(cls):
        f = parse_file(FIX / "forward.idl", "fx/msg/forward.idl")
        cls.types = build_types(Schema([f], {"fx": "tcn"}), "9.9.9")

    def _type(self, name):
        return next(t for t in self.types["types"] if t["name"] == name)

    def test_forward_declared_flag(self):
        self.assertTrue(self._type("fx::msg::V")["forward_declared"])
        self.assertTrue(self._type("fx::msg::L")["forward_declared"])
        self.assertFalse(self._type("fx::msg::P")["forward_declared"])

    def test_pointer_members(self):
        # fastddsgen 3.0.0 stores members whose type is a forward-declared union as raw pointers (not structs).
        self.assertEqual(self._type("fx::msg::P")["pointer_members"], ["value"])
        self.assertEqual(self._type("fx::msg::V")["pointer_members"], [])
        self.assertEqual(self._type("fx::msg::Holder")["pointer_members"], [])

    def test_eq_by_pointer_is_transitive(self):
        for name in ("fx::msg::P", "fx::msg::L", "fx::msg::V", "fx::msg::Holder"):
            self.assertTrue(self._type(name)["eq_by_pointer"], name)
        for name in ("fx::msg::Plain", "fx::msg::K"):
            self.assertFalse(self._type(name).get("eq_by_pointer", False), name)


if __name__ == "__main__":
    unittest.main()
