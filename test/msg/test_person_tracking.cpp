// SPDX-License-Identifier: internal
//
// Round trip of tcnart_msgs::msg::PersonTrackingMessage through CDR the way the Zenoh transport carries it (XCDR2,
// encapsulation 00 09, 0.11 decisions E1). 0.11 (decisions §7 HumanPose): StreamHeader (group -> header.group_id,
// world_frame -> header.frame; up moved to PersonStreamInfo), the JointSet enum with a version, the
// PersonTrackState enum (the old PERSON_STATE_* constants + 1) and an optional previous_id.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>

#include "tcnart_msgs/msg/PersonTracking.h"
#include "tcnart_msgs/msg/PersonTrackingCdrAux.hpp"

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

static bool encode(const msg::PersonTrackingMessage& m, std::vector<char>& out)
{
    out.assign(1 << 20, 0);
    FastBuffer fb(out.data(), out.size());
    Cdr ser(fb, Cdr::DEFAULT_ENDIAN, CdrVersion::XCDRv2);
    try { ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::DELIMIT_CDR2); ser.serialize_encapsulation(); ser << m; } catch (...) { return false; }
    out.resize(ser.get_serialized_data_length());
    return true;
}

static bool decode(const std::vector<char>& in, msg::PersonTrackingMessage& m)
{
    std::vector<char> copy(in);
    FastBuffer fb(copy.data(), copy.size());
    Cdr de(fb, Cdr::DEFAULT_ENDIAN, CdrVersion::XCDRv2);
    try { de.read_encapsulation(); de >> m; } catch (...) { return false; }
    return true;
}

static msg::Person make_person(uint32_t id, std::size_t joints, std::size_t bones)
{
    msg::Person p;
    p.id(id);
    p.previous_id(id + 100);
    p.state(msg::PersonTrackState::PERSON_TRACK_STATE_CONFIRMED);
    p.ambiguous(true);
    p.confidence(0.875f);
    p.age_s(12.5);
    p.last_seen_ns(1700000000123456789LL);
    p.root().x(1.0); p.root().y(-2.0); p.root().z(0.9);
    for (std::size_t j = 0; j < joints; ++j) {
        msg::PersonJoint pj;
        pj.position().x(0.1 * j); pj.position().y(0.2 * j); pj.position().z(0.3 * j);
        pj.confidence(0.5f + 0.01f * j);
        pj.observed(j % 2 == 0);
        pj.age_s(j % 2 == 0 ? 0.f : 0.25f * j);
        p.joints().push_back(pj);
    }
    for (std::size_t b = 0; b < bones; ++b) p.bone_lengths().push_back((b < 8) ? 0.25f + 0.01f * b : std::nanf(""));
    msg::PersonCameraView v;
    v.camera(3);
    v.box()[0] = 10.f; v.box()[1] = 20.f; v.box()[2] = 110.f; v.box()[3] = 220.f;
    v.joints_used(15);
    p.views().push_back(v);
    v.camera(5);
    p.views().push_back(v);
    return p;
}

int main()
{
    // The enum values that replace the 0.10 constants (0 TENTATIVE, 1 CONFIRMED, 2 PREDICTED): each is one higher,
    // UNKNOWN takes 0 (E6). A mirror that keeps the old numbers would decode CONFIRMED as PREDICTED.
    CHECK(static_cast<int>(msg::PersonTrackState::PERSON_TRACK_STATE_UNKNOWN) == 0);
    CHECK(static_cast<int>(msg::PersonTrackState::PERSON_TRACK_STATE_TENTATIVE) == 1);
    CHECK(static_cast<int>(msg::PersonTrackState::PERSON_TRACK_STATE_CONFIRMED) == 2);
    CHECK(static_cast<int>(msg::PersonTrackState::PERSON_TRACK_STATE_PREDICTED) == 3);

    // Full message: two coco17 persons (the joint count follows the joint set).
    msg::PersonTrackingMessage in;
    in.header().capture_time_ns(1700000000123456789LL);
    in.header().sequence_number(12);
    in.header().group_id(4711);
    in.header().frame().scope(tcn_types::msg::SRNodeScope::SR_NODE_SCOPE_DOMAIN);
    in.header().frame().owner("rig");
    in.header().frame().name("world");
    in.header().frame().selector(tcn_types::msg::SRFrameSelector::SR_FRAME_SELECTOR_FRAME);
    in.joint_set(msg::JointSet::JOINT_SET_COCO17);
    in.joint_set_version(1);
    in.camera_count(4);
    in.persons().push_back(make_person(7, 17, 16));
    in.persons().push_back(make_person(9, 17, 16));
    in.persons()[1].state(msg::PersonTrackState::PERSON_TRACK_STATE_PREDICTED);
    in.camera_reprojection_px({1.5f, 0.75f, 2.0f, 1.0f});

    std::vector<char> wire;
    CHECK(encode(in, wire));
    msg::PersonTrackingMessage out;
    CHECK(decode(wire, out));
    // operator== cannot be used: the NaN bone lengths compare unequal; the re-encoded bytes must match instead.
    std::vector<char> wire2;
    CHECK(encode(out, wire2));
    CHECK(wire2 == wire);
    CHECK(out.header().group_id().has_value() && *out.header().group_id() == 4711);
    CHECK(out.header().frame().owner() == "rig" && out.header().frame().name() == "world");
    CHECK(out.header().capture_time_ns() == 1700000000123456789LL && out.header().sequence_number() == 12);
    CHECK(!out.header().clock().has_value());
    CHECK(out.joint_set() == msg::JointSet::JOINT_SET_COCO17 && out.joint_set_version() == 1);
    CHECK(out.persons().size() == 2);
    const msg::Person& a = out.persons()[0];
    const msg::Person& b = out.persons()[1];
    CHECK(a.joints().size() == 17 && a.bone_lengths().size() == 16);
    CHECK(a.joints()[16].observed() && a.joints()[16].age_s() == 0.f && a.joints()[15].age_s() == 0.25f * 15);
    CHECK(a.joints()[3].position().z() == in.persons()[0].joints()[3].position().z());
    CHECK(std::isnan(a.bone_lengths()[15]));  // NaN until frozen survives the wire
    CHECK(a.id() == 7 && a.previous_id().has_value() && *a.previous_id() == 107 && a.ambiguous());
    CHECK(a.state() == msg::PersonTrackState::PERSON_TRACK_STATE_CONFIRMED);
    CHECK(a.confidence() == 0.875f && a.age_s() == 12.5 && a.last_seen_ns() == 1700000000123456789LL);
    CHECK(a.root().x() == 1.0 && a.root().y() == -2.0 && a.root().z() == 0.9);
    CHECK(a.views().size() == 2 && a.views()[1].camera() == 5 && a.views()[0].box()[2] == 110.f);
    CHECK(b.state() == msg::PersonTrackState::PERSON_TRACK_STATE_PREDICTED && b.id() == 9);
    CHECK(out.camera_count() == 4 && out.camera_reprojection_px().size() == 4);

    // halpe26: 26 joints, 26 bones (the table's halpe26 bone list).
    msg::PersonTrackingMessage halpe;
    halpe.joint_set(msg::JointSet::JOINT_SET_HALPE26);
    halpe.joint_set_version(1);
    halpe.persons().push_back(make_person(3, 26, 26));
    CHECK(encode(halpe, wire));
    msg::PersonTrackingMessage halpe_out;
    CHECK(decode(wire, halpe_out));
    CHECK(halpe_out.joint_set() == msg::JointSet::JOINT_SET_HALPE26 && halpe_out.persons()[0].joints().size() == 26);
    CHECK(halpe_out.persons()[0].joints()[25].age_s() == 0.25f * 25);

    // Empty: no persons, no group, joint set UNKNOWN (refused by consumers, but it decodes).
    msg::PersonTrackingMessage empty;
    CHECK(encode(empty, wire));
    msg::PersonTrackingMessage empty_out;
    CHECK(decode(wire, empty_out));
    CHECK(empty_out.persons().empty() && !empty_out.header().group_id().has_value());
    CHECK(empty_out.joint_set() == msg::JointSet::JOINT_SET_UNKNOWN);
    // A track that continues none: previous_id absent (0.10 sent 0), not 0.
    msg::PersonTrackingMessage one;
    msg::Person p = make_person(1, 17, 16);
    p.previous_id().reset();
    one.persons().push_back(p);
    CHECK(encode(one, wire));
    msg::PersonTrackingMessage one_out;
    CHECK(decode(wire, one_out));
    CHECK(!one_out.persons()[0].previous_id().has_value());

    if (g_failures == 0) std::printf("person_tracking: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
