"""0.11 plan 6 (#38 A1): every stream message starts with `tcn_types::msg::StreamHeader header`."""
import unittest
from pathlib import Path

from tools.idl.parser import load_schema
from tools.idl.tables import build_types

REPO = Path(__file__).resolve().parents[3]
DDS = REPO / "src" / "dds"

STREAM_HEADER = "tcn_types::msg::StreamHeader"

# Task 3 adds PersonTrackingMessage, HumanPoseMessage and HumanShapeMessage.
STREAM_MESSAGES = (
    "tcnart_msgs::msg::VideoStreamMessage",
    "tcnart_msgs::msg::Pose6DMessage",
    "tcnart_msgs::msg::TargetTrackingMessage",
    "tcnart_msgs::msg::EyeTrackingMessage",
    "tcnart_msgs::msg::HandTrackingMessage",
    "tcnart_msgs::msg::ImuMessage",
    "pcpd_msgs::msg::MeshBitstreamMessage",
)

# tcn structs that still carry a std_msgs/Header. TcnartPresenceMessage keeps it; Task 3 removes the two person
# messages and Task 4 the descriptor.
STD_HEADER_EXCEPTIONS = {
    "tcnart_msgs::msg::TcnartPresenceMessage",
    "tcnart_msgs::msg::PersonTrackingMessage",
    "tcnart_msgs::msg::HumanPoseMessage",
    "tcnart_msgs::msg::StreamDescriptorMessage",
}


class StreamHeaderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.types = {t["name"]: t for t in build_types(load_schema(DDS), "9.9.9")["types"]}

    def test_stream_header_shape(self):
        h = self.types[STREAM_HEADER]
        self.assertEqual((h["kind"], h["extensibility"]), ("struct", "appendable"))
        self.assertEqual([(m["name"], m["type"], m["optional"]) for m in h["members"]], [
            ("capture_time_ns", "int64", False),
            ("device_time_ns", "int64", True),
            ("sequence_number", "uint64", False),
            ("group_id", "uint64", True),
            ("clock", "tcn_types::msg::ClockDomain", True),
            ("frame", "tcn_types::msg::SRFrameRef", False),
        ])

    def test_stream_messages_start_with_stream_header(self):
        for name in STREAM_MESSAGES:
            first = self.types[name]["members"][0]
            self.assertEqual((first["name"], first["type"], first["optional"]), ("header", STREAM_HEADER, False),
                             name)

    def test_no_other_std_header(self):
        with_std = {t["name"] for t in self.types.values()
                    if t["family"] == "tcn" and t["kind"] == "struct"
                    and any("std_msgs::msg::Header" in m["type"] for m in t["members"])}
        self.assertEqual(with_std, STD_HEADER_EXCEPTIONS)


if __name__ == "__main__":
    unittest.main()
