// SPDX-License-Identifier: internal
//
// The engine-issued topic shapes.
#include <string>

#include "check.h"
#include "tcn/sqe/topics.h"
#include "tcn/sqe/tracker.h"

using namespace tcn::sqe;

TCN_SQE_TEST(an_engine_issued_relation_topic_has_the_documented_shape)
{
    CHECK_STR_EQ(engine_relation_topic("lab1", "h42"), "sqe/lab1/rel/h42");
    CHECK_STR_EQ(engine_relation_topic("engine-0", "0123456789abcdef"),
                 "sqe/engine-0/rel/0123456789abcdef");
}

TCN_SQE_TEST(a_status_topic_is_the_output_topic_plus_slash_status)
{
    CHECK_STR_EQ(status_topic_for("sqe/lab1/rel/h42"), "sqe/lab1/rel/h42/status");
    CHECK_STR_EQ(status_topic_for("hmd/rel/marker_7"), "hmd/rel/marker_7/status");
    CHECK_STR_EQ(engine_relation_status_topic("lab1", "h42"), "sqe/lab1/rel/h42/status");
}

// The status topic is derived from the data topic in exactly one place, so
// the two can never disagree about the shape.
TCN_SQE_TEST(the_status_topic_is_derived_from_the_relation_topic_not_rebuilt)
{
    const std::string data = engine_relation_topic("lab1", "h42");
    CHECK_STR_EQ(engine_relation_status_topic("lab1", "h42"), status_topic_for(data));
}

TCN_SQE_TEST(the_namespace_constants_are_the_engine_s)
{
    CHECK_STR_EQ(std::string(kEngineNamespace), "sqe");
    CHECK_STR_EQ(std::string(kRelationFamily), "rel");
    CHECK_STR_EQ(std::string(kStatusSuffix), "/status");
}

// ---------------------------------------------------------------------------
// The tracker key family (tracker.h)
// ---------------------------------------------------------------------------

TCN_SQE_TEST(the_three_tracker_keys_have_the_engines_shapes)
{
    const ClientId client = ClientId::parse("tcn/loc/app/default").value();

    CHECK_STR_EQ(descriptor_key(client, "camera").value(), "tcn/loc/app/default/cfg/desc/camera");
    CHECK_STR_EQ(tracker_stream_key(client, "camera").value(), "tcn/loc/app/default/str/trk/camera");
    CHECK_STR_EQ(marker_announce_key(client, "camera", "_mtarucom___sis_marker__7").value(),
                 "tcn/loc/app/default/cfg/marker/camera/_mtarucom___sis_marker__7");
}

// The engine resolves the descriptor key *from* the announcement key, taking
// the same client and the same device out of it. Composed from one pair here,
// the two cannot name different devices -- which is the only failure mode that
// matters and the one no amount of spelling each correctly would prevent.
TCN_SQE_TEST(an_announcement_and_its_descriptor_name_the_same_device)
{
    const ClientId client = ClientId::parse("tcn/loc/app/default").value();
    const std::string announced = marker_announce_key(client, "camera", "m").value();
    const std::string descriptor = descriptor_key(client, "camera").value();

    // What the engine does, step for step: split from the right twice for the
    // marker and the device, then strip the family off what remains to recover
    // the client id -- from the right, because a client id contains '/' by
    // design. Then compose the descriptor key from those two.
    const std::size_t marker_at = announced.rfind('/');
    const std::string head = announced.substr(0, marker_at);
    const std::size_t device_at = head.rfind('/');
    const std::string device = head.substr(device_at + 1);
    const std::string family = head.substr(0, device_at);
    CHECK_STR_EQ(device, "camera");

    const std::string suffix(keys::kMarkerAnnounceFamily);
    CHECK(family.size() > suffix.size());
    CHECK_STR_EQ(family.substr(family.size() - suffix.size()), suffix);
    const std::string recovered_client = family.substr(0, family.size() - suffix.size());
    CHECK_STR_EQ(recovered_client, client.str());
    CHECK_STR_EQ(descriptor_key(ClientId::parse(recovered_client).value(), device).value(),
                 descriptor);
}

// A device or marker with a separator in it is refused, not passed through.
// The engine parses an announcement by splitting from the right twice; one
// extra separator makes the parse fail and the announcement is dropped with
// nothing said anywhere.
TCN_SQE_TEST(a_device_or_marker_that_is_not_an_identifier_is_refused)
{
    const ClientId client = ClientId::parse("tcn/loc/app/default").value();

    CHECK(!descriptor_key(client, "two/chunks"));
    CHECK(!tracker_stream_key(client, "two/chunks"));
    CHECK(!marker_announce_key(client, "two/chunks", "m"));
    CHECK(!marker_announce_key(client, "camera", "two/chunks"));
    CHECK(!descriptor_key(client, ""));
    CHECK(!marker_announce_key(client, "camera", ""));
    // A wildcard would make the token match keys it does not name.
    CHECK(!descriptor_key(client, "*"));
    CHECK(!marker_announce_key(client, "camera", "**"));
}

// Three underscores in the middle, and that is the engine's own spelling: the
// prefix ends with one and the infix begins with two. A name that differs by
// one character is a different node, fed by nothing.
TCN_SQE_TEST(a_marker_node_name_is_the_prefix_the_engine_derives)
{
    CHECK_STR_EQ(marker_node_name(MarkerKind::ArucoMarker, 7), "_mtarucom___sis_marker__7");
    CHECK_STR_EQ(marker_node_name(MarkerKind::ArucoBoard, 0), "_mtarucob___sis_marker__0");
    CHECK_STR_EQ(marker_node_name(MarkerKind::ArucoFractal, 1), "_mtarucof___sis_marker__1");
    CHECK_STR_EQ(marker_node_name(MarkerKind::IrTarget, 2), "_mtirt___sis_marker__2");
    CHECK_STR_EQ(marker_node_name(MarkerKind::HmdPose, 3), "_hmdvp___sis_marker__3");
    CHECK_STR_EQ(marker_node_name(MarkerKind::None, 4), "_mtnone___sis_marker__4");
}

// Every name it composes must survive the identifier rule, or the very key
// that carries the announcement cannot be built.
TCN_SQE_TEST(every_marker_node_name_is_a_legal_identifier_and_key_chunk)
{
    const ClientId client = ClientId::parse("c").value();
    const MarkerKind kinds[] = {MarkerKind::None,        MarkerKind::ArucoMarker,
                                MarkerKind::ArucoBoard,  MarkerKind::ArucoFractal,
                                MarkerKind::IrTarget,    MarkerKind::HmdPose};
    for (const MarkerKind kind : kinds)
    {
        const std::string name = marker_node_name(kind, 4294967295u);
        CHECK(check_identifier(name));
        CHECK(marker_announce_key(client, "cam", name));
    }
}
