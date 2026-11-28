// SPDX-License-Identifier: internal
//
// Round trips of the shared person stream (tcnart_msgs::msg::HumanPoseMessage, HumanShapeMessage, PersonStreamInfo;
// 0.11 decisions §7 HumanPose) and TimeRefRequest/TimeRefReply (new in 0.10.5) through CDR with the encapsulation
// header, as the Zenoh transport carries them. The TimeRef bytes are the examples of tcnart's
// v3/docs/protocols/time-reference.md.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>

#include "tcnart_msgs/msg/HumanPose.h"
#include "tcnart_msgs/msg/HumanPoseCdrAux.hpp"
#include "tcnart_msgs/msg/TimeRef.h"
#include "tcnart_msgs/msg/TimeRefCdrAux.hpp"

namespace msg = tcnart_msgs::msg;
using eprosima::fastcdr::Cdr;
using eprosima::fastcdr::CdrVersion;
using eprosima::fastcdr::FastBuffer;

static int g_failures = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                                 \
        }                                                                                 \
    } while (0)

template <typename T>
static bool encode(const T& m, std::vector<char>& out)
{
    out.assign(1 << 20, 0);
    FastBuffer fb(out.data(), out.size());
    Cdr ser(fb, Cdr::LITTLE_ENDIANNESS, CdrVersion::XCDRv2);
    try { ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::DELIMIT_CDR2); ser.serialize_encapsulation(); ser << m; } catch (...) { return false; }
    out.resize(ser.get_serialized_data_length());
    return true;
}

template <typename T>
static bool decode(const std::vector<char>& in, T& m)
{
    std::vector<char> copy(in);
    FastBuffer fb(copy.data(), copy.size());
    Cdr de(fb, Cdr::DEFAULT_ENDIAN, CdrVersion::XCDRv2);
    try { de.read_encapsulation(); de >> m; } catch (...) { return false; }
    return true;
}

static std::vector<char> bytes(std::initializer_list<int> b)
{
    std::vector<char> v;
    for (int x : b) v.push_back(static_cast<char>(x));
    return v;
}

int main()
{
    // HumanPoseMessage: two TCN_BODY persons (20 joints), rotations on some joints, SMPL on one person.
    msg::HumanPoseMessage m;
    m.header().capture_time_ns(1700000000500000000LL);
    m.header().sequence_number(3);
    m.header().group_id(4711);  // the PersonTracking group it came from
    m.header().frame().scope(tcn_types::msg::SRNodeScope::SR_NODE_SCOPE_DOMAIN);
    m.header().frame().owner("rig");
    m.header().frame().name("world");
    m.header().frame().selector(tcn_types::msg::SRFrameSelector::SR_FRAME_SELECTOR_FRAME);
    m.joint_set(msg::JointSet::JOINT_SET_TCN_BODY);
    m.joint_set_version(1);
    for (uint32_t id : {7u, 9u}) {
        msg::HumanPose p;
        p.person(id);
        if (id == 7u) p.member(eprosima::fastcdr::fixed_string<128>("alice@hmd-1"));
        p.state(id == 7u ? msg::PersonState::PERSON_STATE_TRACKED : msg::PersonState::PERSON_STATE_COASTING);
        p.confidence(id == 7u ? 0.95f : 0.4f);
        p.source_ids({id + 40, id + 30});
        for (int j = 0; j < 20; ++j) {
            msg::HumanJoint hj;
            hj.position().x(0.1 * j); hj.position().y(1.0); hj.position().z(-0.05 * j);
            hj.confidence(0.9f);
            hj.source(j == 0 ? msg::JointSource::JOINT_SOURCE_DERIVED : msg::JointSource::JOINT_SOURCE_OBSERVED);
            if (j % 3 == 0) {
                tcn_types::msg::Quaternion q;
                q.z(0.6); q.w(0.8);
                hj.rotation(q);
            }
            p.joints().push_back(hj);
        }
        if (id == 7u) {
            msg::SmplParams smpl;
            smpl.model(msg::SmplModel::SMPL_MODEL_SMPLH);
            smpl.global_orient().y(3.14);
            smpl.translation().z(0.9);
            smpl.pose().resize(52);
            smpl.pose()[51].x(0.25);
            p.smpl(smpl);
        }
        m.persons().push_back(p);
    }
    std::vector<char> wire;
    CHECK(encode(m, wire));
    msg::HumanPoseMessage out;
    CHECK(decode(wire, out));
    CHECK(out.header().capture_time_ns() == 1700000000500000000LL && out.header().frame().name() == "world");
    CHECK(out.header().group_id().has_value() && *out.header().group_id() == 4711);
    CHECK(out.joint_set() == msg::JointSet::JOINT_SET_TCN_BODY && out.joint_set_version() == 1);
    CHECK(out.persons().size() == 2);
    const auto& p0 = out.persons()[0];
    CHECK(p0.person() == 7 && p0.member().has_value() && p0.member()->to_string() == "alice@hmd-1");
    CHECK(p0.state() == msg::PersonState::PERSON_STATE_TRACKED && p0.confidence() == 0.95f);
    CHECK(p0.source_ids().size() == 2 && p0.source_ids()[0] == 47);
    CHECK(p0.joints().size() == 20);
    CHECK(p0.joints()[3].rotation().has_value() && p0.joints()[3].rotation()->w() == 0.8);
    CHECK(!p0.joints()[4].rotation().has_value());  // absent, never identity
    CHECK(p0.joints()[0].source() == msg::JointSource::JOINT_SOURCE_DERIVED);
    CHECK(p0.smpl().has_value() && p0.smpl()->model() == msg::SmplModel::SMPL_MODEL_SMPLH);
    CHECK(p0.smpl()->pose().size() == 52 && p0.smpl()->pose()[51].x() == 0.25 && p0.smpl()->expression().empty());
    const auto& p1 = out.persons()[1];
    CHECK(!p1.member().has_value() && p1.state() == msg::PersonState::PERSON_STATE_COASTING && !p1.smpl().has_value());
    CHECK(out == m);

    // HumanShapeMessage (latched): one TCN_BODY person with SMPL betas, one COCO17 person without SMPL.
    msg::HumanShapeMessage shapes;
    shapes.header().capture_time_ns(1700000000000000000LL);
    shapes.header().sequence_number(1);
    msg::HumanShape s7;
    s7.person(7);
    s7.joint_set(msg::JointSet::JOINT_SET_TCN_BODY);
    s7.joint_set_version(1);
    for (int b = 0; b < 19; ++b) s7.bone_lengths().push_back(b < 10 ? 0.2f + 0.01f * b : std::nanf(""));
    s7.smpl_model(msg::SmplModel::SMPL_MODEL_SMPLH);
    s7.smpl_gender(msg::SmplGender::SMPL_GENDER_FEMALE);
    s7.smpl_betas({0.5f, -0.25f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f});
    shapes.persons().push_back(s7);
    msg::HumanShape s9;
    s9.person(9);
    s9.joint_set(msg::JointSet::JOINT_SET_COCO17);
    s9.joint_set_version(1);
    s9.bone_lengths().assign(16, 0.3f);
    shapes.persons().push_back(s9);
    CHECK(encode(shapes, wire));
    msg::HumanShapeMessage shapes_out;
    CHECK(decode(wire, shapes_out));
    // NaN bone lengths (not frozen yet) survive the wire; operator== cannot compare them, so re-encode.
    std::vector<char> wire2;
    CHECK(encode(shapes_out, wire2));
    CHECK(wire2 == wire);
    CHECK(shapes_out.persons().size() == 2);
    CHECK(std::isnan(shapes_out.persons()[0].bone_lengths()[18]) && shapes_out.persons()[0].bone_lengths()[0] == 0.2f);
    CHECK(shapes_out.persons()[0].smpl_gender().has_value() &&
          *shapes_out.persons()[0].smpl_gender() == msg::SmplGender::SMPL_GENDER_FEMALE);
    CHECK(shapes_out.persons()[0].smpl_betas().size() == 16 && shapes_out.persons()[0].smpl_betas()[1] == -0.25f);
    CHECK(!shapes_out.persons()[1].smpl_model().has_value() && !shapes_out.persons()[1].smpl_gender().has_value());
    CHECK(shapes_out.persons()[1].smpl_betas().empty() && shapes_out.persons()[1].bone_lengths().size() == 16);

    // PersonStreamInfo: the descriptor's person case.
    msg::PersonStreamInfo info;
    info.joint_set(msg::JointSet::JOINT_SET_HALPE26);
    info.joint_set_version(1);
    info.up().z(1.0);
    info.coast_limit_s(1.0f);
    info.reid_window_s(5.0f);
    info.smoothing(msg::PersonSmoothing::PERSON_SMOOTHING_ONE_EURO);
    CHECK(encode(info, wire));
    msg::PersonStreamInfo info_out;
    CHECK(decode(wire, info_out));
    CHECK(info_out == info);
    CHECK(static_cast<int>(msg::PersonSmoothing::PERSON_SMOOTHING_ONE_EURO) == 2);
    CHECK(static_cast<int>(msg::SmplGender::SMPL_GENDER_NEUTRAL) == 1);  // UNKNOWN is 0 (E6)

    // TimeRef: the documented bytes, both ways.
    msg::TimeRefRequest req;
    req.t1(1000000000);
    CHECK(encode(req, wire));
    // XCDR2: encapsulation 00 09, DHEADER (8 bytes follow), t1.
    CHECK(wire == bytes({0x00, 0x09, 0x00, 0x00, 0x08, 0, 0, 0, 0x00, 0xca, 0x9a, 0x3b, 0, 0, 0, 0}));
    msg::TimeRefReply rep;
    rep.t1(1000000000); rep.t2(1000250000); rep.t3(1000260000);
    CHECK(encode(rep, wire));
    CHECK(wire == bytes({0x00, 0x09, 0x00, 0x00, 0x18, 0, 0, 0, 0x00, 0xca, 0x9a, 0x3b, 0, 0, 0, 0,
                         0x90, 0x9a, 0x9e, 0x3b, 0, 0, 0, 0, 0xa0, 0xc1, 0x9e, 0x3b, 0, 0, 0, 0}));
    msg::TimeRefReply rep_out;
    CHECK(decode(wire, rep_out));
    CHECK(rep_out.t1() == 1000000000 && rep_out.t2() == 1000250000 && rep_out.t3() == 1000260000);

    if (g_failures == 0) std::printf("human_pose, time_ref: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
