// SPDX-License-Identifier: internal
//
// The `application/cdr;{TYPE};tcn_schema={M.m}` annotation.
#include <string>

#include "check.h"
#include "tcn/sqe/wire_type.h"

using namespace tcn::sqe;
namespace msg = tcnart_msgs::msg;
namespace rpc = tcnart_msgs::rpc;

TCN_SQE_TEST(cdr_encoding_names_every_registered_type_exactly)
{
    CHECK_STR_EQ(cdr_encoding<msg::Pose6DMessage>(),          "application/cdr;tcnart_msgs::msg::Pose6DMessage;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::Pose6DMapMessage>(),       "application/cdr;tcnart_msgs::msg::Pose6DMapMessage;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::TargetTrackingMessage>(),  "application/cdr;tcnart_msgs::msg::TargetTrackingMessage;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::StreamDescriptorMessage>(),"application/cdr;tcnart_msgs::msg::StreamDescriptorMessage;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::SRGraph>(),                "application/cdr;tcnart_msgs::msg::SRGraph;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::SRNode>(),                 "application/cdr;tcnart_msgs::msg::SRNode;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::SREdge>(),                 "application/cdr;tcnart_msgs::msg::SREdge;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::SRNodeView>(),             "application/cdr;tcnart_msgs::msg::SRNodeView;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<msg::SREdgeView>(),             "application/cdr;tcnart_msgs::msg::SREdgeView;tcn_schema=0.9");

    CHECK_STR_EQ(cdr_encoding<rpc::SISJoinRequest>(),         "application/cdr;tcnart_msgs::rpc::SISJoinRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISLeaveRequest>(),        "application/cdr;tcnart_msgs::rpc::SISLeaveRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISNodeUpdateRequest>(),   "application/cdr;tcnart_msgs::rpc::SISNodeUpdateRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISNodeRemoveRequest>(),   "application/cdr;tcnart_msgs::rpc::SISNodeRemoveRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEdgeUpdateRequest>(),   "application/cdr;tcnart_msgs::rpc::SISEdgeUpdateRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEdgeRemoveRequest>(),   "application/cdr;tcnart_msgs::rpc::SISEdgeRemoveRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStartRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStartRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStopRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStopRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStatus>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStatus;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamHandle>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamHandle;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISReoptimisePolicy>(),
                 "application/cdr;tcnart_msgs::rpc::SISReoptimisePolicy;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISBlockingRelation>(),
                 "application/cdr;tcnart_msgs::rpc::SISBlockingRelation;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamReply;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStatusNotification>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStatusNotification;tcn_schema=0.9");

    CHECK_STR_EQ(cdr_encoding<rpc::SISMutationStatus>(),
                 "application/cdr;tcnart_msgs::rpc::SISMutationStatus;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISMutationReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryKind>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryKind;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryStatus>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryStatus;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryReply;tcn_schema=0.9");

    // 0.9.0: the pattern read (B63) and the target registry (B78).
    CHECK_STR_EQ(cdr_encoding<rpc::SRPatternExpansion>(),
                 "application/cdr;tcnart_msgs::rpc::SRPatternExpansion;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISPatternQueryRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISPatternQueryRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISPatternQueryReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISPatternQueryReply;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetKind>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetKind;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetAlias>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetAlias;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGlobalTarget>(),
                 "application/cdr;tcnart_msgs::rpc::SISGlobalTarget;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetQueryKind>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetQueryKind;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetQueryRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetQueryRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetQueryStatus>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetQueryStatus;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetQueryReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetQueryReply;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetMutationKind>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetMutationKind;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetMutationRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetMutationRequest;tcn_schema=0.9");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetChange>(),
                 "application/cdr;tcnart_msgs::rpc::SISTargetChange;tcn_schema=0.9");
}

// The two replies are answers to different questions on different keys, and a
// reader of one must refuse the other. Nothing about their CDR layouts stops a
// SISMutationReply decoding as a SISGraphQueryReply -- both open with an enum
// -- so the annotation is the only thing that separates them.
TCN_SQE_TEST(the_two_new_replies_do_not_answer_for_each_other)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;tcnart_msgs::rpc::SISMutationReply"));
    CHECK_OK(check_declared_type<rpc::SISGraphQueryReply>(
        "application/cdr;tcnart_msgs::rpc::SISGraphQueryReply"));

    CHECK_ERR(check_declared_type<rpc::SISGraphQueryReply>(
                  "application/cdr;tcnart_msgs::rpc::SISMutationReply"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISGraphQueryReply"),
              Error::WrongDeclaredType);

    // /sis/relation/update is a queryable: the `get` carries a request and is
    // answered with something else entirely, on one key. Reading the reply as
    // the request is the mistake that shape invites.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISEdgeUpdateRequest"),
              Error::WrongDeclaredType);
}

// The relation a pending stream is blocked on is an `::rpc::` type, and it is
// not the reply it travels inside. Both open with data a CDR reader will
// happily consume as the other's, so the annotation is again the only
// separator -- and a client that reads one blocking relation off a key
// expecting a whole reply gets a name it will then push a calibration to.
TCN_SQE_TEST(a_blocking_relation_is_not_the_reply_that_carries_it)
{
    CHECK_OK(check_declared_type<rpc::SISBlockingRelation>(
        "application/cdr;tcnart_msgs::rpc::SISBlockingRelation"));

    CHECK_ERR(check_declared_type<rpc::SISBlockingRelation>(
                  "application/cdr;tcnart_msgs::rpc::SISRelationStreamReply"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISRelationStreamReply>(
                  "application/cdr;tcnart_msgs::rpc::SISBlockingRelation"),
              Error::WrongDeclaredType);

    // It is an rpc type: it names frames the way a relation-stream request
    // does, not the way the graph's own `::msg::` declarations do.
    CHECK_ERR(check_declared_type<rpc::SISBlockingRelation>(
                  "application/cdr;tcnart_msgs::msg::SISBlockingRelation"),
              Error::WrongDeclaredType);
}

// The policy is a field of the start request, not the request, and not the
// status enum it sits two fields away from in the same IDL module. All three
// are `::rpc::` names opening with an enum, so a CDR reader will consume any of
// them as any other and the annotation is the only separator. A client that
// read a bare policy off a key expecting a whole request would start a stream
// for two frames it invented.
TCN_SQE_TEST(the_reoptimise_policy_is_not_the_request_that_carries_it)
{
    CHECK_OK(check_declared_type<rpc::SISReoptimisePolicy>(
        "application/cdr;tcnart_msgs::rpc::SISReoptimisePolicy"));

    CHECK_ERR(check_declared_type<rpc::SISReoptimisePolicy>(
                  "application/cdr;tcnart_msgs::rpc::SISRelationStreamStartRequest"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISRelationStreamStartRequest>(
                  "application/cdr;tcnart_msgs::rpc::SISReoptimisePolicy"),
              Error::WrongDeclaredType);

    // And it is not the other enum on the same conversation.
    CHECK_ERR(check_declared_type<rpc::SISReoptimisePolicy>(
                  "application/cdr;tcnart_msgs::rpc::SISRelationStreamStatus"),
              Error::WrongDeclaredType);
}

// A view is a `::msg::` graph type, not a `::rpc::` one, and the namespace is
// on the wire. A peer that guessed `tcnart_msgs::rpc::SISNodeView` -- the name
// the design prose used before this rule was applied -- must be refused rather
// than accepted on a shape match.
TCN_SQE_TEST(the_view_types_are_msg_types_and_say_so)
{
    CHECK_OK(check_declared_type<msg::SRNodeView>(
        "application/cdr;tcnart_msgs::msg::SRNodeView"));
    CHECK_ERR(check_declared_type<msg::SRNodeView>(
                  "application/cdr;tcnart_msgs::rpc::SISNodeView"),
              Error::WrongDeclaredType);

    // And a view is not the declaration it embeds.
    CHECK_ERR(check_declared_type<msg::SRNodeView>(
                  "application/cdr;tcnart_msgs::msg::SRNode"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<msg::SREdge>(
                  "application/cdr;tcnart_msgs::msg::SREdgeView"),
              Error::WrongDeclaredType);
}

// The engine matches the type name between the first and second `;`, and
// reads `;key=value` parameters after it. A space after a separator would
// become part of the declared name on some readers, so the exact byte sequence
// is asserted rather than the shape.
TCN_SQE_TEST(there_are_exactly_two_separators_and_no_space_after_either)
{
    const std::string e = cdr_encoding<rpc::SISJoinRequest>();
    const std::string::size_type at = e.find(';');
    CHECK(at != std::string::npos);
    if (at == std::string::npos) { return; }
    const std::string::size_type at2 = e.find(';', at + 1);
    CHECK(at2 != std::string::npos);
    if (at2 == std::string::npos) { return; }
    CHECK_MSG(e.find(';', at2 + 1) == std::string::npos, "exactly two separators");
    CHECK_MSG(e[at + 1] != ' ' && e[at2 + 1] != ' ', "no space may follow a separator");
    CHECK_STR_EQ(e.substr(0, at), "application/cdr");
    CHECK_STR_EQ(e.substr(at + 1, at2 - at - 1), "tcnart_msgs::rpc::SISJoinRequest");
    CHECK_STR_EQ(e.substr(at2 + 1), "tcn_schema=0.9");
    CHECK(kEncodingSeparator == ';');
    CHECK_STR_EQ(std::string(kWireSchemaVersion), "0.9");
}

TCN_SQE_TEST(json_encoding_uses_the_same_type_name)
{
    CHECK_STR_EQ(json_encoding<rpc::SISJoinRequest>(),
                 "application/json;tcnart_msgs::rpc::SISJoinRequest;tcn_schema=0.9");
    const std::string cdr = cdr_encoding<rpc::SISJoinRequest>();
    const std::string json = json_encoding<rpc::SISJoinRequest>();
    const std::string::size_type c_at = cdr.find(';');
    const std::string::size_type j_at = json.find(';');
    CHECK(c_at != std::string::npos && j_at != std::string::npos);
    if (c_at == std::string::npos || j_at == std::string::npos) { return; }
    CHECK_STR_EQ(cdr.substr(c_at), json.substr(j_at));
}

TCN_SQE_TEST(a_payload_must_declare_the_type_that_is_being_read)
{
    CHECK_OK(check_declared_type<rpc::SISRelationStreamReply>(
        "application/cdr;tcnart_msgs::rpc::SISRelationStreamReply"));

    // The SISJoinMessage-class bug on the read side: CDR carrying something
    // else entirely, which decodes without complaint and means nothing.
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>(
                  "application/cdr;tcnart_msgs::rpc::SISJoinMessage"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>(
                  "application/cdr;tcnart_msgs::rpc::SISLeaveRequest"),
              Error::WrongDeclaredType);

    // A bare `application/cdr` is what both C++ repositories publish today.
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("application/cdr"),
              Error::MissingDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("application/cdr;"),
              Error::MissingDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("application/cdr;   "),
              Error::MissingDeclaredType);

    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("text/plain;tcnart_msgs::rpc::SISJoinRequest"),
              Error::UnknownEncoding);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>(""), Error::UnknownEncoding);
}

// The engine lower-cases the media type and trims the schema. Matching that
// is what stops a peer being refused for a difference the engine tolerates.
TCN_SQE_TEST(the_media_type_is_matched_case_insensitively_and_the_schema_trimmed)
{
    CHECK_OK(check_declared_type<rpc::SISJoinRequest>(
        "APPLICATION/CDR;tcnart_msgs::rpc::SISJoinRequest"));
    CHECK_OK(check_declared_type<rpc::SISJoinRequest>(
        "application/cdr; tcnart_msgs::rpc::SISJoinRequest "));
    CHECK_OK(check_declared_type<rpc::SISJoinRequest>(
        "application/json;tcnart_msgs::rpc::SISJoinRequest"));
}

// A type name containing "json" must not be mistaken for a JSON media type:
// only the half before the `;` is examined.
TCN_SQE_TEST(only_the_media_half_decides_the_format)
{
    const WireEncoding e = parse_encoding("application/cdr;some::json::Type");
    CHECK_STR_EQ(std::string(e.media), "application/cdr");
    CHECK_STR_EQ(std::string(e.type_name), "some::json::Type");
    CHECK(e.has_type_name);

    const WireEncoding bare = parse_encoding("application/cdr");
    CHECK(!bare.has_type_name);
    CHECK_STR_EQ(std::string(bare.media), "application/cdr");
}

// ---------------------------------------------------------------------------
// B76: the schema version on the wire
// ---------------------------------------------------------------------------

// What this library produces, it must accept. A round trip through
// `check_declared_type` of every encoding it writes is the minimum.
TCN_SQE_TEST(an_annotation_this_library_writes_is_one_it_accepts)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(cdr_encoding<rpc::SISMutationReply>()));
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(json_encoding<rpc::SISMutationReply>()));
    CHECK_OK(check_declared_type<rpc::SISTargetChange>(cdr_encoding<rpc::SISTargetChange>()));

    const std::string e = cdr_encoding<rpc::SISMutationReply>();
    const WireEncoding parsed = parse_encoding(e);
    CHECK_STR_EQ(std::string(parsed.type_name), "tcnart_msgs::rpc::SISMutationReply");
    CHECK(parsed.has_schema_version);
    CHECK_STR_EQ(std::string(parsed.schema_version), "0.9");
}

// SISMutationReply gained two trailing fields in 0.9.0 under the same name.
// A peer on another minor may lay out the same name differently, and CDR
// decodes either way without complaint -- the version is the only signal.
TCN_SQE_TEST(a_different_major_minor_is_refused)
{
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.8"),
              Error::SchemaVersionMismatch);
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.10"),
              Error::SchemaVersionMismatch);
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=1.9"),
              Error::SchemaVersionMismatch);
    // A prefix is not a match: 0.90 is not 0.9.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.90"),
              Error::SchemaVersionMismatch);

    // The type is checked first: a wrong type with a wrong version is a wrong
    // type, because that is the more fundamental refusal.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISGraphQueryReply;tcn_schema=0.8"),
              Error::WrongDeclaredType);
}

// Only major.minor is compared. A patch changes no layout.
TCN_SQE_TEST(a_patch_level_is_accepted_and_ignored)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.9.0"));
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.9.17"));
}

// Every pre-0.9 peer omits the parameter, and refusing them would make 0.9.0 a
// flag day. Accepted -- and visible to a caller that wants to warn.
TCN_SQE_TEST(an_absent_version_is_accepted_and_reported_as_absent)
{
    const char* old_peer = "application/cdr;tcnart_msgs::rpc::SISMutationReply";
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(old_peer));
    const WireEncoding parsed = parse_encoding(old_peer);
    CHECK(!parsed.has_schema_version);
    CHECK(parsed.schema_version.empty());
    CHECK_STR_EQ(std::string(parsed.type_name), "tcnart_msgs::rpc::SISMutationReply");
}

// A later release may add parameters. A reader that refused unknown ones would
// turn every such addition into a break, so they are skipped -- before and
// after the version, and including one with no `=` at all.
TCN_SQE_TEST(unknown_parameters_are_ignored)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;foo=bar;tcn_schema=0.9;x=1"));
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;flag;tcn_schema=0.9"));
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;foo=bar"));
    // ... but an unknown parameter does not hide a bad version.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;tcnart_msgs::rpc::SISMutationReply;foo=bar;tcn_schema=0.8"),
              Error::SchemaVersionMismatch);
    // The key is matched exactly: `TCN_SCHEMA` is some other parameter.
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;TCN_SCHEMA=0.8"));
}

// The engine trims every segment; so does this reader, key and value apart.
TCN_SQE_TEST(whitespace_around_every_segment_is_trimmed)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr; tcnart_msgs::rpc::SISMutationReply ; tcn_schema = 0.9 "));
    const WireEncoding parsed =
        parse_encoding("application/cdr; tcnart_msgs::rpc::SISMutationReply ; tcn_schema = 0.9 ");
    CHECK_STR_EQ(std::string(parsed.type_name), "tcnart_msgs::rpc::SISMutationReply");
    CHECK_STR_EQ(std::string(parsed.schema_version), "0.9");
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr; tcnart_msgs::rpc::SISMutationReply ;tcn_schema= 0.8"),
              Error::SchemaVersionMismatch);
}

// A peer that wrote the parameter meant to say something. If it is not a
// version, this reader cannot vouch for the layout, so it is a mismatch --
// never silently treated as absent.
TCN_SQE_TEST(a_malformed_version_is_a_mismatch_not_an_absence)
{
    const char* const bad[] = {
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=abc",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=.9",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.9.",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.9.1.2",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.9a",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=v0.9",
        "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0..9",
    };
    for (const char* e : bad)
    {
        CHECK_ERR(check_declared_type<rpc::SISMutationReply>(e), Error::SchemaVersionMismatch);
        CHECK(parse_encoding(e).has_schema_version);
    }
}

// The type name is the segment between the first and second `;` -- a 0.9.0
// annotation must not be read as the type `T;tcn_schema=0.9`, which is exactly
// what a pre-0.9 reader of this library did.
TCN_SQE_TEST(the_type_name_stops_at_the_second_separator)
{
    const WireEncoding e = parse_encoding("application/cdr;a::B;tcn_schema=0.9");
    CHECK_STR_EQ(std::string(e.media), "application/cdr");
    CHECK_STR_EQ(std::string(e.type_name), "a::B");
    CHECK(e.has_type_name);

    // No type, a version: still no type, which is refused.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>("application/cdr;;tcn_schema=0.9"),
              Error::MissingDeclaredType);
}

TCN_SQE_TEST(schema_version_mismatch_has_a_description)
{
    CHECK_STR_EQ(describe(Error::SchemaVersionMismatch),
                 "encoding declares another tcn_schema version");
}

// The pattern reply reuses SISGraphQueryStatus and so opens with the same enum
// as the graph reply; the annotation is the only separator.
TCN_SQE_TEST(a_pattern_reply_is_not_a_graph_reply)
{
    CHECK_OK(check_declared_type<rpc::SISPatternQueryReply>(cdr_encoding<rpc::SISPatternQueryReply>()));
    CHECK_ERR(check_declared_type<rpc::SISPatternQueryReply>(cdr_encoding<rpc::SISGraphQueryReply>()),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISGraphQueryReply>(cdr_encoding<rpc::SISPatternQueryReply>()),
              Error::WrongDeclaredType);
    // And the registry write is answered with the common reply, not its own.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(cdr_encoding<rpc::SISTargetQueryReply>()),
              Error::WrongDeclaredType);
}
