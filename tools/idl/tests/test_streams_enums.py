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


def _rows(prefix, *rest, first="UNKNOWN"):
    """The table's notation: the prefix is written out on the first enumerator only. The first enumerator is
    `{prefix}_UNKNOWN` unless the table names another (`SESSION_STATE_REASON_NONE`)."""
    return [f"{prefix}_{first}"] + [f"{prefix}_{x}" for x in rest]


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
    # Task 6 (#36): BARGE_IN_MODE_OFF is 1, not 0 as in #36 S6 (E6: UNKNOWN first).
    "conversation_msgs::msg::BargeInMode": _rows("BARGE_IN_MODE", "OFF", "VAD", "SUSTAINED", "WORDS"),
    "conversation_msgs::msg::SessionStateReason": _rows("SESSION_STATE_REASON", "COMPLETED", "BARGE_IN",
                                                        "NON_SPEECH", "NOT_ADDRESSED", "CANCELLED", "ERROR",
                                                        first="NONE"),
    "conversation_msgs::msg::PlaybackState": _rows("PLAYBACK_STATE", "STARTED", "DRAINED", "CLEARED"),
    "conversation_msgs::msg::FaceRig": _rows("FACE_RIG", "ARKIT_52", "CUSTOM"),
}

# Replaced enums that must be gone (a mirror keeping them decodes old ordinals as new values).
DELETED = (
    "tcnart_msgs::msg::SensorType",
    "tcnart_msgs::msg::VideoImageCompression",
    # Task 5 (#38 A3): CameraSensor carries tcn_types CameraIntrinsics.
    "tcnart_msgs::msg::CameraModelType",
    "tcnart_msgs::msg::CameraModel",
    # Task 6 (#36 L1, A1; S2): EventState repeated what the message type says; the two never-published messages.
    "conversation_msgs::msg::EventState",
    "conversation_msgs::msg::SessionConfigUpdate",
    "conversation_msgs::msg::SttRequest",
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
        self.assertEqual(self._members("tcn_types::msg::CalibrationProvenance"), [
            ("tool", "string", False), ("method", "string", False),
            ("residual", "double", True), ("covariance", "tcn_types::msg::SRCovariance6", True),
            ("computed_at_ns", "int64", True), ("samples", "uint32", True)])
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


class Task6TablesTest(unittest.TestCase):
    """Task 6 in types.json: conversation_msgs per #36 as placed (decisions §7)."""

    @classmethod
    def setUpClass(cls):
        cls.types = {t["name"]: t for t in build_types(load_schema(DDS), "0")["types"]}

    def _members(self, name):
        return [(m["name"], m["type"], m["optional"]) for m in self.types[name]["members"]]

    def _names(self, name):
        return [m["name"] for m in self.types[name]["members"]]

    def test_session_start(self):
        # #36 S1 exactly (team review IVA 2): formats, backends and hints come from AppServerInfo only.
        self.assertEqual(self._members("conversation_msgs::msg::SessionStart"), [
            ("header", "conversation_msgs::msg::MessageHeader", False),
            ("identity", "conversation_msgs::msg::ClientIdentity", False),
            ("allow_barge_in", "boolean", False),
            ("enable_partial_stt", "boolean", False),
            ("barge_in_mode", "conversation_msgs::msg::BargeInMode", True)])

    def test_session_state_event(self):
        # S5: the reason comes after the state; status stays for errors.
        self.assertEqual(self._names("conversation_msgs::msg::SessionStateEvent"),
                         ["header", "state", "reason", "status"])
        m = dict((n, t) for n, t, _ in self._members("conversation_msgs::msg::SessionStateEvent"))
        self.assertEqual(m["reason"], "conversation_msgs::msg::SessionStateReason")

    def test_playback_event(self):
        # S4, D3.
        self.assertEqual(self._members("conversation_msgs::msg::PlaybackEvent"), [
            ("header", "conversation_msgs::msg::MessageHeader", False),
            ("state", "conversation_msgs::msg::PlaybackState", False),
            ("played_samples", "unsigned long long", False),
            ("sample_rate_hz", "unsigned long", False)])
        self.assertEqual(self.types["conversation_msgs::msg::PlaybackEvent"]["file"],
                         "conversation_msgs/msg/Control.idl")

    def test_vad_event(self):
        # A1: a position, not start/end; a probability only from a VAD that has one.
        self.assertEqual(self._members("conversation_msgs::msg::VadEvent"), [
            ("header", "conversation_msgs::msg::MessageHeader", False),
            ("speech_active", "boolean", False),
            ("stream_position_ms", "unsigned long long", False),
            ("speech_probability", "float", True),
            ("backend", "string", False)])

    def test_payload_last_and_turn_timeline(self):
        # F1/F2: turn_sample_index just before the payload (A8, payload last).
        self.assertEqual(self._members("conversation_msgs::msg::TtsAudioChunk")[-3:], [
            ("is_final_chunk", "boolean", False),
            ("turn_sample_index", "unsigned long long", False),
            ("samples_f32", "sequence<float>", False)])
        self.assertEqual(self._members("conversation_msgs::msg::AudioOpusFrame")[-3:], [
            ("first_sample_index", "unsigned long long", False),
            ("turn_sample_index", "unsigned long long", False),
            ("packet", "sequence<octet>", False)])
        self.assertEqual(self._names("conversation_msgs::msg::AudioPcmChunk")[-1], "samples_f32")

    def test_no_constant_state(self):
        # L1: the four `state` members are gone.
        for name in ("SttPartial", "SttFinal", "LlmPartial", "LlmFinal"):
            with self.subTest(name):
                self.assertNotIn("state", self._names(f"conversation_msgs::msg::{name}"))

    def test_no_flag_pairs(self):
        # L2 (P3): absence is @optional, never a has_* flag.
        for name, t in self.types.items():
            if t["package"] == "conversation_msgs" and t["kind"] == "struct":
                with self.subTest(name):
                    self.assertEqual([n for n in self._names(name) if n.startswith("has_")], [])

    def test_optionals(self):
        m = {n: (t, o) for n, t, o in self._members("conversation_msgs::msg::SttFinal")}
        self.assertEqual(m["confidence"], ("float", True))
        m = {n: (t, o) for n, t, o in self._members("conversation_msgs::msg::LlmFinal")}
        self.assertEqual(m["prompt_tokens"], ("unsigned long", True))
        self.assertEqual(m["generated_tokens"], ("unsigned long", True))

    def test_interaction_activity_header(self):
        # L4.
        self.assertEqual(self._members("conversation_msgs::msg::InteractionActivity")[0],
                         ("header", "conversation_msgs::msg::MessageHeader", False))

    def test_tts_done(self):
        # T1: after total_chunks.
        self.assertEqual(self._names("conversation_msgs::msg::TtsDone"),
                         ["header", "backend", "voice", "total_chunks", "total_samples", "sample_rate_hz", "status"])

    def test_health_status(self):
        # M1: no escaped IDL keyword.
        self.assertEqual(self._names("conversation_msgs::msg::HealthStatus"),
                         ["header", "component_name", "ready", "status"])

    def test_face_layout(self):
        # F3, D5.
        self.assertEqual(self._members("conversation_msgs::msg::FaceAnimationLayout"), [
            ("layout_id", "string", False),
            ("rig", "conversation_msgs::msg::FaceRig", False),
            ("channel_names", "sequence<string>", False),
            ("emotion_names", "sequence<string>", False),
            ("frame_rate_hz", "float", False)])
        self.assertEqual(self.types["conversation_msgs::msg::FaceAnimationLayout"]["file"],
                         "conversation_msgs/msg/Face.idl")

    def test_app_server_info(self):
        # D1, D4, D5 at the end; language_hint/voice_hint stay as server defaults (team review IVA 2).
        members = self._members("conversation_msgs::rpc::AppServerInfo")
        self.assertEqual(members[-3:], [
            ("capabilities", "sequence<string>", False),
            ("barge_in_mode", "conversation_msgs::msg::BargeInMode", False),
            ("face_layouts", "sequence<conversation_msgs::msg::FaceAnimationLayout>", False)])
        self.assertIn("language_hint", [n for n, _, _ in members])
        self.assertIn("voice_hint", [n for n, _, _ in members])

    def test_appendable(self):
        for name in ("PlaybackEvent", "FaceAnimationLayout", "SessionStart", "VadEvent"):
            with self.subTest(name):
                self.assertEqual(self.types[f"conversation_msgs::msg::{name}"]["extensibility"], "appendable")


if __name__ == "__main__":
    unittest.main()
