import unittest
from pathlib import Path

from tools.idl.model import Enum, Struct, Typedef, Union
from tools.idl.parser import load_schema, parse_file

REPO = Path(__file__).resolve().parents[3]
DDS = REPO / "src" / "dds"
FIX = Path(__file__).resolve().parent / "fixtures"


def _decls(schema):
    return {d.name: d for f in schema.files for d in f.decls}


class RepoSchemaTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.schema = load_schema(DDS)
        cls.decls = _decls(cls.schema)

    def test_all_repo_files_parse(self):
        # 0.11 plan 2: the ROS 2 tree is generated from Jazzy (all tiers committed); plan 3: the tcn layout;
        # plan 5 task 4: sqe_msgs/rpc/Reply.idl.
        self.assertEqual(len(self.schema.files), 473)
        n = sum(1 for f in self.schema.files for d in f.decls if isinstance(d, (Struct, Enum, Union)))
        self.assertEqual(n, 1168)  # 0.11 plan 4 deleted six types (tools/idl/fields_0_11.json); plan 5 task 2 added 11, task 3 added 6 and deleted 5, task 4 added 16 and deleted 5

    def test_sensor_type(self):
        e = self.decls["tcnart_msgs::msg::SensorType"]
        self.assertIsInstance(e, Enum)
        self.assertEqual(e.enumerators[0], ("SENSOR_TYPE_NONE", 0))
        self.assertEqual(e.enumerators[1], ("SENSOR_TYPE_VIDEO_COLOR", 1))

    def test_header(self):
        h = self.decls["std_msgs::msg::Header"]
        self.assertEqual([(m.name, m.type) for m in h.members],
                         [("stamp", "builtin_interfaces::msg::Time"), ("frame_id", "string")])
        self.assertEqual(h.extensibility, "final")

    def test_union(self):
        u = self.decls["sqe_msgs::rpc::SRStreamTrigger"]
        self.assertIsInstance(u, Union)
        self.assertEqual(u.discriminator, "sqe_msgs::rpc::SRStreamTriggerKind")

    def test_forward_decl_dropped(self):
        names = [d.name for f in self.schema.files for d in f.decls]
        self.assertEqual(names.count("pcpd_msgs::rpc::ParameterValue"), 1)

    def test_forward_decls_recorded(self):
        f = next(f for f in self.schema.files if f.path == "pcpd_msgs/rpc/Parameters.idl")
        self.assertEqual(set(f.forward_decls), {"pcpd_msgs::rpc::Parameter", "pcpd_msgs::rpc::ParameterValue",
                                                "pcpd_msgs::rpc::ParameterList"})

    def test_ros_fixed_array_member(self):
        m = self.decls["unique_identifier_msgs::msg::UUID"].members[0]
        self.assertEqual((m.type, m.array), ("uint8", [16]))

    def test_file_metadata(self):
        f = next(f for f in self.schema.files if f.path == "tcnart_msgs/msg/MediaTypes.idl")
        self.assertEqual((f.package, f.kind), ("tcnart_msgs", "msg"))
        self.assertEqual(self.schema.family["std_msgs"], "ros2")
        self.assertEqual(self.schema.family["tcnart_msgs"], "tcn")
        self.assertEqual(self.schema.family["sqe_msgs"], "tcn")
        hdr = next(f for f in self.schema.files if f.path == "std_msgs/msg/Header.idl")
        self.assertEqual(hdr.includes, ["builtin_interfaces/msg/Time.idl"])


class FixtureTest(unittest.TestCase):
    def test_typedef_array(self):
        f = parse_file(FIX / "typedef.idl", "fx/msg/typedef.idl")
        t = f.decls[0]
        self.assertIsInstance(t, Typedef)
        self.assertEqual((t.name, t.type, t.array), ("fx::msg::uint8__16", "uint8", [16]))
        self.assertEqual(f.decls[1].members[0].type, "fx::msg::uint8__16")

    def test_verbatim_multiline(self):
        f = parse_file(FIX / "verbatim.idl", "fx/msg/verbatim.idl")
        s = f.decls[0]
        self.assertEqual(s.name, "fx::msg::V")
        self.assertEqual([m.name for m in s.members], ["a", "b"])
        self.assertEqual(s.members[0].annotations.get("unit"), 'value="m"')

    def test_escaped_identifier(self):
        # IDL 4.2 7.2.3: a leading underscore escapes a keyword; the identifier is the name without it.
        f = parse_file(FIX / "escaped.idl", "fx/msg/escaped.idl")
        self.assertEqual([m.name for m in f.decls[0].members], ["map", "value"])

    def test_map(self):
        f = parse_file(FIX / "map.idl", "fx/msg/map.idl")
        self.assertEqual(f.decls[1].members[0].type, "map<fx::msg::K, string>")

    def test_optional_and_bounds(self):
        f = parse_file(FIX / "optional.idl", "fx/msg/optional.idl")
        s = f.decls[0]
        self.assertEqual(s.extensibility, "appendable")
        a, c, d, e = s.members
        self.assertTrue(a.optional)
        self.assertEqual(a.type, "long")
        self.assertEqual(c.type, "sequence<double, 21>")
        self.assertEqual(d.type, "string<8>")
        self.assertEqual((e.type, e.array), ("uint8", [3]))


if __name__ == "__main__":
    unittest.main()
