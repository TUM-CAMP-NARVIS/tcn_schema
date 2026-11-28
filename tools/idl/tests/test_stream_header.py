"""0.11 plan 6 (#38 A1): every stream message starts with `tcn_types::msg::StreamHeader header`."""
import unittest
from pathlib import Path

from tools.idl.parser import load_schema
from tools.idl.tables import build_types

REPO = Path(__file__).resolve().parents[3]
DDS = REPO / "src" / "dds"

STREAM_HEADER = "tcn_types::msg::StreamHeader"

STREAM_MESSAGES = (
    "tcnart_msgs::msg::VideoStreamMessage",
    "tcnart_msgs::msg::Pose6DMessage",
    "tcnart_msgs::msg::TargetTrackingMessage",
    "tcnart_msgs::msg::EyeTrackingMessage",
    "tcnart_msgs::msg::HandTrackingMessage",
    "tcnart_msgs::msg::ImuMessage",
    "tcnart_msgs::msg::PersonTrackingMessage",
    "tcnart_msgs::msg::HumanPoseMessage",
    "tcnart_msgs::msg::HumanShapeMessage",
    "pcpd_msgs::msg::MeshBitstreamMessage",
)

# tcn structs that still carry a std_msgs/Header: only TcnartPresenceMessage (the descriptor dropped it in Task 4).
STD_HEADER_EXCEPTIONS = {
    "tcnart_msgs::msg::TcnartPresenceMessage",
}

T = "tcn_types::msg::"
M = "tcnart_msgs::msg::"


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

    def test_stream_descriptor_shape(self):
        # 0.11 plan 6 task 4 (#35 S1, #38 A2/A7, #37 T7/T8/T10): a common header and a union per stream kind.
        d = self.types[M + "StreamDescriptorMessage"]
        self.assertEqual((d["kind"], d["extensibility"]), ("struct", "appendable"))
        self.assertEqual([(m["name"], m["type"], m["optional"]) for m in d["members"]], [
            ("stream_topic", "string", False),
            ("stream_topic_schema", "string", False),
            ("revision", "uint32", True),
            ("frame", T + "SRFrameRef", False),
            ("clock", T + "ClockDomain", False),
            ("frame_rate", "double", False),
            ("sensor_latency_ns", "int64", True),
            ("calib_topic", "string", False),
            ("info", M + "StreamInfo", False),
            ("properties", "sequence<" + M + "BufferInfoProperty>", False),
        ])

    def test_stream_info_union(self):
        u = self.types[M + "StreamInfo"]
        self.assertEqual((u["kind"], u["extensibility"], u["discriminator"]),
                         ("union", "appendable", M + "StreamKind"))
        kinds = ("VIDEO", "TRACKER", "PERSONS", "MESH", "SURFELS", "EYE", "HAND", "IMU")
        names = ("video", "tracker", "persons", "mesh", "surfels", "eye", "hand", "imu")
        types = ("Video", "Tracker", "Person", "Mesh", "Surfel", "Eye", "Hand", "Imu")
        self.assertEqual([(c["labels"], c["member"]["name"], c["member"]["type"]) for c in u["cases"]],
                         [([f"STREAM_KIND_{k}"], n, f"{M}{t}StreamInfo") for k, n, t in zip(kinds, names, types)])

    def test_media_stream_infos(self):
        def shape(name):
            return [(m["name"], m["type"], m["optional"]) for m in self.types[M + name]["members"]]
        self.assertEqual(shape("VideoStreamInfo"), [
            ("width", "uint32", False), ("height", "uint32", False), ("step", "uint32", False),
            ("pixel_format", M + "VideoPixelFormat", False), ("semantics", M + "PixelSemantics", False),
            ("codec", M + "VideoCodec", False), ("h26x_profile", M + "VideoH26xProfile", False),
            ("bitrate", "uint32", True), ("depth_units_per_meter", "float", True),
            ("intrinsics", T + "CameraIntrinsics", True), ("codec_data", "sequence<uint8>", False),
        ])
        self.assertEqual(shape("TrackerStreamInfo"), [
            ("marker_type", T + "MarkerType", False), ("marker_system", "string", False),
            ("marker_domain", "string", False), ("targets", "sequence<" + T + "SISTargetInfo>", False),
            ("nominal_covariance", T + "SRCovariance6", True),
        ])
        self.assertEqual(shape("MeshStreamInfo"), [
            ("encoding", M + "MeshEncoding", False), ("vertices_capacity", "uint64", True),
            ("faces_capacity", "uint64", True), ("camera_id_count", "uint32", False),
        ])
        for empty in ("SurfelStreamInfo", "EyeStreamInfo", "HandStreamInfo", "ImuStreamInfo"):
            self.assertEqual(shape(empty), [], empty)
            self.assertEqual(self.types[M + empty]["extensibility"], "appendable", empty)
        self.assertNotIn(M + "BufferInfo", self.types)


if __name__ == "__main__":
    unittest.main()
