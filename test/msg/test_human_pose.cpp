// SPDX-License-Identifier: internal
//
// Round trips of tcnart_msgs::msg::HumanPoseMessage and TimeRefRequest/TimeRefReply (new in 0.10.5) through CDR with
// the encapsulation header, as the Zenoh transport carries them. The TimeRef bytes are the examples of tcnart's
// v3/docs/protocols/time-reference.md.
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
    // HumanPoseMessage: two persons, 20 joints (tcnart_body_v1's count), rotations on some joints.
    msg::HumanPoseMessage m;
    m.header().stamp().sec(1700000000);
    m.header().stamp().nanosec(500000000u);
    m.header().frame_id("site_world");
    m.joint_set("tcnart_body_v1");
    m.source("tcn/loc/holoscan/pose_tracking/persons");
    for (uint32_t id : {7u, 9u}) {
        msg::HumanPose p;
        p.person(id);
        p.participant(id == 7u ? "alice" : "");
        p.state(id == 7u ? msg::HUMAN_POSE_TRACKED : msg::HUMAN_POSE_COASTING);
        p.source_ids({id + 40, id + 30});
        for (int j = 0; j < 20; ++j) {
            msg::HumanJoint hj;
            hj.position().x(0.1 * j); hj.position().y(1.0); hj.position().z(-0.05 * j);
            hj.rotation().w(1.0);
            if (j % 3 == 0) { hj.rotation().z(0.6); hj.rotation().w(0.8); }
            hj.confidence(0.9f);
            hj.flags(static_cast<uint8_t>(msg::HUMAN_JOINT_OBSERVED | (j % 3 == 0 ? msg::HUMAN_JOINT_HAS_ROTATION : 0)));
            p.joints().push_back(hj);
        }
        for (int b = 0; b < 19; ++b) p.bone_lengths().push_back(0.2f + 0.01f * b);
        m.persons().push_back(p);
    }
    std::vector<char> wire;
    CHECK(encode(m, wire));
    msg::HumanPoseMessage out;
    CHECK(decode(wire, out));
    CHECK(out.header().stamp().sec() == 1700000000 && out.header().frame_id() == "site_world");
    CHECK(out.joint_set().to_string() == "tcnart_body_v1" && out.source().to_string() == "tcn/loc/holoscan/pose_tracking/persons");
    CHECK(out.persons().size() == 2);
    const auto& p0 = out.persons()[0];
    CHECK(p0.person() == 7 && p0.participant().to_string() == "alice" && p0.state() == msg::HUMAN_POSE_TRACKED);
    CHECK(p0.source_ids().size() == 2 && p0.source_ids()[0] == 47);
    CHECK(p0.joints().size() == 20 && p0.bone_lengths().size() == 19);
    CHECK(p0.joints()[3].rotation().w() == 0.8 && (p0.joints()[3].flags() & msg::HUMAN_JOINT_HAS_ROTATION) != 0);
    CHECK((p0.joints()[4].flags() & msg::HUMAN_JOINT_HAS_ROTATION) == 0 && p0.joints()[4].rotation().w() == 1.0);
    CHECK(out.persons()[1].participant().to_string().empty() && out.persons()[1].state() == msg::HUMAN_POSE_COASTING);
    CHECK(out == m);

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
