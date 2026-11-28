// SPDX-License-Identifier: internal
//
// What an engine says it is and offers (#35 R5, D4): the protocol floor,
// capability names, and the client scope.
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "check.h"
#include "stub_msgs.h"
#include "tcn/sqe/capabilities.h"
#include "tcn/sqe/contract.h"

using namespace tcn::sqe;

// The one floor, written once: the constant is the macro.
static_assert(kProtocolBreak == 99, "");
static_assert(TCN_SQE_PROTOCOL_BREAK == kProtocolBreak, "");
static_assert(std::is_same<decltype(kProtocolBreak), const std::uint32_t>::value, "");

namespace {

using sqe_msgs::rpc::SISEngineCapabilities;

SISEngineCapabilities engine(std::uint32_t pb, std::vector<std::string> caps, std::string scope)
{
    SISEngineCapabilities e;
    e.protocol_break_ = pb;
    e.capabilities_ = std::move(caps);
    e.client_scope_ = std::move(scope);
    return e;
}

}  // namespace

TCN_SQE_TEST(the_initial_capability_names_are_the_engine_s)
{
    CHECK_STR_EQ(std::string(capability::kSample), "sample");
    CHECK_STR_EQ(std::string(capability::kDomain), "domain");
    CHECK_STR_EQ(std::string(capability::kRationalLens), "rational_lens");
    CHECK_STR_EQ(std::string(capability::kMarkerDomain), "marker_domain");
}

// An unknown name is kept, not refused: capabilities are how a newer engine
// offers a feature an older client never asks about.
TCN_SQE_TEST(capabilities_are_read_from_the_engine_and_unknown_names_are_kept)
{
    const Result<Capabilities> c =
        Capabilities::from_wire(engine(99, {"sample", "future_x", "sample"}, "tcn/loc"));
    CHECK_OK(c);
    if (!c) { return; }
    CHECK_INT_EQ(c.value().protocol_break(), 99);
    CHECK(c.value().has("sample"));
    CHECK(c.value().has(capability::kSample));
    CHECK(c.value().has("future_x"));
    CHECK(!c.value().has("domain"));
    CHECK(!c.value().has(""));
    CHECK(c.value().meets_floor());
    // A duplicate is one capability.
    CHECK_INT_EQ(c.value().names().size(), 2);
    CHECK_STR_EQ(c.value().scope().str(), "tcn/loc");
    CHECK(c.value().scope().contains(ClientId::parse("tcn/loc/a").value()));
}

TCN_SQE_TEST(a_malformed_capability_name_is_refused)
{
    CHECK_ERR(Capabilities::from_wire(engine(99, {"Sample"}, "")), Error::MalformedCapability);
    CHECK_ERR(Capabilities::from_wire(engine(99, {""}, "")), Error::MalformedCapability);
    CHECK_ERR(Capabilities::from_wire(engine(99, {std::string(65, 'a')}, "")), Error::MalformedCapability);
    CHECK_ERR(Capabilities::from_wire(engine(99, {"sample", "rational-lens"}, "")), Error::MalformedCapability);
    CHECK_ERR(Capabilities::from_wire(engine(99, {"a b"}, "")), Error::MalformedCapability);
    CHECK_OK(Capabilities::from_wire(engine(99, {std::string(64, 'a')}, "")));
    CHECK_OK(Capabilities::from_wire(engine(99, {"abc_0123456789_z"}, "")));
}

TCN_SQE_TEST(a_malformed_client_scope_is_refused)
{
    CHECK_ERR(Capabilities::from_wire(engine(99, {}, "/tcn")), Error::LeadingSlash);
    const Result<Capabilities> c = Capabilities::from_wire(engine(99, {}, ""));
    CHECK_OK(c);
    if (c) { CHECK(c.value().scope().unscoped()); CHECK(c.value().names().empty()); }
}

TCN_SQE_TEST(an_engine_below_the_floor_does_not_meet_it)
{
    CHECK(!Capabilities::from_wire(engine(98, {"sample"}, "")).value().meets_floor());
    CHECK(!Capabilities::from_wire(engine(0, {}, "")).value().meets_floor());
    CHECK(Capabilities::from_wire(engine(100, {}, "")).value().meets_floor());
}
