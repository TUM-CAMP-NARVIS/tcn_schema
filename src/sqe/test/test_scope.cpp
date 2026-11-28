// SPDX-License-Identifier: internal
//
// ClientScope: which client ids an engine serves (#35 R5).
#include <string>
#include <type_traits>

#include "check.h"
#include "tcn/sqe/scope.h"

using namespace tcn::sqe;

static_assert(!std::is_default_constructible<ClientScope>::value, "");
static_assert(!std::is_constructible<ClientScope, std::string>::value, "");
static_assert(!std::is_constructible<ClientScope, const char*>::value, "");
static_assert(!std::is_convertible<const char*, ClientScope>::value, "");

namespace {

ClientId id(const char* s) { return ClientId::parse(s).value(); }

}  // namespace

TCN_SQE_TEST(the_empty_scope_is_unscoped_and_contains_every_client)
{
    const Result<ClientScope> s = ClientScope::parse("");
    CHECK_OK(s);
    if (!s) { return; }
    CHECK(s.value().unscoped());
    CHECK_STR_EQ(s.value().str(), "");
    CHECK(s.value().contains(id("a/b")));
    CHECK(s.value().contains(id("x")));
}

// A prefix ending at a `/` chunk boundary, or the id itself (Review Focus 5).
TCN_SQE_TEST(a_scope_contains_itself_and_the_ids_below_it_at_a_chunk_boundary)
{
    const Result<ClientScope> s = ClientScope::parse("tcn/loc/ch");
    CHECK_OK(s);
    if (!s) { return; }
    CHECK(!s.value().unscoped());
    CHECK_STR_EQ(s.value().str(), "tcn/loc/ch");
    CHECK(s.value().contains(id("tcn/loc/ch")));
    CHECK(s.value().contains(id("tcn/loc/ch/room")));
    CHECK(s.value().contains(id("tcn/loc/ch/room/camera")));
    CHECK(!s.value().contains(id("tcn/loc/chx")));
    CHECK(!s.value().contains(id("tcn/loc")));
    CHECK(!s.value().contains(id("tcn/loc/c")));
    CHECK(!s.value().contains(id("xtcn/loc/ch")));
    CHECK(!s.value().contains(id("tcn/loc/ch:x")));
}

TCN_SQE_TEST(a_scope_takes_the_fragment_rule)
{
    CHECK_ERR(ClientScope::parse("/x"), Error::LeadingSlash);
    CHECK_ERR(ClientScope::parse("x/"), Error::TrailingSlash);
    CHECK_ERR(ClientScope::parse("a//b"), Error::EmptyChunk);
    CHECK_ERR(ClientScope::parse("tcn/*"), Error::ContainsWildcard);
    CHECK_ERR(ClientScope::parse("tcn loc"), Error::ContainsWhitespace);
}
