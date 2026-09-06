// SPDX-License-Identifier: internal
//
// The terms of a relation stream: what the library refuses, and what "the same
// terms" means.
#include <limits>
#include <string>

#include "check.h"
#include "tcn/sqe/relation.h"

using namespace tcn::sqe;

namespace {

FrameRef frame(const char* client, const char* node)
{
    return FrameRef::parse(client, node).value();
}

StreamTrigger hz(double v) { return StreamTrigger::fixed_rate(v).value(); }

}  // namespace

// ---------------------------------------------------------------------------
// A frame reference is two different rules, not one
// ---------------------------------------------------------------------------

// The client half is a key prefix and takes the fragment rule; the node half is
// a bare name and takes the identifier rule. Validating the client half as an
// identifier is B39, and it took the daemon down.
TCN_SQE_TEST(a_frame_ref_applies_the_fragment_rule_to_the_client_and_the_identifier_rule_to_the_node)
{
    CHECK_OK(FrameRef::parse("tcn/loc/pcpd/hl2-01", "head"));
    CHECK_OK(FrameRef::parse("single", "head"));

    // A '/' is legal on the left and illegal on the right.
    CHECK_ERR(FrameRef::parse("tcn/loc/pcpd/hl2-01", "head/left"), Error::ContainsSlash);

    // ... and the fragment rule's chunk structure still applies on the left.
    CHECK_ERR(FrameRef::parse("/tcn/loc", "head"), Error::LeadingSlash);
    CHECK_ERR(FrameRef::parse("tcn//loc", "head"), Error::EmptyChunk);
    CHECK_ERR(FrameRef::parse("tcn/loc/", "head"), Error::TrailingSlash);

    CHECK_ERR(FrameRef::parse("", "head"), Error::Empty);
    CHECK_ERR(FrameRef::parse("tcn/loc", ""), Error::Empty);
    CHECK_ERR(FrameRef::parse("tcn/*/loc", "head"), Error::ContainsWildcard);
    CHECK_ERR(FrameRef::parse("tcn/loc", "he ad"), Error::ContainsWhitespace);
}

TCN_SQE_TEST(frame_refs_order_by_client_then_node)
{
    CHECK(frame("a", "z") < frame("b", "a"));
    CHECK(frame("a", "a") < frame("a", "b"));
    CHECK(!(frame("a", "a") < frame("a", "a")));
    CHECK(frame("a", "a") == frame("a", "a"));
    CHECK(frame("a", "a") != frame("a", "b"));
}

// ---------------------------------------------------------------------------
// Triggers
// ---------------------------------------------------------------------------

TCN_SQE_TEST(a_fixed_rate_trigger_refuses_anything_that_is_not_a_positive_finite_rate)
{
    CHECK_OK(StreamTrigger::fixed_rate(30.0));
    CHECK_OK(StreamTrigger::fixed_rate(0.001));

    CHECK_ERR(StreamTrigger::fixed_rate(0.0), Error::InvalidTriggerRate);
    CHECK_ERR(StreamTrigger::fixed_rate(-30.0), Error::InvalidTriggerRate);

    // NaN compares false against everything, which is why the check is written
    // as a positive test: a `<= 0` test would let NaN straight through and the
    // engine would be asked to run a timer at "not a number".
    CHECK_ERR(StreamTrigger::fixed_rate(std::numeric_limits<double>::quiet_NaN()),
              Error::InvalidTriggerRate);
    CHECK_ERR(StreamTrigger::fixed_rate(std::numeric_limits<double>::infinity()),
              Error::InvalidTriggerRate);
    CHECK_ERR(StreamTrigger::fixed_rate(-std::numeric_limits<double>::infinity()),
              Error::InvalidTriggerRate);
}

TCN_SQE_TEST(an_on_stream_trigger_needs_a_topic_and_on_any_needs_nothing)
{
    CHECK_OK(StreamTrigger::on_stream("tcn/loc/pcpd/cam/pose"));
    CHECK_ERR(StreamTrigger::on_stream(""), Error::EmptyTriggerTopic);

    const StreamTrigger any = StreamTrigger::on_any();
    CHECK(any.kind() == TriggerKind::OnAny);
    CHECK_STR_EQ(any.kind_name(), "SRG_TRIGGER_ON_ANY");
}

// The ordinals are the IDL's declaration order, spelled out so a renumbering
// upstream is a visible diff here rather than a stream evaluated on the wrong
// clock.
TCN_SQE_TEST(trigger_kind_ordinals_match_the_idl_declaration_order)
{
    CHECK_INT_EQ(static_cast<int>(TriggerKind::FixedRate), 0);
    CHECK_INT_EQ(static_cast<int>(TriggerKind::OnStream), 1);
    CHECK_INT_EQ(static_cast<int>(TriggerKind::OnAny), 2);
    CHECK_INT_EQ(static_cast<int>(TriggerKind::OnReference), 3);
}

// Same reasoning one level down: the policy is its own IDL enum, and it is
// the payload of union case 3, so a renumbering here selects a different hop
// of the path rather than failing.
TCN_SQE_TEST(reference_policy_ordinals_match_the_idl_declaration_order)
{
    CHECK_INT_EQ(static_cast<int>(ReferencePolicy::First), 0);
    CHECK_INT_EQ(static_cast<int>(ReferencePolicy::Fastest), 1);
    CHECK_INT_EQ(static_cast<int>(ReferencePolicy::Slowest), 2);
}

// The trigger a composed relation should be asking for. `on_any` fires once
// per input, so its rate is the sum of its inputs' -- and a reused derived
// edge carries that inflation downstream.
TCN_SQE_TEST(a_reference_trigger_defaults_to_the_first_hop_and_carries_its_policy)
{
    const StreamTrigger first = StreamTrigger::on_reference();
    CHECK(first.kind() == TriggerKind::OnReference);
    CHECK(first.policy() == ReferencePolicy::First);
    CHECK_STR_EQ(first.kind_name(), "SRG_TRIGGER_ON_REFERENCE");
    CHECK_STR_EQ(first.policy_name(), "SRG_REF_FIRST");

    const StreamTrigger slowest = StreamTrigger::on_reference(ReferencePolicy::Slowest);
    CHECK(slowest.policy() == ReferencePolicy::Slowest);
    CHECK_STR_EQ(slowest.policy_name(), "SRG_REF_SLOWEST");

    // Two policies are two different requests: the engine picks a different
    // hop, so the stream is composed off a different sensor's clock.
    CHECK(first != slowest);
    CHECK(first == StreamTrigger::on_reference(ReferencePolicy::First));
    CHECK(first != StreamTrigger::on_any());
}

// Exact equality, not an epsilon. 30 Hz and 30.000001 Hz are different streams
// -- the engine compiles them differently -- and an epsilon here would hand the
// second caller the first caller's stream while reporting success.
TCN_SQE_TEST(triggers_compare_exactly_and_across_kinds)
{
    CHECK(hz(30.0) == hz(30.0));
    CHECK(hz(30.0) != hz(30.000001));
    CHECK(hz(30.0) != StreamTrigger::on_any());
    CHECK(StreamTrigger::on_any() == StreamTrigger::on_any());
    // Both orders. `on_any` carries no payload to disagree about, so a
    // comparison that did not check the kind first would call it equal to
    // everything -- but only when it is on the left.
    CHECK(StreamTrigger::on_any() != hz(30.0));
    CHECK(StreamTrigger::on_any() != StreamTrigger::on_stream("a").value());

    const StreamTrigger a = StreamTrigger::on_stream("a").value();
    const StreamTrigger b = StreamTrigger::on_stream("b").value();
    CHECK(a == StreamTrigger::on_stream("a").value());
    CHECK(a != b);
    // A fixed-rate trigger's topic is empty and an on-stream trigger's hz is
    // zero, so a comparison that read the wrong member would call these equal.
    CHECK(a != hz(30.0));
}

// ---------------------------------------------------------------------------
// The request
// ---------------------------------------------------------------------------

// Refused, not clamped. A caller who passed 30 meant something, and clamping to
// 1 would silently give them a stream that ignores every derived edge.
TCN_SQE_TEST(freshness_bias_outside_the_unit_interval_is_refused_not_clamped)
{
    CHECK_OK(RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 0.0));
    CHECK_OK(RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 1.0));
    CHECK_OK(RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 0.5));

    CHECK_ERR(RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), -0.0001),
              Error::FreshnessBiasOutOfRange);
    CHECK_ERR(RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 1.0001),
              Error::FreshnessBiasOutOfRange);
    CHECK_ERR(RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 30.0),
              Error::FreshnessBiasOutOfRange);
    CHECK_ERR(RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0),
                                          std::numeric_limits<double>::quiet_NaN()),
              Error::FreshnessBiasOutOfRange);
}

TCN_SQE_TEST(the_default_freshness_bias_prefers_reuse)
{
    const RelationStreamRequest r =
        RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0)).value();
    CHECK(r.freshness_bias() == kDefaultFreshnessBias);
    CHECK(kDefaultFreshnessBias == 0.0);
}

// "The same relation" is the two frames and nothing else. That is what lets a
// second acquire on *different* terms be caught locally as a conflict instead
// of being sent to the engine, which answers it `Rejected` with no way for the
// caller to tell which of the two requests was the odd one.
TCN_SQE_TEST(a_relation_is_the_two_frames_and_excludes_the_terms)
{
    const RelationStreamRequest fast =
        RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(90.0), 1.0).value();
    const RelationStreamRequest slow =
        RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 0.0).value();
    const RelationStreamRequest other =
        RelationStreamRequest::make(frame("a", "x"), frame("b", "z"), hz(30.0), 0.0).value();

    CHECK(fast.relation() == slow.relation());
    CHECK(!(other.relation() == slow.relation()));
    CHECK(!fast.same_terms_as(slow));
    CHECK(slow.same_terms_as(other));

    // The relation is direction-sensitive: "head as seen from world" is not
    // "world as seen from head".
    const RelationStreamRequest reversed =
        RelationStreamRequest::make(frame("b", "y"), frame("a", "x"), hz(30.0), 0.0).value();
    CHECK(!(reversed.relation() == slow.relation()));
}

TCN_SQE_TEST(same_terms_compares_the_trigger_and_the_bias_independently)
{
    const RelationStreamRequest base =
        RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 0.25).value();
    const RelationStreamRequest same_trigger_other_bias =
        RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(30.0), 0.75).value();
    const RelationStreamRequest other_trigger_same_bias =
        RelationStreamRequest::make(frame("a", "x"), frame("b", "y"), hz(60.0), 0.25).value();

    CHECK(base.same_terms_as(base));
    CHECK(!base.same_terms_as(same_trigger_other_bias));
    CHECK(!base.same_terms_as(other_trigger_same_bias));
}
