// SPDX-License-Identifier: internal
//
// NodeName / LocalNodeId / GlobalNodeId.
#include <string>
#include <type_traits>

#include "check.h"
#include "tcn/sqe/node_name.h"

using namespace tcn::sqe;

static_assert(!std::is_default_constructible<NodeName>::value, "");
static_assert(!std::is_constructible<NodeName, std::string>::value, "");
static_assert(!std::is_default_constructible<LocalNodeId>::value, "");
static_assert(!std::is_constructible<LocalNodeId, std::string>::value, "");

// Distinct types, so the fragment rule cannot be applied to a global id "for
// symmetry" and admit a '/' into a bare global name.
static_assert(!std::is_same<GlobalNodeId, LocalNodeId>::value, "");
static_assert(!std::is_same<GlobalNodeId, NodeName>::value, "");

TCN_SQE_TEST(a_node_name_takes_the_identifier_rule)
{
    CHECK_OK(NodeName::parse("marker_7"));
    CHECK_ERR(NodeName::parse(""), Error::Empty);
    CHECK_ERR(NodeName::parse("a/b"), Error::ContainsSlash);
    CHECK_ERR(NodeName::parse("caf\xC3\xA9"), Error::NonAscii);
}

TCN_SQE_TEST(a_local_node_id_is_fragment_then_name)
{
    const Result<ClientId> c = ClientId::parse("tcn/loc/pcpd/cam1");
    const Result<NodeName> n = NodeName::parse("marker_7");
    CHECK_OK(c); CHECK_OK(n);
    if (!c || !n) { return; }
    CHECK_STR_EQ(LocalNodeId::compose(c.value(), n.value()).str(),
                 "tcn/loc/pcpd/cam1::marker_7");
}

// `:` is a legal identifier character, so any split but the first is wrong.
TCN_SQE_TEST(a_local_node_id_splits_at_the_first_separator)
{
    const Result<LocalNodeId> id = LocalNodeId::parse("a::b::c");
    CHECK_OK(id);
    if (!id) { return; }
    CHECK_STR_EQ(id.value().str(), "a::b::c");

    // The value that separates the two possible splits. Splitting at the
    // LAST "::" would make the fragment "a::b/c" -- which is a perfectly
    // legal fragment, because '/' is legal there -- and the name "d", and the
    // whole thing would be accepted. Splitting at the first makes the name
    // "b/c::d", which contains a '/' and is not a legal identifier. Only the
    // first split refuses it, and the engine refuses it.
    CHECK_ERR(LocalNodeId::parse("a::b/c::d"), Error::ContainsSlash);

    // The mirror: ':' is a legal identifier character, so a name may contain
    // "::" and must not be mistaken for a second separator.
    CHECK_OK(LocalNodeId::parse("tcn/loc::b::c"));
    CHECK_ERR(LocalNodeId::parse("a::b/c"), Error::ContainsSlash);
}

TCN_SQE_TEST(a_local_node_id_validates_each_half_by_its_own_rule)
{
    CHECK_OK(LocalNodeId::parse("tcn/loc/pcpd/cam1::marker_7"));
    CHECK_ERR(LocalNodeId::parse("/tcn::marker_7"), Error::LeadingSlash);
    CHECK_ERR(LocalNodeId::parse("tcn//loc::marker_7"), Error::EmptyChunk);
    CHECK_ERR(LocalNodeId::parse("tcn/loc::mark er"), Error::ContainsWhitespace);
    CHECK_ERR(LocalNodeId::parse("tcn/loc::"), Error::Empty);
    CHECK_ERR(LocalNodeId::parse("::marker_7"), Error::Empty);
    CHECK(!LocalNodeId::parse("no_separator_here").is_ok());
}

// A global id is a bare name and takes the identifier rule whole. Keeping it
// a distinct type is what stops the fragment rule being applied to it "for
// symmetry" and admitting a '/' into a bare global name.
TCN_SQE_TEST(a_global_node_id_never_takes_the_fragment_rule)
{
    CHECK_OK(GlobalNodeId::parse("world_origin"));
    CHECK_ERR(GlobalNodeId::parse("a/b"), Error::ContainsSlash);
}

// ---------------------------------------------------------------------------
// 0.10.7 (engine B94): domain node ids
// ---------------------------------------------------------------------------

static_assert(!std::is_same<DomainNodeId, LocalNodeId>::value, "");
static_assert(!std::is_same<DomainNodeId, GlobalNodeId>::value, "");

TCN_SQE_TEST(a_domain_node_id_is_sigil_domain_separator_name)
{
    const auto id = DomainNodeId::compose("tcn/ksv1/rig01", NodeName::parse("camera01_color").value());
    CHECK_OK(id);
    if (!id) { return; }
    CHECK_STR_EQ(id.value().str(), "@tcn/ksv1/rig01::camera01_color");
    CHECK_STR_EQ(std::string(id.value().domain()), "tcn/ksv1/rig01");
    CHECK_STR_EQ(std::string(id.value().name()), "camera01_color");
    CHECK(DomainNodeId::parse("@tcn/ksv1/rig01::camera01_color").value() == id.value());
}

// The sigil is what keeps a domain id apart from a Local id, even when the
// domain equals a client id: `tcn/ksv1/rig01::camera01_color` is rig01's own.
TCN_SQE_TEST(a_domain_node_id_never_equals_a_local_node_id)
{
    const auto local = LocalNodeId::compose(ClientId::parse("tcn/ksv1/rig01").value(), NodeName::parse("camera01_color").value());
    const auto domain = DomainNodeId::compose("tcn/ksv1/rig01", NodeName::parse("camera01_color").value());
    CHECK(local.str() != domain.value().str());
    CHECK_ERR(DomainNodeId::parse(local.str()), Error::IllegalCharacter);
    CHECK(!LocalNodeId::parse(domain.value().str()));  // '@' is outside the fragment alphabet
}

TCN_SQE_TEST(a_domain_takes_the_fragment_rule_without_a_separator)
{
    const NodeName n = NodeName::parse("cam").value();
    CHECK_ERR(DomainNodeId::compose("", n), Error::Empty);
    CHECK_ERR(DomainNodeId::compose("/tcn/ksv1", n), Error::LeadingSlash);
    CHECK_ERR(DomainNodeId::compose("tcn//ksv1", n), Error::EmptyChunk);
    CHECK_ERR(DomainNodeId::compose("tcn::ksv1", n), Error::IllegalCharacter);
    CHECK_OK(DomainNodeId::compose("tcn/ksv1:a", n));  // one ':' is a legal character
    CHECK_ERR(domain_node_id("tcn/ksv1", "head/left"), Error::ContainsSlash);
    CHECK(!DomainNodeId::parse("tcn/ksv1::cam"));
    CHECK(!DomainNodeId::parse("@tcn/ksv1"));
    CHECK(!DomainNodeId::parse("@::cam"));
}
