// SPDX-License-Identifier: internal
//
// Key construction. Nine keys, and the client id that is allowed to build them.
#include <string>
#include <type_traits>

#include "check.h"
#include "tcn/sqe/client_id.h"

using namespace tcn::sqe;

// A ClientId exists only if it validated. There is no default-constructed one
// to forget to fill in, and no way to make one from an unchecked string.
static_assert(!std::is_default_constructible<ClientId>::value,
              "a ClientId must not exist without having been validated");
static_assert(!std::is_constructible<ClientId, std::string>::value, "");
static_assert(!std::is_constructible<ClientId, const char*>::value, "");
static_assert(std::is_copy_constructible<ClientId>::value, "");

TCN_SQE_TEST(client_id_is_validated_by_the_fragment_rule_not_the_identifier_rule)
{
    // This is B39: the id below is the documented default prefix shape, and
    // validating it with the identifier rule is what took the daemon down.
    CHECK_OK(ClientId::parse("tcn/loc/pcpd/cam1"));
    CHECK_OK(ClientId::parse("hmd"));
    CHECK_ERR(ClientId::parse(""), Error::Empty);
    CHECK_ERR(ClientId::parse("/tcn/loc"), Error::LeadingSlash);
    CHECK_ERR(ClientId::parse("tcn/loc/"), Error::TrailingSlash);
    CHECK_ERR(ClientId::parse("tcn//loc"), Error::EmptyChunk);
    CHECK_ERR(ClientId::parse("tcn/loc *"), Error::ContainsWhitespace);
    CHECK_ERR(ClientId::parse("tcn/*"), Error::ContainsWildcard);
    CHECK_ERR(ClientId::parse("tcn/l\xC3\xB8" "c"), Error::NonAscii);
}

TCN_SQE_TEST(client_id_builds_all_nine_keys)
{
    const Result<ClientId> id = ClientId::parse("tcn/loc/pcpd/cam1");
    CHECK_OK(id);
    if (!id) { return; }
    const ClientId& c = id.value();

    CHECK_STR_EQ(c.str(),               "tcn/loc/pcpd/cam1");
    CHECK_STR_EQ(c.join_key(),          "tcn/loc/pcpd/cam1/sis/join");
    CHECK_STR_EQ(c.leave_key(),         "tcn/loc/pcpd/cam1/sis/leave");
    CHECK_STR_EQ(c.alive_key(),         "tcn/loc/pcpd/cam1/sis/alive");
    CHECK_STR_EQ(c.node_update_key(),   "tcn/loc/pcpd/cam1/sis/component/update");
    CHECK_STR_EQ(c.node_remove_key(),   "tcn/loc/pcpd/cam1/sis/component/remove");
    CHECK_STR_EQ(c.edge_update_key(),   "tcn/loc/pcpd/cam1/sis/relation/update");
    CHECK_STR_EQ(c.edge_remove_key(),   "tcn/loc/pcpd/cam1/sis/relation/remove");
    CHECK_STR_EQ(c.stream_start_key(),  "tcn/loc/pcpd/cam1/sis/stream/start");
    CHECK_STR_EQ(c.stream_stop_key(),   "tcn/loc/pcpd/cam1/sis/stream/stop");
}

TCN_SQE_TEST(the_key_suffixes_are_the_engine_s)
{
    CHECK_STR_EQ(std::string(keys::kJoin),        "/sis/join");
    CHECK_STR_EQ(std::string(keys::kLeave),       "/sis/leave");
    CHECK_STR_EQ(std::string(keys::kAlive),       "/sis/alive");
    CHECK_STR_EQ(std::string(keys::kNodeUpdate),  "/sis/component/update");
    CHECK_STR_EQ(std::string(keys::kNodeRemove),  "/sis/component/remove");
    CHECK_STR_EQ(std::string(keys::kEdgeUpdate),  "/sis/relation/update");
    CHECK_STR_EQ(std::string(keys::kEdgeRemove),  "/sis/relation/remove");
    CHECK_STR_EQ(std::string(keys::kStreamStart), "/sis/stream/start");
    CHECK_STR_EQ(std::string(keys::kStreamStop),  "/sis/stream/stop");
}

// Every suffix begins with '/' and the id never ends with one, so no key can
// contain an empty chunk however the two are combined. That is the invariant
// the fragment rule's trailing-slash clause exists to hold up.
TCN_SQE_TEST(no_constructed_key_can_contain_an_empty_chunk)
{
    const Result<ClientId> id = ClientId::parse("a");
    CHECK_OK(id);
    if (!id) { return; }

    const std::string all[] = {
        id.value().join_key(), id.value().leave_key(), id.value().alive_key(),
        id.value().node_update_key(), id.value().node_remove_key(),
        id.value().edge_update_key(), id.value().edge_remove_key(),
        id.value().stream_start_key(), id.value().stream_stop_key(),
    };
    for (const std::string& k : all)
    {
        CHECK_MSG(k.find("//") == std::string::npos, "a constructed key contains an empty chunk");
        CHECK_MSG(k.front() != '/', "a constructed key has a leading slash");
        CHECK_MSG(k.back() != '/', "a constructed key has a trailing slash");
        CHECK_MSG(k.compare(0, 1, "a") == 0, "a constructed key does not begin with the client id");
    }
}
