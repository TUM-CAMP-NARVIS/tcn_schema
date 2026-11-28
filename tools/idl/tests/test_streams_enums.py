"""0.11 plan 6: the new and changed enums of Global Constraints, exactly (Review Focus 1: a mirror that keeps old
ordinals decodes plausible wrong values). Ordinal = position; every enum is @appendable with UNKNOWN (or NONE) at
0 (E6). Each task adds the rows of the enums it lands."""
import unittest
from pathlib import Path

from tools.idl.model import Enum
from tools.idl.parser import load_schema

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
}

# Replaced enums that must be gone (a mirror keeping them decodes old ordinals as new values).
DELETED = (
    "tcnart_msgs::msg::SensorType",
    "tcnart_msgs::msg::VideoImageCompression",
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


if __name__ == "__main__":
    unittest.main()
