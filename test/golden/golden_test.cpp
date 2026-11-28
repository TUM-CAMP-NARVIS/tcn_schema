// SPDX-License-Identifier: internal
//
// Golden byte files: fixed values, encoded, compared byte for byte with test/golden/<case>.hex (space-separated
// lower-case hex). The files are the contract every non-fastcdr codec (Rust, TypeScript, C#, Python) tests against.
//
// TCN_SCHEMA_UPDATE_GOLDEN=1 rewrites the files instead of comparing. Do that only for a deliberate wire change, and
// review the diff: the encodings change exactly when the decisions say (0.11 plan 4: tcn types move to XCDR2).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cdr_util.h"

#include "pcpd_msgs/rpc/Parameters.h"
#include "pcpd_msgs/rpc/ParametersCdrAux.hpp"
#include "std_msgs/msg/Header.h"
#include "std_msgs/msg/HeaderCdrAux.hpp"
#include "tcn_types/msg/Frames.h"
#include "tcn_types/msg/FramesCdrAux.hpp"
#include "tcn_types/msg/Stream.h"
#include "tcn_types/msg/StreamCdrAux.hpp"
#include "tcnart_msgs/msg/HumanPose.h"
#include "tcnart_msgs/msg/HumanPoseCdrAux.hpp"
#include "tcnart_msgs/msg/PoseTracking.h"
#include "tcnart_msgs/msg/PoseTrackingCdrAux.hpp"
#include "tcnart_msgs/msg/StreamDescriptor.h"
#include "tcnart_msgs/msg/StreamDescriptorCdrAux.hpp"
#include "tcnart_msgs/msg/VideoStream.h"
#include "tcnart_msgs/msg/VideoStreamCdrAux.hpp"
#include "sqe_msgs/msg/Graph.h"
#include "sqe_msgs/msg/GraphCdrAux.hpp"
#include "sqe_msgs/rpc/Mutations.h"
#include "sqe_msgs/rpc/MutationsCdrAux.hpp"
#include "sqe_msgs/rpc/Streams.h"
#include "sqe_msgs/rpc/StreamsCdrAux.hpp"
#include "evo_v1/msg/EvolutionV1.h"
#include "evo_v1/msg/EvolutionV1CdrAux.hpp"
#include "evo_v2/msg/EvolutionV2.h"
#include "evo_v2/msg/EvolutionV2CdrAux.hpp"

#ifndef TCN_GOLDEN_DIR
#error "TCN_GOLDEN_DIR must name the directory of the .hex files"
#endif

static std::string to_hex(const std::vector<char>& b)
{
    std::string s;
    char buf[4];
    for (size_t i = 0; i < b.size(); ++i) {
        std::snprintf(buf, sizeof buf, "%02x", static_cast<unsigned char>(b[i]));
        if (i) s += ' ';
        s += buf;
    }
    return s;
}

using tcn_test::Encoding;

static std::vector<char> from_hex(const std::string& hex)
{
    std::vector<char> b;
    std::istringstream in(hex);
    unsigned x;
    while (in >> std::hex >> x) b.push_back(static_cast<char>(x));
    return b;
}

static std::vector<char> golden_bytes(const char* name)
{
    std::ifstream in(std::string(TCN_GOLDEN_DIR) + "/" + name + ".hex");
    std::stringstream s;
    s << in.rdbuf();
    return from_hex(s.str());
}

// Encodes `value` with `e` and compares with test/golden/<name>.hex; returns the bytes.
template <typename T>
static std::vector<char> golden(const char* name, const T& value, Encoding e)
{
    std::vector<char> bytes;
    CHECK(tcn_test::encode(value, bytes, e));
    CHECK(bytes.size() > 1 && bytes[1] == tcn_test::encapsulation_id(e));
    // Only ROS 2 types (std_msgs here) are XCDR1; every other golden must prove XCDR2 (decisions E1, E2).
    CHECK((std::string(name).rfind("std_msgs_", 0) == 0) == (e == Encoding::XCDR1));
    const std::string path = std::string(TCN_GOLDEN_DIR) + "/" + name + ".hex";
    const std::string hex = to_hex(bytes);
    const char* update = std::getenv("TCN_SCHEMA_UPDATE_GOLDEN");
    if (update && std::string(update) == "1") {
        std::ofstream(path) << hex << "\n";
        std::printf("updated %s\n", path.c_str());
    } else {
        std::ifstream in(path);
        std::stringstream want;
        want << in.rdbuf();
        std::string w = want.str();
        while (!w.empty() && (w.back() == '\n' || w.back() == ' ')) w.pop_back();
        if (w != hex) std::fprintf(stderr, "%s:\n  want %s\n  got  %s\n", name, w.c_str(), hex.c_str());
        CHECK(w == hex);
    }
    T back{};
    CHECK(tcn_test::decode(bytes, back));
    CHECK(back == value);
    return bytes;
}

// Type evolution across the two versions of evo::Thing (decisions E5, E8), against the golden bytes.
static void evolution()
{
    evo_v1::msg::Thing v1;
    v1.a(5);
    v1.b("ab");
    golden("evo_v1_Thing", v1, Encoding::XCDR2);

    evo_v2::msg::Thing v2;
    v2.a(7);
    v2.b("hi");
    v2.c(1.5);
    v2.d(0x1122334455667788LL);
    golden("evo_v2_Thing", v2, Encoding::XCDR2);

    // Newer bytes, older reader: the appended members are skipped by the DHEADER.
    evo_v1::msg::Thing old_reader{};
    CHECK(tcn_test::decode(golden_bytes("evo_v2_Thing"), old_reader));
    CHECK(old_reader.a() == 7);
    CHECK(old_reader.b() == "hi");

    // Older bytes, newer reader decoding into a fresh object: c absent, d = 0.
    evo_v2::msg::Thing new_reader{};
    CHECK(tcn_test::decode(golden_bytes("evo_v1_Thing"), new_reader));
    CHECK(new_reader.a() == 5);
    CHECK(new_reader.b() == "ab");
    CHECK(!new_reader.c().has_value());
    CHECK(new_reader.d() == 0);

    // Grown structs inside a container, followed by more data: the old reader must skip by every DHEADER.
    evo_v2::msg::Holder h2;
    evo_v2::msg::Thing t2b = v2;
    t2b.a(8);
    t2b.b("yo");
    h2.things({v2, t2b});
    h2.tail(-1);
    golden("evo_v2_Holder", h2, Encoding::XCDR2);
    evo_v1::msg::Holder old_holder{};
    const auto bytes2 = golden_bytes("evo_v2_Holder");
    CHECK(tcn_test::decode(bytes2, old_holder));
    CHECK(tcn_test::consumed(bytes2, old_holder) == bytes2.size());
    CHECK(old_holder.things().size() == 2);
    CHECK(old_holder.things().size() == 2 && old_holder.things()[1].a() == 8 && old_holder.things()[1].b() == "yo");
    CHECK(old_holder.tail() == -1);

    evo_v1::msg::Holder h1;
    h1.things({v1, v1});
    h1.tail(42);
    golden("evo_v1_Holder", h1, Encoding::XCDR2);
    evo_v2::msg::Holder new_holder{};
    const auto bytes1 = golden_bytes("evo_v1_Holder");
    CHECK(tcn_test::decode(bytes1, new_holder));
    CHECK(new_holder.things().size() == 2 && !new_holder.things()[1].c().has_value() && new_holder.things()[1].d() == 0);
    CHECK(new_holder.tail() == 42);

    // A union case the reader does not know (Review Focus 4, E6): V2's KIND_B is skipped by the union's DHEADER and
    // the member after the union still decodes, with every byte consumed.
    evo_v2::msg::Boxed b2;
    b2.choice().b("yo");
    b2.tail(-1);
    golden("evo_v2_Boxed", b2, Encoding::XCDR2);
    evo_v1::msg::Boxed old_boxed{};
    const auto boxed2 = golden_bytes("evo_v2_Boxed");
    CHECK(tcn_test::decode(boxed2, old_boxed));
    CHECK(tcn_test::consumed(boxed2, old_boxed) == boxed2.size());
    CHECK(old_boxed.tail() == -1);

    evo_v1::msg::Boxed b1;
    b1.choice().a(5);
    b1.tail(42);
    golden("evo_v1_Boxed", b1, Encoding::XCDR2);
    evo_v2::msg::Boxed new_boxed{};
    const auto boxed1 = golden_bytes("evo_v1_Boxed");
    CHECK(tcn_test::decode(boxed1, new_boxed));
    CHECK(tcn_test::consumed(boxed1, new_boxed) == boxed1.size());
    CHECK(new_boxed.choice()._d() == evo_v2::msg::Kind::KIND_A && new_boxed.choice().a() == 5);
    CHECK(new_boxed.tail() == 42);
}

static std_msgs::msg::Header header()
{
    std_msgs::msg::Header h;
    h.stamp().sec(1);
    h.stamp().nanosec(2);
    h.frame_id("map");
    return h;
}

static sqe_msgs::rpc::SISReplyHeader reply_header(const char* engine_id, uint64_t engine_epoch)
{
    sqe_msgs::rpc::SISReplyHeader h;
    h.status(sqe_msgs::rpc::SISStatus::SIS_STATUS_OK);
    h.reason(sqe_msgs::rpc::SISReason::SIS_REASON_UNKNOWN);
    h.detail("");
    h.engine_id(engine_id);
    h.engine_epoch(engine_epoch);
    return h;
}

static tcn_types::msg::SRFrameRef frame_ref(tcn_types::msg::SRNodeScope scope, const char* owner, const char* name,
                                            tcn_types::msg::SRFrameSelector selector =
                                                    tcn_types::msg::SRFrameSelector::SR_FRAME_SELECTOR_FRAME)
{
    tcn_types::msg::SRFrameRef r;
    r.scope(scope);
    r.owner(owner);
    r.name(name);
    r.selector(selector);
    return r;
}

// The reply header, the stream terms and the start reply (#35 R1-R4, H1-H2, S3).
static void sqe_replies()
{
    using tcn_types::msg::SRFrameSelector;
    using tcn_types::msg::SRNodeScope;
    namespace rpc = sqe_msgs::rpc;

    // By hand: outer DHEADER 0x24, header DHEADER 0x20; status OK = 1, reason UNKNOWN = 0; "" as length 1 + NUL +
    // pad; "e" as length 2 + bytes + pad; engine_epoch 7 as uint64 at a 4-aligned offset (XCDR2 max alignment 4).
    rpc::SISMutationReply mu;
    mu.header(reply_header("e", 7));
    golden("sqe_msgs_SISMutationReply", mu, Encoding::XCDR2);

    // local:app/hmd -> domain:rig01/cam0[instances], FIXED_RATE 60 Hz, bias 0.25, BEST_POSSIBLE, ENGINE, BEST_PATH.
    rpc::SISRelationStreamStartRequest start;
    rpc::SISRelationTerms& terms = start.terms();
    terms.observer(frame_ref(SRNodeScope::SR_NODE_SCOPE_LOCAL, "app", "hmd"));
    terms.target(frame_ref(SRNodeScope::SR_NODE_SCOPE_DOMAIN, "rig01", "cam0",
                           SRFrameSelector::SR_FRAME_SELECTOR_INSTANCES));
    terms.trigger().hz(60.0);
    terms.freshness_bias(0.25);
    terms.reoptimise(rpc::SISReoptimisePolicy::SIS_REOPTIMISE_POLICY_BEST_POSSIBLE);
    terms.evaluation(rpc::SISEvaluationMode::SIS_EVALUATION_MODE_ENGINE);
    terms.fusion(rpc::SISFusionPolicy::SIS_FUSION_POLICY_BEST_PATH);
    golden("sqe_msgs_SISRelationStreamStartRequest", start, Encoding::XCDR2);

    // A PENDING start: handle present (01), park reason present (01), one blocking edge, plan absent (00).
    rpc::SISRelationStreamReply reply;
    reply.header(reply_header("sqe-a", 7));
    rpc::SISStreamHandle handle;
    handle.handle("h1");
    handle.data_topic("sqe/sqe-a/rel/h1");
    handle.status_topic("sqe/sqe-a/rel/h1/status");
    handle.plan_topic("sqe/sqe-a/rel/h1/plan");
    reply.handle(handle);
    reply.relation().state(rpc::SISRelationState::SIS_RELATION_STATE_PENDING);
    reply.relation().park_reason(rpc::SISParkReason::SIS_PARK_REASON_UNCALIBRATED);
    reply.relation().detail("");
    rpc::SISBlockingRelation blocking;
    blocking.edge().source(frame_ref(SRNodeScope::SR_NODE_SCOPE_GLOBAL, "", "world"));
    blocking.edge().target(frame_ref(SRNodeScope::SR_NODE_SCOPE_LOCAL, "app", "hmd"));
    blocking.edge().name("e1");
    blocking.kind(sqe_msgs::msg::SREdgeKind::SR_EDGE_KIND_POSE);
    blocking.timing(sqe_msgs::msg::SREdgeTiming::SR_EDGE_TIMING_STATIC_UNCALIBRATED);
    reply.relation().blocked_by().push_back(blocking);
    golden("sqe_msgs_SISRelationStreamReply", reply, Encoding::XCDR2);
}

// {capture_time_ns 1, sequence_number 2, frame global:w}; device_time_ns, group_id and clock absent.
static tcn_types::msg::StreamHeader stream_header()
{
    tcn_types::msg::StreamHeader h;
    h.capture_time_ns(1);
    h.sequence_number(2);
    h.frame().scope(tcn_types::msg::SRNodeScope::SR_NODE_SCOPE_GLOBAL);
    h.frame().owner("");
    h.frame().name("w");
    h.frame().selector(tcn_types::msg::SRFrameSelector::SR_FRAME_SELECTOR_FRAME);
    return h;
}

// The stream header and two stream messages with optional members absent and present (#38 A1, A5; #37 T8, T19).
static void streams()
{
    // By hand: DHEADER 0x34; int64 1 at a 4-aligned offset; device_time_ns absent (00 + 3 pad); uint64 2;
    // group_id and clock absent (00 00 + 2 pad); SRFrameRef DHEADER 0x18, GLOBAL = 2, "" and "w", FRAME = 1.
    golden("tcn_types_StreamHeader", stream_header(), Encoding::XCDR2);

    // Target 1 tracked with a pose (presence 01), no covariance, no stamp; target 2 untracked with no pose
    // (presence 00, never identity) and its own capture_time_ns 5; target 3 as target 1 with a covariance.
    tcnart_msgs::msg::TargetTrackingMessage tt;
    tt.header(stream_header());
    tcn_types::msg::Target tracked;
    tracked.target_id(1);
    tcn_types::msg::RigidTransform pose;
    pose.translation().x(1);
    pose.translation().y(2);
    pose.translation().z(3);
    pose.rotation().w(1);
    tracked.pose(pose);
    tracked.is_tracked(true);
    tt.targets().push_back(tracked);
    tcn_types::msg::Target lost;
    lost.target_id(2);
    lost.is_tracked(false);
    lost.capture_time_ns(5);
    tt.targets().push_back(lost);
    // Target 3: tracked, pose present, a stated covariance (presence 01, SRCovariance6 DHEADER, upper[k] = k).
    tcn_types::msg::Target stated = tracked;
    stated.target_id(3);
    tcn_types::msg::SRCovariance6 cov;
    for (size_t k = 0; k < 21; ++k) cov.upper()[k] = static_cast<double>(k);
    stated.pose_covariance(cov);
    tt.targets().push_back(stated);
    tt.status_flags(tcnart_msgs::msg::TRACKER_STATUS_RUNNING);
    golden("tcnart_msgs_TargetTrackingMessage", tt, Encoding::XCDR2);

    // Per-frame intrinsics present (presence 01, then the union): pinhole 500, 500, 320, 240, NONE, no coefficients.
    tcnart_msgs::msg::VideoStreamMessage video;
    video.header(stream_header());
    video.pose().rotation().w(1);
    tcn_types::msg::PinholeIntrinsics pin;
    pin.fx(500);
    pin.fy(500);
    pin.cx(320);
    pin.cy(240);
    pin.distortion_model(tcn_types::msg::SRDistortionModel::SR_DISTORTION_MODEL_NONE);
    tcn_types::msg::CameraIntrinsics intr;
    intr.pinhole(pin);
    video.intrinsics(intr);
    const std::vector<uint8_t> image{0xde, 0xad, 0xbe, 0xef};
    video.image_bytes(image.size());
    video.image(image);
    golden("tcnart_msgs_VideoStreamMessage", video, Encoding::XCDR2);

    // The same frame with the intrinsics absent (the descriptor's nominal ones): the image after them survives.
    // Presence 00 and no union bytes: image_bytes follows the pose directly.
    tcnart_msgs::msg::VideoStreamMessage nominal = video;
    nominal.intrinsics().reset();
    const std::vector<char> bytes = golden("tcnart_msgs_VideoStreamMessage_nominal", nominal, Encoding::XCDR2);
    tcnart_msgs::msg::VideoStreamMessage back;
    CHECK(tcn_test::decode(bytes, back));
    CHECK(!back.intrinsics().has_value());
    CHECK(back.image() == image && back.image_bytes() == 4);
    CHECK(back == nominal);

    // The shared person stream (decisions §7 HumanPose): header {1, -, 1, group 7, -, domain rig:world}, TCN_LITE v1,
    // person 3 (member absent, TRACKED, confidence 1) with two joints: the first with a rotation (presence 01), the
    // second without (presence 00, never identity); smpl absent (presence 00).
    namespace tm = tcnart_msgs::msg;
    tm::HumanPoseMessage hp;
    hp.header().capture_time_ns(1);
    hp.header().sequence_number(1);
    hp.header().group_id(7);
    hp.header().frame().scope(tcn_types::msg::SRNodeScope::SR_NODE_SCOPE_DOMAIN);
    hp.header().frame().owner("rig");
    hp.header().frame().name("world");
    hp.header().frame().selector(tcn_types::msg::SRFrameSelector::SR_FRAME_SELECTOR_FRAME);
    hp.joint_set(tm::JointSet::JOINT_SET_TCN_LITE);
    hp.joint_set_version(1);
    tm::HumanPose person;
    person.person(3);
    person.state(tm::PersonState::PERSON_STATE_TRACKED);
    person.confidence(1.0f);
    tm::HumanJoint head;
    head.position().y(1.5);
    head.confidence(1.0f);
    head.source(tm::JointSource::JOINT_SOURCE_OBSERVED);
    tcn_types::msg::Quaternion upright;
    upright.w(1);
    head.rotation(upright);
    person.joints().push_back(head);
    tm::HumanJoint chest;
    chest.position().y(0.5);
    chest.confidence(0.5f);
    chest.source(tm::JointSource::JOINT_SOURCE_DERIVED);
    person.joints().push_back(chest);
    hp.persons().push_back(person);
    const std::vector<char> hp_bytes = golden("tcnart_msgs_HumanPoseMessage", hp, Encoding::XCDR2);
    tm::HumanPoseMessage hp_back;
    CHECK(tcn_test::decode(hp_bytes, hp_back));
    CHECK(hp_back.header().group_id().has_value() && *hp_back.header().group_id() == 7);
    CHECK(!hp_back.header().clock().has_value() && !hp_back.header().device_time_ns().has_value());
    CHECK(hp_back.persons().size() == 1 && !hp_back.persons()[0].member().has_value());
    CHECK(hp_back.persons()[0].joints()[0].rotation().has_value());
    CHECK(!hp_back.persons()[0].joints()[1].rotation().has_value());
    CHECK(!hp_back.persons()[0].smpl().has_value());
}

// The typed stream descriptor (#35 S1/N6, #38 A2/A7, #37 T7/T8/T10).
static void descriptor()
{
    namespace tm = tcnart_msgs::msg;

    // The tracker case, by hand: union DHEADER 0x25; discriminator TRACKER = 2; TrackerStreamInfo DHEADER 0x1d;
    // ARUCO_MARKER = 1; "a" (length 2 + pad); "" (length 1 + pad); targets: sequence DHEADER 4 (non-primitive
    // elements) + length 0; nominal_covariance absent (presence 00, last byte, no trailing pad).
    tm::TrackerStreamInfo tracker;
    tracker.marker_type(tcn_types::msg::MarkerType::MARKER_TYPE_ARUCO_MARKER);
    tracker.marker_system("a");
    tracker.marker_domain("");
    tm::StreamInfo info;
    info.tracker(tracker);
    const std::vector<char> info_bytes = golden("tcnart_msgs_StreamInfo", info, Encoding::XCDR2);
    tm::StreamInfo info_back;
    CHECK(tcn_test::decode(info_bytes, info_back));
    CHECK(info_back._d() == tm::StreamKind::STREAM_KIND_TRACKER);
    CHECK(!info_back.tracker().nominal_covariance().has_value());

    // The video case: frame domain:rig/cam1_color, clock {"", HOST}, 30 Hz, revision 2 (presence 01);
    // H265 + COLOR + NV12, nominal intrinsics present (presence 01), codec_data 00 00 00 01, no properties.
    tm::StreamDescriptorMessage d;
    d.stream_topic("rig/str/cam1_color");
    d.stream_topic_schema("tcnart_msgs::msg::VideoStreamMessage");
    d.revision(2);
    d.frame(frame_ref(tcn_types::msg::SRNodeScope::SR_NODE_SCOPE_DOMAIN, "rig", "cam1_color"));
    d.clock().id("");
    d.clock().kind(tcn_types::msg::ClockKind::CLOCK_KIND_HOST);
    d.frame_rate(30.0);
    d.sensor_latency_ns(0);
    d.calib_topic("");
    tm::VideoStreamInfo video;
    video.width(640);
    video.height(480);
    video.step(640);
    video.pixel_format(tm::VideoPixelFormat::VIDEO_PIXEL_FORMAT_NV12);
    video.semantics(tm::PixelSemantics::PIXEL_SEMANTICS_COLOR);
    video.codec(tm::VideoCodec::VIDEO_CODEC_H265);
    video.h26x_profile(tm::VideoH26xProfile::VIDEO_H26X_PROFILE_H265_MAIN);
    video.bitrate(4000000);
    tcn_types::msg::PinholeIntrinsics pin;
    pin.fx(500);
    pin.fy(500);
    pin.cx(320);
    pin.cy(240);
    pin.distortion_model(tcn_types::msg::SRDistortionModel::SR_DISTORTION_MODEL_NONE);
    tcn_types::msg::CameraIntrinsics intr;
    intr.pinhole(pin);
    video.intrinsics(intr);
    video.codec_data({0x00, 0x00, 0x00, 0x01});
    d.info().video(video);
    const std::vector<char> d_bytes = golden("tcnart_msgs_StreamDescriptorMessage", d, Encoding::XCDR2);
    tm::StreamDescriptorMessage d_back;
    CHECK(tcn_test::decode(d_bytes, d_back));
    CHECK(d_back.revision().has_value() && *d_back.revision() == 2);
    CHECK(d_back.info()._d() == tm::StreamKind::STREAM_KIND_VIDEO);
    CHECK(!d_back.info().video().depth_units_per_meter().has_value());
    CHECK(d_back.info().video().intrinsics().has_value());
    CHECK(d_back.info().video().codec_data().size() == 4 && d_back.properties().empty());
    CHECK(d_back.info().video().bitrate().has_value() && *d_back.info().video().bitrate() == 4000000);

    // An unversioned tracker descriptor (#38 A7, P3): revision absent (presence 00, no uint32), "t", "", frame
    // global:w, clock {"", UNKNOWN}, irregular (0 Hz), the tracker case above. Absent is "compare the content",
    // never revision 0: decoding into an object that held a revision must leave it absent.
    tm::StreamDescriptorMessage u;
    u.stream_topic("t");
    u.stream_topic_schema("");
    u.frame(frame_ref(tcn_types::msg::SRNodeScope::SR_NODE_SCOPE_GLOBAL, "", "w"));
    u.clock().id("");
    u.frame_rate(0.0);
    u.sensor_latency_ns(0);
    u.calib_topic("");
    u.info().tracker(tracker);
    const std::vector<char> u_bytes = golden("tcnart_msgs_StreamDescriptorMessage_unversioned", u, Encoding::XCDR2);
    tm::StreamDescriptorMessage u_back;
    u_back.revision(7);
    CHECK(tcn_test::decode(u_bytes, u_back));
    CHECK(!u_back.revision().has_value());
    CHECK(u_back.info()._d() == tm::StreamKind::STREAM_KIND_TRACKER && u_back.info().tracker().marker_system() == "a");
    tm::StreamDescriptorMessage u_fresh;
    CHECK(tcn_test::decode(u_bytes, u_fresh) && !u_fresh.revision().has_value());
}

// The parameter union's ENUM case (0.11 plan 6 task 5, #37 T13), by hand: Parameter DHEADER 0x2c; "mode" (length 5
// + pad 3); ParameterValue DHEADER 0x1c; discriminator PARAMETER_VALUE_TYPE_ENUM = 9 (STRUCT deleted, so UINT32 = 7,
// UINT64 = 8); EnumEntry DHEADER 0x14; "auto" (length 5 + pad 3); int64 2 (XCDR2 aligns it to 4).
static void parameters()
{
    namespace pr = pcpd_msgs::rpc;
    pr::EnumEntry entry;
    entry.label("auto");
    entry.value(2);
    pr::Parameter p;
    p.key("mode");
    p.value().enum_value(entry);
    CHECK(p.value()._d() == pr::ParameterValueType::PARAMETER_VALUE_TYPE_ENUM);
    CHECK(static_cast<int>(pr::ParameterValueType::PARAMETER_VALUE_TYPE_UINT32) == 7);
    CHECK(static_cast<int>(pr::ParameterValueType::PARAMETER_VALUE_TYPE_UINT64) == 8);
    const std::vector<char> bytes = golden("pcpd_msgs_Parameter", p, Encoding::XCDR2);
    pr::Parameter back;
    CHECK(tcn_test::decode(bytes, back));
    CHECK(back.value()._d() == pr::ParameterValueType::PARAMETER_VALUE_TYPE_ENUM);
    CHECK(back.value().enum_value().label() == "auto" && back.value().enum_value().value() == 2);
}

int main()
{
    golden("std_msgs_Header", header(), Encoding::XCDR1);

    streams();

    descriptor();

    parameters();

    // The XCDR2 rules by hand: DHEADER 0x18; enum int32 LOCAL=1; "a" and "b" as length 2 + bytes + pad; enum FRAME=1.
    tcn_types::msg::SRFrameRef ref;
    ref.scope(tcn_types::msg::SRNodeScope::SR_NODE_SCOPE_LOCAL);
    ref.owner("a");
    ref.name("b");
    ref.selector(tcn_types::msg::SRFrameSelector::SR_FRAME_SELECTOR_FRAME);
    golden("tcn_types_SRFrameRef", ref, Encoding::XCDR2);

    // The graph: a LOCAL and a DOMAIN node, an uncalibrated edge with no transform, a projection
    // edge with pinhole intrinsics (metric_radius absent, image size present) and a pose edge
    // naming entry 0. The presence bytes are reviewed by hand in the task report.
    using tcn_types::msg::SRFrameSelector;
    using tcn_types::msg::SRNodeScope;
    auto frame = [](SRNodeScope scope, const char* owner, const char* name) {
        tcn_types::msg::SRFrameRef r;
        r.scope(scope);
        r.owner(owner);
        r.name(name);
        r.selector(SRFrameSelector::SR_FRAME_SELECTOR_FRAME);
        return r;
    };

    sqe_msgs::msg::SRGraph g;
    g.origin("a");
    sqe_msgs::msg::SRNode node;
    node.name("a");
    node.scope(SRNodeScope::SR_NODE_SCOPE_LOCAL);
    g.nodes().push_back(node);
    sqe_msgs::msg::SRNode dnode;
    dnode.name("cam");
    dnode.scope(SRNodeScope::SR_NODE_SCOPE_DOMAIN);
    dnode.domain("rig01");
    g.nodes().push_back(dnode);

    sqe_msgs::msg::SREdge uncal;  // transform stays absent
    uncal.name("e1");
    uncal.source(frame(SRNodeScope::SR_NODE_SCOPE_LOCAL, "f", "a"));
    uncal.target(frame(SRNodeScope::SR_NODE_SCOPE_DOMAIN, "rig01", "cam"));
    uncal.kind(sqe_msgs::msg::SREdgeKind::SR_EDGE_KIND_POSE);
    uncal.timing(sqe_msgs::msg::SREdgeTiming::SR_EDGE_TIMING_STATIC_UNCALIBRATED);
    g.edges().push_back(uncal);

    sqe_msgs::msg::SREdge proj;
    proj.name("e2");
    proj.source(frame(SRNodeScope::SR_NODE_SCOPE_DOMAIN, "rig01", "cam"));
    proj.target(frame(SRNodeScope::SR_NODE_SCOPE_GLOBAL, "", "img"));
    proj.kind(sqe_msgs::msg::SREdgeKind::SR_EDGE_KIND_PROJECTION);
    proj.timing(sqe_msgs::msg::SREdgeTiming::SR_EDGE_TIMING_STATIC);
    tcn_types::msg::PinholeIntrinsics pin;
    pin.fx(600);
    pin.fy(601);
    pin.cx(320);
    pin.cy(240);
    pin.distortion_model(tcn_types::msg::SRDistortionModel::SR_DISTORTION_MODEL_OPENCV_RADTAN);
    pin.distortion({0.1, 0.2, 0.3, 0.4, 0.5});
    pin.image_width(640);
    pin.image_height(480);
    tcn_types::msg::CameraIntrinsics intr;
    intr.pinhole(pin);
    sqe_msgs::msg::SRTransform proj_tf;
    proj_tf.projection(intr);
    proj.transform(proj_tf);
    g.edges().push_back(proj);

    sqe_msgs::msg::SREdge pose;
    pose.name("e3");
    pose.source(frame(SRNodeScope::SR_NODE_SCOPE_GLOBAL, "", "world"));
    pose.target(frame(SRNodeScope::SR_NODE_SCOPE_LOCAL, "f", "a"));
    pose.kind(sqe_msgs::msg::SREdgeKind::SR_EDGE_KIND_POSE);
    pose.timing(sqe_msgs::msg::SREdgeTiming::SR_EDGE_TIMING_DYNAMIC);
    pose.entry_id(0);  // present, and 0
    g.edges().push_back(pose);
    golden("sqe_msgs_SRGraph", g, Encoding::XCDR2);

    sqe_replies();

    evolution();

    return tcn_test_exit("golden");
}
