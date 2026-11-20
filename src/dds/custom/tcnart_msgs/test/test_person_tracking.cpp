// SPDX-License-Identifier: internal
//
// Round trip of tcnart_msgs::msg::PersonTrackingMessage (0.10.4, new revision) through plain CDR (XCDRv1, no
// encapsulation header: the way the SQE/Zenoh transport carries it).
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
    Cdr ser(fb, Cdr::LITTLE_ENDIANNESS, CdrVersion::XCDRv1);
    try { ser << m; } catch (...) { return false; }
    out.resize(ser.get_serialized_data_length());
    return true;
}

static bool decode(const std::vector<char>& in, msg::PersonTrackingMessage& m)
{
    std::vector<char> copy(in);
    FastBuffer fb(copy.data(), copy.size());
    Cdr de(fb, Cdr::LITTLE_ENDIANNESS, CdrVersion::XCDRv1);
    try { de >> m; } catch (...) { return false; }
    return true;
}

static msg::Person make_person(uint32_t id)
{
    msg::Person p;
    p.id(id);
    p.state(msg::PERSON_STATE_CONFIRMED);
    p.confidence(0.875f);
    p.age_s(12.5);
    p.last_seen_ns(1700000000123456789LL);
    p.root().x(1.0); p.root().y(-2.0); p.root().z(0.9);
    for (int j = 0; j < 17; ++j) {
        msg::PersonJoint& pj = p.joints()[j];
        pj.position().x(0.1 * j); pj.position().y(0.2 * j); pj.position().z(0.3 * j);
        pj.confidence(0.5f + 0.01f * j);
        pj.observed(j % 2 == 0);
    }
    for (int b = 0; b < 16; ++b) p.bone_lengths()[b] = (b < 8) ? 0.25f + 0.01f * b : std::nanf("");
    msg::PersonCameraView v;
    v.camera(3);
    v.box()[0] = 10.f; v.box()[1] = 20.f; v.box()[2] = 110.f; v.box()[3] = 220.f;
    v.joints_used(15);
    p.views().push_back(v);
    v.camera(5);
    p.views().push_back(v);
    p.role(-1);
    p.role_confidence(0.f);
    p.body_model("smpl-x");
    p.body_shape({0.5f, -0.5f, 1.5f});
    p.body_rotations(std::vector<float>(216, 0.125f));
    p.body_translation().x(0.25); p.body_translation().y(0.5); p.body_translation().z(0.75);
    return p;
}

int main()
{
    // Full message.
    msg::PersonTrackingMessage in;
    in.header().stamp().sec(1700000000);
    in.header().stamp().nanosec(123456789);
    in.header().frame_id("world");
    in.world_frame("world");
    in.up().x(0.0); in.up().y(0.0); in.up().z(1.0);
    in.camera_count(4);
    in.persons().push_back(make_person(7));
    in.persons().push_back(make_person(9));
    in.persons()[1].state(msg::PERSON_STATE_PREDICTED);
    in.camera_reprojection_px({1.5f, 0.75f, 2.0f, 1.0f});

    std::vector<char> wire;
    CHECK(encode(in, wire));
    msg::PersonTrackingMessage out;
    CHECK(decode(wire, out));
    // operator== cannot be used: the NaN bone lengths compare unequal; the re-encoded bytes must match instead.
    std::vector<char> wire2;
    CHECK(encode(out, wire2));
    CHECK(wire2 == wire);
    CHECK(out.persons().size() == 2);
    CHECK(out.persons()[0].joints()[16].observed());
    CHECK(out.persons()[0].joints()[3].position().z() == in.persons()[0].joints()[3].position().z());
    CHECK(std::isnan(out.persons()[0].bone_lengths()[15]));  // NaN until frozen survives the wire
    CHECK(out.persons()[0].views().size() == 2 && out.persons()[0].views()[1].camera() == 5);
    CHECK(out.persons()[0].body_model() == "smpl-x");
    CHECK(out.persons()[1].state() == msg::PERSON_STATE_PREDICTED);
    CHECK(out.camera_count() == 4 && out.camera_reprojection_px().size() == 4);
    CHECK(msg::PERSON_STATE_TENTATIVE != msg::PERSON_STATE_CONFIRMED);

    // Empty: no persons, reserved body-model fields empty.
    msg::PersonTrackingMessage empty;
    CHECK(encode(empty, wire));
    msg::PersonTrackingMessage empty_out;
    CHECK(decode(wire, empty_out));
    CHECK(empty_out.persons().empty());

    // Reserved fields empty on a person.
    msg::PersonTrackingMessage one;
    msg::Person p = make_person(1);
    p.body_model("");
    p.body_shape({});
    p.body_rotations({});
    one.persons().push_back(p);
    CHECK(encode(one, wire));
    msg::PersonTrackingMessage one_out;
    CHECK(decode(wire, one_out));
    CHECK(one_out.persons()[0].body_shape().empty() && one_out.persons()[0].body_rotations().empty());

    if (g_failures == 0) std::printf("person_tracking: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
