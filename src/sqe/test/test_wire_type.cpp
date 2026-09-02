// SPDX-License-Identifier: internal
//
// The `application/cdr;{TYPE}` annotation.
#include <string>

#include "check.h"
#include "tcn/sqe/wire_type.h"

using namespace tcn::sqe;
namespace msg = tcnart_msgs::msg;
namespace rpc = tcnart_msgs::rpc;

TCN_SQE_TEST(cdr_encoding_names_every_registered_type_exactly)
{
    CHECK_STR_EQ(cdr_encoding<msg::Pose6DMessage>(),          "application/cdr;tcnart_msgs::msg::Pose6DMessage");
    CHECK_STR_EQ(cdr_encoding<msg::Pose6DMapMessage>(),       "application/cdr;tcnart_msgs::msg::Pose6DMapMessage");
    CHECK_STR_EQ(cdr_encoding<msg::TargetTrackingMessage>(),  "application/cdr;tcnart_msgs::msg::TargetTrackingMessage");
    CHECK_STR_EQ(cdr_encoding<msg::StreamDescriptorMessage>(),"application/cdr;tcnart_msgs::msg::StreamDescriptorMessage");
    CHECK_STR_EQ(cdr_encoding<msg::SRGraph>(),                "application/cdr;tcnart_msgs::msg::SRGraph");
    CHECK_STR_EQ(cdr_encoding<msg::SRNode>(),                 "application/cdr;tcnart_msgs::msg::SRNode");
    CHECK_STR_EQ(cdr_encoding<msg::SREdge>(),                 "application/cdr;tcnart_msgs::msg::SREdge");
    CHECK_STR_EQ(cdr_encoding<msg::SRNodeView>(),             "application/cdr;tcnart_msgs::msg::SRNodeView");
    CHECK_STR_EQ(cdr_encoding<msg::SREdgeView>(),             "application/cdr;tcnart_msgs::msg::SREdgeView");

    CHECK_STR_EQ(cdr_encoding<rpc::SISJoinRequest>(),         "application/cdr;tcnart_msgs::rpc::SISJoinRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISLeaveRequest>(),        "application/cdr;tcnart_msgs::rpc::SISLeaveRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISNodeUpdateRequest>(),   "application/cdr;tcnart_msgs::rpc::SISNodeUpdateRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISNodeRemoveRequest>(),   "application/cdr;tcnart_msgs::rpc::SISNodeRemoveRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEdgeUpdateRequest>(),   "application/cdr;tcnart_msgs::rpc::SISEdgeUpdateRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISEdgeRemoveRequest>(),   "application/cdr;tcnart_msgs::rpc::SISEdgeRemoveRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStartRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStartRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStopRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStopRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStatus>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStatus");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamHandle>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamHandle");
    CHECK_STR_EQ(cdr_encoding<rpc::SISReoptimisePolicy>(),
                 "application/cdr;tcnart_msgs::rpc::SISReoptimisePolicy");
    CHECK_STR_EQ(cdr_encoding<rpc::SISBlockingRelation>(),
                 "application/cdr;tcnart_msgs::rpc::SISBlockingRelation");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamReply");
    CHECK_STR_EQ(cdr_encoding<rpc::SISRelationStreamStatusNotification>(),
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStatusNotification");

    CHECK_STR_EQ(cdr_encoding<rpc::SISMutationStatus>(),
                 "application/cdr;tcnart_msgs::rpc::SISMutationStatus");
    CHECK_STR_EQ(cdr_encoding<rpc::SISMutationReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISMutationReply");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryKind>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryKind");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryStatus>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryStatus");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryRequest>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryRequest");
    CHECK_STR_EQ(cdr_encoding<rpc::SISGraphQueryReply>(),
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryReply");
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

// The engine matches the schema half after a `;`. A space after the separator
// would become part of the declared name on some readers, so the exact byte
// sequence is asserted rather than the shape.
TCN_SQE_TEST(there_is_exactly_one_separator_and_no_space_after_it)
{
    const std::string e = cdr_encoding<rpc::SISJoinRequest>();
    const std::string::size_type at = e.find(';');
    CHECK(at != std::string::npos);
    if (at == std::string::npos) { return; }
    CHECK_MSG(e.find(';', at + 1) == std::string::npos, "only one separator");
    CHECK_MSG(e[at + 1] != ' ', "no space may follow the separator");
    CHECK_STR_EQ(e.substr(0, at), "application/cdr");
    CHECK(kEncodingSeparator == ';');
}

TCN_SQE_TEST(json_encoding_uses_the_same_type_name)
{
    CHECK_STR_EQ(json_encoding<rpc::SISJoinRequest>(),
                 "application/json;tcnart_msgs::rpc::SISJoinRequest");
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
