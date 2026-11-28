// SPDX-License-Identifier: internal
//
// The `application/cdr;{TYPE};tcn_schema={M.m}` annotation.
#include <string>

#include "check.h"
#include "tcn/sqe/wire_type.h"

using namespace tcn::sqe;
namespace msg = tcnart_msgs::msg;
namespace smsg = sqe_msgs::msg;
namespace rpc = sqe_msgs::rpc;
namespace tmsg = tcn_types::msg;

TCN_SQE_TEST(cdr_encoding_names_every_registered_type_exactly)
{
    CHECK_STR_EQ(cdr_encoding<msg::Pose6DMessage>(),          "application/cdr;tcnart_msgs::msg::Pose6DMessage;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<msg::TargetTrackingMessage>(),  "application/cdr;tcnart_msgs::msg::TargetTrackingMessage;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<msg::StreamDescriptorMessage>(),"application/cdr;tcnart_msgs::msg::StreamDescriptorMessage;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<smsg::SRGraph>(),                "application/cdr;sqe_msgs::msg::SRGraph;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<smsg::SRNode>(),                 "application/cdr;sqe_msgs::msg::SRNode;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<smsg::SREdge>(),                 "application/cdr;sqe_msgs::msg::SREdge;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<smsg::SRNodeView>(),             "application/cdr;sqe_msgs::msg::SRNodeView;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<smsg::SREdgeView>(),             "application/cdr;sqe_msgs::msg::SREdgeView;tcn_schema=0.11");

    CHECK_STR_EQ(cdr_encoding<rpc::SISJoinRequest>(),         "application/cdr;sqe_msgs::rpc::SISJoinRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISLeaveRequest>(),        "application/cdr;sqe_msgs::rpc::SISLeaveRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISNodeUpdateRequest>(),   "application/cdr;sqe_msgs::rpc::SISNodeUpdateRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISNodeRemoveRequest>(),   "application/cdr;sqe_msgs::rpc::SISNodeRemoveRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEdgeUpdateRequest>(),   "application/cdr;sqe_msgs::rpc::SISEdgeUpdateRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEdgeRemoveRequest>(),   "application/cdr;sqe_msgs::rpc::SISEdgeRemoveRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStartRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISRelationStreamStartRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStopRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISRelationStreamStopRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISStreamHandle>(),
                 "application/cdr;sqe_msgs::rpc::SISStreamHandle;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISReoptimisePolicy>(),
                 "application/cdr;sqe_msgs::rpc::SISReoptimisePolicy;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISBlockingRelation>(),
                 "application/cdr;sqe_msgs::rpc::SISBlockingRelation;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamReply>(),
                 "application/cdr;sqe_msgs::rpc::SISRelationStreamReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStatusNotification>(),
                 "application/cdr;sqe_msgs::rpc::SISRelationStreamStatusNotification;tcn_schema=0.11");
    // 0.11 (#35 H1-H3, R6): the data topic carries SISRelationOutput (no more
    // TargetTrackingMessage + attachment), and plans have their own notification.
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationOutput>(),
                 "application/cdr;sqe_msgs::rpc::SISRelationOutput;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISPlanNotification>(),
                 "application/cdr;sqe_msgs::rpc::SISPlanNotification;tcn_schema=0.11");

    // 0.11 (#35 R1-R5): one header, one status space, the join's own reply and
    // the unscoped engine summary, both carrying the capabilities.
    CHECK_STR_EQ(cdr_encoding<rpc::SISReplyHeader>(),
                 "application/cdr;sqe_msgs::rpc::SISReplyHeader;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISStatus>(),
                 "application/cdr;sqe_msgs::rpc::SISStatus;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISJoinReply>(),
                 "application/cdr;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEngineSummaryReply>(),
                 "application/cdr;sqe_msgs::rpc::SISEngineSummaryReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEngineCapabilities>(),
                 "application/cdr;sqe_msgs::rpc::SISEngineCapabilities;tcn_schema=0.11");
    // The frame reference moved to tcn_types (#35 N2a): the namespace is on the wire.
    CHECK_STR_EQ(cdr_encoding<tmsg::SRFrameRef>(),
                 "application/cdr;tcn_types::msg::SRFrameRef;tcn_schema=0.11");

    CHECK_STR_EQ(cdr_encoding<rpc::SISMutationReply>(),
                 "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryKind>(),
                 "application/cdr;sqe_msgs::rpc::SISGraphQueryKind;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISGraphQueryRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryReply>(),
                 "application/cdr;sqe_msgs::rpc::SISGraphQueryReply;tcn_schema=0.11");

    // 0.9.0: the pattern read (B63) and the target registry (B78).
    CHECK_STR_EQ(cdr_encoding<rpc::SRPatternExpansion>(),
                 "application/cdr;sqe_msgs::rpc::SRPatternExpansion;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISPatternQueryRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISPatternQueryRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISPatternQueryReply>(),
                 "application/cdr;sqe_msgs::rpc::SISPatternQueryReply;tcn_schema=0.11");
    // Moved to tcn_types in 0.11 (it describes a tracker target, not an RPC).
    CHECK_STR_EQ(cdr_encoding<tmsg::SISTargetKind>(),
                 "application/cdr;tcn_types::msg::SISTargetKind;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetAlias>(),
                 "application/cdr;sqe_msgs::rpc::SISTargetAlias;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGlobalTarget>(),
                 "application/cdr;sqe_msgs::rpc::SISGlobalTarget;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetQueryKind>(),
                 "application/cdr;sqe_msgs::rpc::SISTargetQueryKind;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetQueryRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISTargetQueryRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetQueryReply>(),
                 "application/cdr;sqe_msgs::rpc::SISTargetQueryReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetMutationKind>(),
                 "application/cdr;sqe_msgs::rpc::SISTargetMutationKind;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetMutationRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISTargetMutationRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTargetChange>(),
                 "application/cdr;sqe_msgs::rpc::SISTargetChange;tcn_schema=0.11");
    // 0.9.2: the operator surface (B82). Same major.minor, so the same
    // annotation: the release adds types and changes none.
    CHECK_STR_EQ(cdr_encoding<rpc::SISEngineInfoReply>(),
                 "application/cdr;sqe_msgs::rpc::SISEngineInfoReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISFragmentListReply>(),
                 "application/cdr;sqe_msgs::rpc::SISFragmentListReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISOperatorGraphQueryRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISOperatorGraphQueryRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphChangeNotification>(),
                 "application/cdr;sqe_msgs::rpc::SISGraphChangeNotification;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISOperatorMutationRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISOperatorMutationRequest;tcn_schema=0.11");
    // 0.9.3: explain (B89). Every break moves the annotation (0.10.0 B90; 0.11.0, XCDR2).
    CHECK_STR_EQ(cdr_encoding<rpc::SISExplainRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISExplainRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISExplainReply>(),
                 "application/cdr;sqe_msgs::rpc::SISExplainReply;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTranslationPlan>(),
                 "application/cdr;sqe_msgs::rpc::SISTranslationPlan;tcn_schema=0.11");
    // 0.10.1: the transaction log (B92). Additive, so the annotation is unchanged.
    CHECK_STR_EQ(cdr_encoding<rpc::SISTransactionLogRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISTransactionLogRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISTransactionLogReply>(),
                 "application/cdr;sqe_msgs::rpc::SISTransactionLogReply;tcn_schema=0.11");
    // 0.10.6: the relation sample (B93). Additive, so the annotation is unchanged.
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationSampleRequest>(),
                 "application/cdr;sqe_msgs::rpc::SISRelationSampleRequest;tcn_schema=0.11");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationSampleReply>(),
                 "application/cdr;sqe_msgs::rpc::SISRelationSampleReply;tcn_schema=0.11");
}

// The two replies are answers to different questions on different keys, and a
// reader of one must refuse the other. Nothing about their CDR layouts stops a
// SISMutationReply decoding as a SISGraphQueryReply -- both open with
// SISReplyHeader -- so the annotation is the only thing that separates them.
TCN_SQE_TEST(the_two_new_replies_do_not_answer_for_each_other)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.11"));
    CHECK_OK(check_declared_type<rpc::SISGraphQueryReply>(
        "application/cdr;sqe_msgs::rpc::SISGraphQueryReply;tcn_schema=0.11"));

    CHECK_ERR(check_declared_type<rpc::SISGraphQueryReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.11"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISGraphQueryReply;tcn_schema=0.11"),
              Error::WrongDeclaredType);

    // /sis/relation/update is a queryable: the `get` carries a request and is
    // answered with something else entirely, on one key. Reading the reply as
    // the request is the mistake that shape invites.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISEdgeUpdateRequest;tcn_schema=0.11"),
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
        "application/cdr;sqe_msgs::rpc::SISBlockingRelation;tcn_schema=0.11"));

    CHECK_ERR(check_declared_type<rpc::SISBlockingRelation>(
                  "application/cdr;sqe_msgs::rpc::SISRelationStreamReply;tcn_schema=0.11"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISRelationStreamReply>(
                  "application/cdr;sqe_msgs::rpc::SISBlockingRelation;tcn_schema=0.11"),
              Error::WrongDeclaredType);

    // It is an rpc type: it names frames the way a relation-stream request
    // does, not the way the graph's own `::msg::` declarations do.
    CHECK_ERR(check_declared_type<rpc::SISBlockingRelation>(
                  "application/cdr;tcnart_msgs::msg::SISBlockingRelation;tcn_schema=0.11"),
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
        "application/cdr;sqe_msgs::rpc::SISReoptimisePolicy;tcn_schema=0.11"));

    CHECK_ERR(check_declared_type<rpc::SISReoptimisePolicy>(
                  "application/cdr;sqe_msgs::rpc::SISRelationStreamStartRequest;tcn_schema=0.11"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISRelationStreamStartRequest>(
                  "application/cdr;sqe_msgs::rpc::SISReoptimisePolicy;tcn_schema=0.11"),
              Error::WrongDeclaredType);

    // And it is not the other enum on the same conversation.
    CHECK_ERR(check_declared_type<rpc::SISReoptimisePolicy>(
                  "application/cdr;sqe_msgs::rpc::SISStatus;tcn_schema=0.11"),
              Error::WrongDeclaredType);
}

// A view is a `::msg::` graph type, not a `::rpc::` one, and the namespace is
// on the wire. A peer that guessed `sqe_msgs::rpc::SISNodeView` -- the name
// the design prose used before this rule was applied -- must be refused rather
// than accepted on a shape match.
TCN_SQE_TEST(the_view_types_are_msg_types_and_say_so)
{
    CHECK_OK(check_declared_type<smsg::SRNodeView>(
        "application/cdr;sqe_msgs::msg::SRNodeView;tcn_schema=0.11"));
    CHECK_ERR(check_declared_type<smsg::SRNodeView>(
                  "application/cdr;sqe_msgs::rpc::SISNodeView;tcn_schema=0.11"),
              Error::WrongDeclaredType);

    // And a view is not the declaration it embeds.
    CHECK_ERR(check_declared_type<smsg::SRNodeView>(
                  "application/cdr;sqe_msgs::msg::SRNode;tcn_schema=0.11"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<smsg::SREdge>(
                  "application/cdr;sqe_msgs::msg::SREdgeView;tcn_schema=0.11"),
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
    CHECK_STR_EQ(e.substr(at + 1, at2 - at - 1), "sqe_msgs::rpc::SISJoinRequest");
    CHECK_STR_EQ(e.substr(at2 + 1), "tcn_schema=0.11");
    CHECK(kEncodingSeparator == ';');
    CHECK_STR_EQ(std::string(kWireSchemaVersion), "0.11");
}

TCN_SQE_TEST(json_encoding_uses_the_same_type_name)
{
    CHECK_STR_EQ(json_encoding<rpc::SISJoinRequest>(),
                 "application/json;sqe_msgs::rpc::SISJoinRequest;tcn_schema=0.11");
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
        "application/cdr;sqe_msgs::rpc::SISRelationStreamReply;tcn_schema=0.11"));

    // The SISJoinMessage-class bug on the read side: CDR carrying something
    // else entirely, which decodes without complaint and means nothing.
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>(
                  "application/cdr;sqe_msgs::rpc::SISJoinMessage;tcn_schema=0.11"),
              Error::WrongDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>(
                  "application/cdr;sqe_msgs::rpc::SISLeaveRequest;tcn_schema=0.11"),
              Error::WrongDeclaredType);

    // A bare `application/cdr` is what both C++ repositories publish today.
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("application/cdr"),
              Error::MissingDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("application/cdr;"),
              Error::MissingDeclaredType);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("application/cdr;   "),
              Error::MissingDeclaredType);

    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>("text/plain;sqe_msgs::rpc::SISJoinRequest"),
              Error::UnknownEncoding);
    CHECK_ERR(check_declared_type<rpc::SISJoinRequest>(""), Error::UnknownEncoding);
}

// V1 (§6): the media type is exactly `application/cdr` or `application/json`.
// Not a substring -- `application/x-cdr` or `text/cdr` named something this
// library does not decode -- and not case-folded. The schema is still trimmed.
TCN_SQE_TEST(the_media_type_is_matched_exactly_and_the_schema_trimmed)
{
    CHECK_OK(check_declared_type<rpc::SISJoinRequest>(
        "application/cdr; sqe_msgs::rpc::SISJoinRequest ;tcn_schema=0.11"));
    CHECK_OK(check_declared_type<rpc::SISJoinRequest>(
        "application/json;sqe_msgs::rpc::SISJoinRequest;tcn_schema=0.11"));

    const char* const not_ours[] = {
        "application/x-cdr;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11",
        "APPLICATION/CDR;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11",
        "application/cdr2;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11",
        "text/cdr;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11",
        "application/jsonl;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11",
        " application/cdr;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11",
    };
    for (const char* e : not_ours)
    {
        CHECK_ERR(check_declared_type<rpc::SISJoinReply>(e), Error::UnknownEncoding);
    }
}

// V1 (§6): `application/cdr;{TYPE};tcn_schema=0.11` is mandatory. The full
// form passes; without the version it is refused, so an unversioned payload
// -- every pre-0.9 peer, and anything that forgot -- is never decoded.
TCN_SQE_TEST(the_full_annotation_is_mandatory)
{
    CHECK_OK(check_declared_type<rpc::SISJoinReply>(
        "application/cdr;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11"));
    CHECK_ERR(check_declared_type<rpc::SISJoinReply>("application/cdr;sqe_msgs::rpc::SISJoinReply"),
              Error::MissingSchemaVersion);
    CHECK_ERR(check_declared_type<rpc::SISJoinReply>(
                  "application/x-cdr;sqe_msgs::rpc::SISJoinReply;tcn_schema=0.11"),
              Error::UnknownEncoding);
    CHECK_STR_EQ(code(Error::MissingSchemaVersion), "missing_schema_version");
}

// A type name containing "json" must not be mistaken for a JSON media type:
// only the half before the `;` is examined.
TCN_SQE_TEST(only_the_media_half_decides_the_format)
{
    const WireEncoding e = parse_encoding("application/cdr;some::json::Type;tcn_schema=0.11");
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
    CHECK_STR_EQ(std::string(parsed.type_name), "sqe_msgs::rpc::SISMutationReply");
    CHECK(parsed.has_schema_version);
    CHECK_STR_EQ(std::string(parsed.schema_version), "0.11");
}

// SISMutationReply gained two trailing fields in 0.9.0 under the same name.
// A peer on another minor may lay out the same name differently, and CDR
// decodes either way without complaint -- the version is the only signal.
TCN_SQE_TEST(a_different_major_minor_is_refused)
{
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.8"),
              Error::SchemaVersionMismatch);
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.9"),
              Error::SchemaVersionMismatch);
    // 0.11 changed the encoding (XCDR1 -> XCDR2): a 0.10 payload is refused, not misread.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.10"),
              Error::SchemaVersionMismatch);
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=1.9"),
              Error::SchemaVersionMismatch);
    // A prefix is not a match: 0.110 is not 0.11, nor 0.1 0.11.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.110"),
              Error::SchemaVersionMismatch);
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.1"),
              Error::SchemaVersionMismatch);

    // The type is checked first: a wrong type with a wrong version is a wrong
    // type, because that is the more fundamental refusal.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISGraphQueryReply;tcn_schema=0.8"),
              Error::WrongDeclaredType);
}

// Only major.minor is compared. A patch changes no layout.
TCN_SQE_TEST(a_patch_level_is_accepted_and_ignored)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.11.0"));
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.11.17"));
}

// 0.9 accepted an absent version so that 0.9.0 was not a flag day. 0.11 is a
// break anyway (XCDR2), so an absent version is refused (V1, §6) -- and still
// reported as absent by the parser, distinct from a malformed one.
TCN_SQE_TEST(an_absent_version_is_refused_and_reported_as_absent)
{
    const char* old_peer = "application/cdr;sqe_msgs::rpc::SISMutationReply";
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(old_peer), Error::MissingSchemaVersion);
    const WireEncoding parsed = parse_encoding(old_peer);
    CHECK(!parsed.has_schema_version);
    CHECK(parsed.schema_version.empty());
    CHECK_STR_EQ(std::string(parsed.type_name), "sqe_msgs::rpc::SISMutationReply");
}

// A later release may add parameters. A reader that refused unknown ones would
// turn every such addition into a break, so they are skipped -- before and
// after the version, and including one with no `=` at all.
TCN_SQE_TEST(unknown_parameters_are_ignored)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;sqe_msgs::rpc::SISMutationReply;foo=bar;tcn_schema=0.11;x=1"));
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr;sqe_msgs::rpc::SISMutationReply;flag;tcn_schema=0.11"));
    // ... an unknown parameter is not a version ...
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;foo=bar"),
              Error::MissingSchemaVersion);
    // ... and does not hide a bad version.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;foo=bar;tcn_schema=0.8"),
              Error::SchemaVersionMismatch);
    // The key is matched exactly: `TCN_SCHEMA` is some other parameter, so
    // this annotation carries no version at all.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr;sqe_msgs::rpc::SISMutationReply;TCN_SCHEMA=0.11"),
              Error::MissingSchemaVersion);
}

// The engine trims every segment; so does this reader, key and value apart.
TCN_SQE_TEST(whitespace_around_every_segment_is_trimmed)
{
    CHECK_OK(check_declared_type<rpc::SISMutationReply>(
        "application/cdr; sqe_msgs::rpc::SISMutationReply ; tcn_schema = 0.11 "));
    const WireEncoding parsed =
        parse_encoding("application/cdr; sqe_msgs::rpc::SISMutationReply ; tcn_schema = 0.11 ");
    CHECK_STR_EQ(std::string(parsed.type_name), "sqe_msgs::rpc::SISMutationReply");
    CHECK_STR_EQ(std::string(parsed.schema_version), "0.11");
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>(
                  "application/cdr; sqe_msgs::rpc::SISMutationReply ;tcn_schema= 0.8"),
              Error::SchemaVersionMismatch);
}

// A peer that wrote the parameter meant to say something. If it is not a
// version, this reader cannot vouch for the layout, so it is a mismatch --
// never silently treated as absent.
TCN_SQE_TEST(a_malformed_version_is_a_mismatch_not_an_absence)
{
    const char* const bad[] = {
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=abc",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=.10",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.10.",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.10.1.2",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0.10a",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=v0.10",
        "application/cdr;sqe_msgs::rpc::SISMutationReply;tcn_schema=0..10",
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
    const WireEncoding e = parse_encoding("application/cdr;a::B;tcn_schema=0.11");
    CHECK_STR_EQ(std::string(e.media), "application/cdr");
    CHECK_STR_EQ(std::string(e.type_name), "a::B");
    CHECK(e.has_type_name);

    // No type, a version: still no type, which is refused.
    CHECK_ERR(check_declared_type<rpc::SISMutationReply>("application/cdr;;tcn_schema=0.11"),
              Error::MissingDeclaredType);
}

TCN_SQE_TEST(schema_version_mismatch_has_a_description)
{
    CHECK_STR_EQ(describe(Error::SchemaVersionMismatch),
                 "encoding declares another tcn_schema version");
}

// The pattern reply and the graph reply both open with SISReplyHeader; the
// annotation is the only separator.
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
