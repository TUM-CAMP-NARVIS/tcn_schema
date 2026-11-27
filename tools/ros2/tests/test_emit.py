import unittest

from tools.ros2.emit import emit_idl, expand_action, expand_service
from tools.ros2.rosmsg import RosType, parse_action, parse_message, parse_service


def _fields(m):
    return [(f.name, f.type) for f in m.fields]


class EmitTest(unittest.TestCase):
    def test_header_idl(self):
        m = parse_message("std_msgs", "Header", "builtin_interfaces/Time stamp\nstring frame_id\n")
        idl = emit_idl("std_msgs", "msg", "Header", [m])
        self.assertIn("#ifndef STD_MSGS_MSG_HEADER_IDL\n#define STD_MSGS_MSG_HEADER_IDL", idl)
        self.assertIn('#include "builtin_interfaces/msg/Time.idl"', idl)
        self.assertIn("module std_msgs {", idl)
        self.assertIn("@final struct Header {\n      builtin_interfaces::msg::Time stamp;\n      string frame_id;\n    };",
                      idl)
        self.assertTrue(idl.rstrip().endswith("#endif"))

    def test_constants_module(self):
        m = parse_message("shape_msgs", "SolidPrimitive", "uint8 BOX=1\nstring NAME=\"a\\\"b\"\nuint8 type\n")
        idl = emit_idl("shape_msgs", "msg", "SolidPrimitive", [m])
        self.assertIn("module SolidPrimitive_Constants {", idl)
        self.assertIn("const uint8 BOX = 1;", idl)
        self.assertIn('const string NAME = "a\\"b";', idl)

    def test_defaults(self):
        m = parse_message("p", "M", "float64 w 1.0\nbool b true\nstring s \"x\"\nint32[] v [1, 2]\n")
        idl = emit_idl("p", "msg", "M", [m])
        self.assertIn("@default (value=1.0)\n      double w;", idl)
        self.assertIn("@default (value=TRUE)\n      boolean b;", idl)
        self.assertIn('@default (value="x")\n      string s;', idl)
        self.assertIn('@default (value="(1, 2)")\n      sequence<int32> v;', idl)

    def test_fixed_array_declarator_and_sequences(self):
        m = parse_message("p", "M", "float64[9] k\nuint8[<=4] b\nstring<=8 s\np2/X[] xs\n")
        idl = emit_idl("p", "msg", "M", [m])
        self.assertIn("double k[9];", idl)
        self.assertIn("sequence<uint8, 4> b;", idl)
        self.assertIn("string<8> s;", idl)
        self.assertIn("sequence<p2::msg::X> xs;", idl)
        self.assertIn('#include "p2/msg/X.idl"', idl)

    def test_keyword_field_is_escaped(self):
        # nav_msgs/srv/GetMap_Response has a field named `map`, an IDL keyword; `_map` is the escaped identifier.
        idl = emit_idl("p", "msg", "M", [parse_message("p", "M", "float64 map\nint32 module\nint32 value\n")])
        self.assertIn("double _map;", idl)
        self.assertIn("int32 _module;", idl)
        self.assertIn("int32 value;", idl)

    def test_empty_message(self):
        idl = emit_idl("std_msgs", "msg", "Empty", [parse_message("std_msgs", "Empty", "")])
        self.assertIn("uint8 structure_needs_at_least_one_member;", idl)

    def test_service_expansion(self):
        parts = expand_service(parse_service("std_srvs", "SetBool", "bool data\n---\nbool success\nstring message\n"))
        self.assertEqual([p.name for p in parts], ["SetBool_Request", "SetBool_Response", "SetBool_Event"])
        self.assertTrue(all(p.kind == "srv" for p in parts))
        self.assertEqual(_fields(parts[2]), [
            ("info", RosType("service_msgs", "ServiceEventInfo")),
            ("request", RosType("std_srvs", "SetBool_Request", ("bounded", 1), None, "srv")),
            ("response", RosType("std_srvs", "SetBool_Response", ("bounded", 1), None, "srv"))])
        idl = emit_idl("std_srvs", "srv", "SetBool", parts)
        self.assertIn('#include "service_msgs/msg/ServiceEventInfo.idl"', idl)
        self.assertIn("sequence<std_srvs::srv::SetBool_Request, 1> request;", idl)
        self.assertNotIn('#include "std_srvs/srv/', idl)

    def test_action_expansion(self):
        a = parse_action("tf2_msgs", "LookupTransform", "string target_frame\n---\nstring r\n---\n")
        parts = expand_action(a)
        self.assertEqual([p.name for p in parts], [
            "LookupTransform_Goal", "LookupTransform_Result", "LookupTransform_Feedback",
            "LookupTransform_SendGoal_Request", "LookupTransform_SendGoal_Response", "LookupTransform_SendGoal_Event",
            "LookupTransform_GetResult_Request", "LookupTransform_GetResult_Response",
            "LookupTransform_GetResult_Event", "LookupTransform_FeedbackMessage"])
        by = {p.name: p for p in parts}
        self.assertEqual(_fields(by["LookupTransform_SendGoal_Response"]),
                         [("accepted", RosType(None, "bool")), ("stamp", RosType("builtin_interfaces", "Time"))])
        self.assertEqual(_fields(by["LookupTransform_SendGoal_Request"]), [
            ("goal_id", RosType("unique_identifier_msgs", "UUID")),
            ("goal", RosType("tf2_msgs", "LookupTransform_Goal", None, None, "action"))])
        self.assertEqual(_fields(by["LookupTransform_GetResult_Response"]), [
            ("status", RosType(None, "int8")),
            ("result", RosType("tf2_msgs", "LookupTransform_Result", None, None, "action"))])
        self.assertTrue(all(p.kind == "action" for p in parts))
        idl = emit_idl("tf2_msgs", "action", "LookupTransform", parts)
        self.assertIn("module tf2_msgs {\n  module action {", idl)
        self.assertIn("uint8 structure_needs_at_least_one_member;", idl)  # the empty feedback


if __name__ == "__main__":
    unittest.main()
