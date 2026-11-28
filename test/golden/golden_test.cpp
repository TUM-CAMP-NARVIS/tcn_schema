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

#include "std_msgs/msg/Header.h"
#include "std_msgs/msg/HeaderCdrAux.hpp"
#include "tcn_types/msg/Frames.h"
#include "tcn_types/msg/FramesCdrAux.hpp"
#include "tcnart_msgs/msg/PoseTracking.h"
#include "tcnart_msgs/msg/PoseTrackingCdrAux.hpp"
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

int main()
{
    golden("std_msgs_Header", header(), Encoding::XCDR1);

    tcnart_msgs::msg::TargetTrackingMessage tt;
    tt.header(header());
    tcn_types::msg::Target target;
    target.target_id(7);
    target.pose().translation().x(1);
    target.pose().translation().y(2);
    target.pose().translation().z(3);
    target.pose().rotation().w(1);
    target.is_tracked(true);
    tt.targets().push_back(target);
    // a second target with a stated covariance: upper[k] = k
    tcn_types::msg::Target stated = target;
    stated.target_id(8);
    tcn_types::msg::SRCovariance6 cov;
    for (size_t k = 0; k < 21; ++k) cov.upper()[k] = static_cast<double>(k);
    stated.pose_covariance(cov);
    tt.targets().push_back(stated);
    golden("tcnart_msgs_TargetTrackingMessage", tt, Encoding::XCDR2);

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
