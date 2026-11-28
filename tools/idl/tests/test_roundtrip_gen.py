import re
import unittest
from pathlib import Path

from tools.idl.parser import load_schema
from tools.idl.roundtrip import generate
from tools.idl.tables import build_types

REPO = Path(__file__).resolve().parents[3]


class RoundtripGenTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.types = build_types(load_schema(REPO / "src" / "dds"), "0")

    def test_encoding_follows_the_family(self):
        # decisions E1, E2: tcn types are XCDR2, ROS 2 types XCDR1.
        self.assertIn("#define TCN_ENCODING ::tcn_test::Encoding::XCDR2", generate(self.types, "tcnart_msgs"))
        self.assertIn("#define TCN_ENCODING ::tcn_test::Encoding::XCDR1", generate(self.types, "std_msgs"))

    def test_emits_every_struct_and_union(self):
        src = generate(self.types, "tcnart_msgs")
        self.assertIn("TCN_ROUNDTRIP(tcnart_msgs::msg::Pose6DMessage)", src)
        self.assertIn("TCN_ROUNDTRIP(tcnart_msgs::msg::TargetTrackingMessage)", src)
        want = sum(1 for t in self.types["types"]
                   if t["package"] == "tcnart_msgs" and t["kind"] in ("struct", "union"))
        self.assertEqual(src.count("TCN_ROUNDTRIP(") + src.count("TCN_ROUNDTRIP_BYTES("), want)
        self.assertIn("void roundtrip_tcnart_msgs()", src)

    def test_includes_cdraux(self):
        src = generate(self.types, "tf2_msgs")
        headers = re.findall(r"#include \"(tf2_msgs/[^\"]+)\.h\"", src)
        self.assertIn("tf2_msgs/action/LookupTransform", headers)
        for h in headers:
            self.assertIn(f'#include "{h}CdrAux.hpp"', src)

    def test_forward_referenced_members_compare_bytes_only(self):
        src = generate(self.types, "pcpd_msgs")
        # Parameter.value is a ParameterValue declared later in Requests.idl: fastddsgen 3.0.0 stores it as a pointer
        # and its operator== compares the pointers.
        self.assertIn("TCN_ROUNDTRIP_BYTES(pcpd_msgs::rpc::Parameter)", src)
        self.assertNotIn("TCN_ROUNDTRIP(pcpd_msgs::rpc::Parameter)", src)
        self.assertIn("TCN_ROUNDTRIP_BYTES(pcpd_msgs::rpc::ParameterList)", src)
        self.assertIn("TCN_ROUNDTRIP_BYTES(pcpd_msgs::rpc::ServiceInfoReply)", src)
        self.assertIn("TCN_ROUNDTRIP(tcnart_msgs::msg::TargetTrackingMessage)", generate(self.types, "tcnart_msgs"))

    def test_enum_only_file_has_no_cdraux(self):
        # fastddsgen writes no CdrAux.hpp for a file without structs or unions (tcnart_msgs/msg/MediaTypes.idl).
        src = generate(self.types, "tcnart_msgs")
        self.assertIn('#include "tcnart_msgs/msg/MediaTypes.h"', src)
        self.assertNotIn('#include "tcnart_msgs/msg/MediaTypesCdrAux.hpp"', src)

    def test_no_enums_or_typedefs(self):
        src = generate(self.types, "unique_identifier_msgs")
        self.assertIn("TCN_ROUNDTRIP(unique_identifier_msgs::msg::UUID)", src)


if __name__ == "__main__":
    unittest.main()
