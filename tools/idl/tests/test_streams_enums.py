"""0.11 plan 6: the new and changed enums of Global Constraints, exactly (Review Focus 1: a mirror that keeps old
ordinals decodes plausible wrong values). Ordinal = position; every enum is @appendable with UNKNOWN (or NONE) at
0 (E6). Each task adds the rows of the enums it lands."""
import unittest
from pathlib import Path

from tools.idl.model import Enum
from tools.idl.parser import load_schema
from tools.idl.tables import build_types

REPO = Path(__file__).resolve().parents[3]
DDS = REPO / "src" / "dds"


def _rows(prefix, *rest):
    """The table's notation: the prefix is written out on the first enumerator only."""
    return [f"{prefix}_UNKNOWN"] + [f"{prefix}_{x}" for x in rest]


ENUMS = {
    # Task 2 (generated from joint_sets.yaml).
    "tcnart_msgs::msg::JointSet": _rows("JOINT_SET", "COCO17", "HALPE26", "TCN_LITE", "TCN_BODY", "SMPL_24",
                                        "SMPLX_55", "SMPLH_52"),
    # Task 3 (HumanPose.idl, PersonTracking.idl).
    "tcnart_msgs::msg::PersonState": _rows("PERSON_STATE", "TRACKED", "COASTING", "LOST"),
    "tcnart_msgs::msg::PersonTrackState": _rows("PERSON_TRACK_STATE", "TENTATIVE", "CONFIRMED", "PREDICTED"),
    "tcnart_msgs::msg::JointSource": _rows("JOINT_SOURCE", "OBSERVED", "DERIVED", "DEFAULTED"),
    "tcnart_msgs::msg::SmplModel": _rows("SMPL_MODEL", "SMPL", "SMPLH", "SMPLX"),
    "tcnart_msgs::msg::SmplGender": _rows("SMPL_GENDER", "NEUTRAL", "FEMALE", "MALE"),
    "tcnart_msgs::msg::PersonSmoothing": _rows("PERSON_SMOOTHING", "OFF", "ONE_EURO"),
    # Task 4 (MediaTypes.idl): StreamKind replaces SensorType, VideoCodec replaces VideoImageCompression.
    "tcnart_msgs::msg::StreamKind": _rows("STREAM_KIND", "VIDEO", "TRACKER", "PERSONS", "MESH", "SURFELS", "EYE",
                                          "HAND", "IMU"),
    "tcnart_msgs::msg::PixelSemantics": _rows("PIXEL_SEMANTICS", "COLOR", "DEPTH", "IR", "MASK"),
    "tcnart_msgs::msg::VideoCodec": _rows("VIDEO_CODEC", "RAW", "PNG", "JPEG", "JPEG_LS", "H264", "H265",
                                          "H264_LOSSLESS", "ZDEPTH", "NDQ"),
    "tcnart_msgs::msg::VideoPixelFormat": _rows("VIDEO_PIXEL_FORMAT", "NV12", "RGB24", "BGR24", "BGRA32", "L8", "L16",
                                                "RGBA32", "DEPTH_F32"),
    "tcnart_msgs::msg::MeshEncoding": _rows("MESH_ENCODING", "BITSTREAM_V1", "STREAM_V2"),
    # Task 5 (Parameters.idl, ServiceController.idl): STRUCT deleted, ENUM appended (UINT32 8 -> 7, UINT64 9 -> 8).
    "pcpd_msgs::rpc::ParameterValueType": _rows("PARAMETER_VALUE_TYPE", "STRING", "INT32", "INT64", "FLOAT",
                                                "DOUBLE", "BOOL", "UINT32", "UINT64", "ENUM"),
    "pcpd_msgs::rpc::StreamRole": _rows("STREAM_ROLE", "DATA", "DESCRIPTOR", "STATE"),
    "pcpd_msgs::rpc::StreamMode": _rows("STREAM_MODE", "STREAM", "LATCHED"),
}

# Replaced enums that must be gone (a mirror keeping them decodes old ordinals as new values).
DELETED = (
    "tcnart_msgs::msg::SensorType",
    "tcnart_msgs::msg::VideoImageCompression",
    # Task 5 (#38 A3): CameraSensor carries tcn_types CameraIntrinsics.
    "tcnart_msgs::msg::CameraModelType",
    "tcnart_msgs::msg::CameraModel",
)


class StreamsEnumsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.decls = load_schema(DDS).decls()

    def test_enumerators_exact(self):
        for name, want in ENUMS.items():
            with self.subTest(name):
                e = self.decls.get(name)
                self.assertIsInstance(e, Enum, f"{name} is missing")
                self.assertEqual(e.enumerators, [(n, i) for i, n in enumerate(want)])

    def test_deleted(self):
        for name in DELETED:
            with self.subTest(name):
                self.assertNotIn(name, self.decls)

    def test_appendable(self):
        for name in ENUMS:
            with self.subTest(name):
                self.assertEqual(self.decls[name].extensibility, "appendable")


class Task5TablesTest(unittest.TestCase):
    """Task 5 in types.json: the parameter union's cases, the deleted CameraModel, the reshaped members."""

    @classmethod
    def setUpClass(cls):
        cls.types = {t["name"]: t for t in build_types(load_schema(DDS), "0")["types"]}

    def _members(self, name):
        return [(m["name"], m["type"], m["optional"]) for m in self.types[name]["members"]]

    def test_parameter_value_cases(self):
        # One case per non-UNKNOWN enumerator, in order; no STRUCT case (T13).
        u = self.types["pcpd_msgs::rpc::ParameterValue"]
        labels = [l for c in u["cases"] for l in c["labels"]]
        self.assertEqual(labels, ENUMS["pcpd_msgs::rpc::ParameterValueType"][1:])
        self.assertEqual((u["cases"][-1]["member"]["name"], u["cases"][-1]["member"]["type"]),
                         ("enum_value", "pcpd_msgs::rpc::EnumEntry"))

    def test_camera_model_gone(self):
        self.assertEqual([n for n in self.types if "CameraModel" in n], [])

    def test_camera_sensor(self):
        m = {n: (t, o) for n, t, o in self._members("pcpd_msgs::msg::CameraSensor")}
        self.assertEqual(m["depth_parameters"], ("tcn_types::msg::CameraIntrinsics", True))
        self.assertEqual(m["color_parameters"], ("tcn_types::msg::CameraIntrinsics", True))
        self.assertEqual(m["frame_rate"], ("double", False))
        self.assertEqual(self._members("pcpd_msgs::msg::CameraSensor")[-3:], [
            ("revision", "uint32", True),
            ("provenance", "tcn_types::msg::CalibrationProvenance", True),
            ("coord", "tcn_types::msg::CoordinateSystem", False)])
        dc = {n: t for n, t, _ in self._members("pcpd_msgs::rpc::DeviceContextReply")}
        self.assertEqual(dc["frame_rate"], "double")

    def test_pcpd_rpc(self):
        sa = {n: t for n, t, _ in self._members("pcpd_msgs::rpc::StreamAnnouncement")}
        self.assertEqual((sa["role"], sa["mode"]), ("pcpd_msgs::rpc::StreamRole", "pcpd_msgs::rpc::StreamMode"))
        pi = {n: t for n, t, _ in self._members("pcpd_msgs::rpc::ProcessInfo")}
        self.assertEqual(pi["started_at"], "builtin_interfaces::msg::Time")

    def test_presence(self):
        self.assertEqual(self._members("tcnart_msgs::msg::TcnartPresenceMessage"), [
            ("header", "std_msgs::msg::Header", False),
            ("rpc_root", "string", False),
            ("heart_beat_counter", "uint64", False),
            ("services", "sequence<tcnart_msgs::msg::ServiceInfo>", False)])
        self.assertEqual([n for n, _, _ in self._members("tcnart_msgs::msg::ServiceInfo")],
                         ["service_name", "topic_prefix", "service_type"])


if __name__ == "__main__":
    unittest.main()
