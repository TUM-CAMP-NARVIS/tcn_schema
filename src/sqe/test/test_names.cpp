// SPDX-License-Identifier: internal
//
// NodeName. The composed node ids (`{fragment}::{name}`, `@{domain}::{name}`)
// are gone with N1: a frame is named by a FrameRef (frame_ref.h).
#include <string>
#include <type_traits>

#include "check.h"
#include "tcn/sqe/node_name.h"

using namespace tcn::sqe;

static_assert(!std::is_default_constructible<NodeName>::value, "");
static_assert(!std::is_constructible<NodeName, std::string>::value, "");

TCN_SQE_TEST(a_node_name_takes_the_identifier_rule)
{
    CHECK_OK(NodeName::parse("marker_7"));
    CHECK_ERR(NodeName::parse(""), Error::Empty);
    CHECK_ERR(NodeName::parse("a/b"), Error::ContainsSlash);
    CHECK_ERR(NodeName::parse("caf\xC3\xA9"), Error::NonAscii);
}
