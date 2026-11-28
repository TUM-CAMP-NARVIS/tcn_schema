// SPDX-License-Identifier: internal
//
// The engine-issued topic shapes.
#include <cstdint>
#include <optional>
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
    CHECK_STR_EQ(plan_topic_for("sqe/lab1/rel/h42"), "sqe/lab1/rel/h42/plan");
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
// Engine presence (B74) and the registry's change topic (B78)
// ---------------------------------------------------------------------------

// The epoch is the engine's process start in Unix milliseconds, in decimal.
// Thirteen digits today; the key must carry all of them, and nothing else.
TCN_SQE_TEST(the_engine_alive_key_and_pattern_have_the_engines_shapes)
{
    CHECK_STR_EQ(engine_alive_key("lab1", 1762300800123ull), "sqe/lab1/alive/1762300800123");
    CHECK_STR_EQ(engine_alive_key("lab1", 0), "sqe/lab1/alive/0");
    CHECK_STR_EQ(engine_alive_key("e", 18446744073709551615ull), "sqe/e/alive/18446744073709551615");
    CHECK_STR_EQ(engine_alive_pattern("lab1"), "sqe/lab1/alive/*");
    CHECK_STR_EQ(std::string(kEngineAliveFamily), "alive");
}

// What a liveliness subscriber hands back is parsed into the epoch, and the
// epoch is what tells "restarted" from "still the same engine". The parse is
// the inverse of the composition.
TCN_SQE_TEST(an_engine_alive_key_parses_back_to_its_epoch)
{
    const std::optional<std::uint64_t> e = parse_engine_alive_key("sqe/lab1/alive/1762300800123");
    CHECK(e.has_value());
    if (e) { CHECK(*e == 1762300800123ull); }

    CHECK(parse_engine_alive_key(engine_alive_key("lab1", 42), "lab1") == std::optional<std::uint64_t>(42));
    CHECK(parse_engine_alive_key(engine_alive_key("e", 18446744073709551615ull))
          == std::optional<std::uint64_t>(18446744073709551615ull));
}

// A subscriber on `sqe/{id}/alive/*` should see nothing else, but a parse that
// trusted that would turn a stray key into a phantom restart. Every malformed
// shape is nothing, not zero.
TCN_SQE_TEST(a_key_that_is_not_an_engine_alive_key_parses_to_nothing)
{
    CHECK(!parse_engine_alive_key(""));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive/"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive/*"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive/12x"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive/+12"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive/-12"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive/ 12"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alive/12/extra"));
    CHECK(!parse_engine_alive_key("sqe/lab1/rel/12"));
    CHECK(!parse_engine_alive_key("sqe//alive/12"));
    CHECK(!parse_engine_alive_key("sqex/lab1/alive/12"));
    CHECK(!parse_engine_alive_key("tcn/lab1/alive/12"));
    CHECK(!parse_engine_alive_key("sqe/lab1/alivex/12"));
    // One past uint64 max does not wrap to a small epoch.
    CHECK(!parse_engine_alive_key("sqe/e/alive/18446744073709551616"));
    // Another engine's token, when the caller asked about one engine.
    CHECK(!parse_engine_alive_key("sqe/lab2/alive/12", "lab1"));
    CHECK(parse_engine_alive_key("sqe/lab2/alive/12").has_value());
}

TCN_SQE_TEST(the_operator_keys_have_the_engines_shape)
{
    CHECK_STR_EQ(engine_operator_key("camp", kOperatorInfo), "sqe/camp/op/info");
    CHECK_STR_EQ(engine_operator_key("camp", kOperatorGraphQuery), "sqe/camp/op/graph/query");
    CHECK_STR_EQ(engine_operator_key("camp", kOperatorChanges), "sqe/camp/op/changes");
    CHECK_STR_EQ(engine_operator_key("camp", kOperatorExplain), "sqe/camp/op/explain");
    CHECK_STR_EQ(engine_operator_key("camp", kOperatorTxlog), "sqe/camp/op/txlog");
}

TCN_SQE_TEST(the_targets_changed_topic_has_the_engines_shape)
{
    CHECK_STR_EQ(engine_targets_changed_topic("lab1"), "sqe/lab1/targets/changed");
}

// The unscoped engine summary (#35 R5, D4): any client may ask any engine
// what it is and which client ids it serves, before joining.
TCN_SQE_TEST(the_engine_info_key_and_pattern_have_the_engines_shapes)
{
    CHECK_STR_EQ(engine_info_key("e1"), "sqe/e1/info");
    CHECK_STR_EQ(engine_info_pattern(), "sqe/*/info");
    // Not the operator info key, which is under op/.
    CHECK(engine_info_key("e1") != engine_operator_key("e1", kOperatorInfo));
}

// ---------------------------------------------------------------------------
// The tracker key family (tracker.h)
// ---------------------------------------------------------------------------

TCN_SQE_TEST(the_three_tracker_keys_have_the_engines_shapes)
{
    const ClientId client = ClientId::parse("tcn/loc/app/default").value();

    CHECK_STR_EQ(descriptor_key(client, "camera").value(), "tcn/loc/app/default/cfg/desc/camera");
    CHECK_STR_EQ(tracker_stream_key(client, "camera").value(), "tcn/loc/app/default/str/trk/camera");
    CHECK_STR_EQ(marker_announce_key(client, "camera", 7).value(),
                 "tcn/loc/app/default/cfg/marker/camera/7");
}

// The marker is its numeric id in decimal (#35 H5), not a composed node name.
TCN_SQE_TEST(a_marker_announcement_names_the_marker_by_its_decimal_id)
{
    const ClientId a = ClientId::parse("a").value();
    CHECK_STR_EQ(marker_announce_key(a, "cam", 7).value(), "a/cfg/marker/cam/7");
    CHECK_STR_EQ(marker_announce_key(a, "cam", 0).value(), "a/cfg/marker/cam/0");
    CHECK_STR_EQ(marker_announce_key(a, "cam", 4294967295u).value(), "a/cfg/marker/cam/4294967295");
}

// The engine resolves the descriptor key *from* the announcement key, taking
// the same client and the same device out of it. Composed from one pair here,
// the two cannot name different devices -- which is the only failure mode that
// matters and the one no amount of spelling each correctly would prevent.
TCN_SQE_TEST(an_announcement_and_its_descriptor_name_the_same_device)
{
    const ClientId client = ClientId::parse("tcn/loc/app/default").value();
    const std::string announced = marker_announce_key(client, "camera", 12).value();
    const std::string descriptor = descriptor_key(client, "camera").value();

    // What the engine does, step for step: split from the right twice for the
    // marker and the device, then strip the family off what remains to recover
    // the client id -- from the right, because a client id contains '/' by
    // design. Then compose the descriptor key from those two.
    const std::size_t marker_at = announced.rfind('/');
    CHECK_STR_EQ(announced.substr(marker_at + 1), "12");
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

// A device with a separator in it is refused, not passed through. The engine
// parses an announcement by splitting from the right twice; one extra
// separator makes the parse fail and the announcement is dropped with nothing
// said anywhere.
TCN_SQE_TEST(a_device_that_is_not_an_identifier_is_refused)
{
    const ClientId client = ClientId::parse("tcn/loc/app/default").value();

    CHECK_ERR(descriptor_key(client, "two/chunks"), Error::ContainsSlash);
    CHECK_ERR(tracker_stream_key(client, "two/chunks"), Error::ContainsSlash);
    CHECK_ERR(marker_announce_key(client, "two/chunks", 1), Error::ContainsSlash);
    CHECK_ERR(descriptor_key(client, ""), Error::Empty);
    CHECK_ERR(marker_announce_key(client, "", 1), Error::Empty);
    // A wildcard would make the token match keys it does not name.
    CHECK_ERR(descriptor_key(client, "*"), Error::ContainsWildcard);
    CHECK_ERR(marker_announce_key(client, "**", 1), Error::ContainsWildcard);
}
