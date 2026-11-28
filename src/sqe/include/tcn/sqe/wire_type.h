// SPDX-License-Identifier: internal
//
// WireType<T> — the `application/cdr;{TYPE};tcn_schema={M.m}` annotation,
// derived from the message type so it cannot be forgotten or misspelled.
//
// See docs/design/2026-08-31-sqe-contract-library-design.md §4.7.
#ifndef TCN_SQE_WIRE_TYPE_H
#define TCN_SQE_WIRE_TYPE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"

// ---------------------------------------------------------------------------
// Forward declarations of the generated types
// ---------------------------------------------------------------------------
//
// Declared, never defined here, and never included. A template specialisation
// only needs the type to be *declared*, so this header registers every wire
// type without dragging in a generated header — which keeps `sqe_contract`
// dependency-free and lets its tests build with no fastddsgen toolchain
// present. If a consumer includes the real headers, these declarations are
// redundant and harmless; if a generated type ever changes its class-key or an
// enum its underlying type, this fails to compile, loudly, which is the right
// outcome. fastddsgen emits every IDL enum as an unscoped `enum X : uint32_t`,
// so that is how each one is declared here.

namespace tcn_types {
namespace msg {

class SRFrameRef;

enum SISTargetKind : std::uint32_t;

}  // namespace msg
}  // namespace tcn_types

namespace tcnart_msgs {
namespace msg {

class Pose6DMessage;
class TargetTrackingMessage;
class StreamDescriptorMessage;

}  // namespace msg
}  // namespace tcnart_msgs

namespace sqe_msgs {
namespace msg {

class SRGraph;
class SRNode;
class SREdge;
class SRNodeView;
class SREdgeView;

}  // namespace msg

namespace rpc {

class SISJoinRequest;
class SISLeaveRequest;
class SISNodeUpdateRequest;
class SISNodeRemoveRequest;
class SISEdgeUpdateRequest;
class SISEdgeRemoveRequest;
class SISRelationStreamStartRequest;
class SISRelationStreamStopRequest;
class SISStreamHandle;
class SISBlockingRelation;
class SISRelationStreamReply;
class SISRelationStreamStatusNotification;
class SISRelationOutput;
class SISPlanNotification;
class SISReplyHeader;
class SISJoinReply;
class SISEngineCapabilities;
class SISEngineSummaryReply;
class SISMutationReply;
class SISGraphQueryRequest;
class SISGraphQueryReply;
class SISPatternQueryRequest;
class SISPatternQueryReply;
class SISTargetAlias;
class SISGlobalTarget;
class SISTargetQueryRequest;
class SISTargetQueryReply;
class SISTargetMutationRequest;
class SISTargetChange;
class SISOperatorRef;
class SISEngineInfoRequest;
class SISReadLimits;
class SISEngineInfoReply;
class SISFragmentListRequest;
class SISFragmentSummary;
class SISFragmentListReply;
class SISOperatorGraphQueryRequest;
class SISGraphChangeNotification;
class SISOperatorMutation;
class SISOperatorMutationRequest;
class SISPlanDynamicOp;
class SISPlanCoordSwapOp;
class SISPlanOp;
class SISPlanInstance;
class SISPlanInput;
class SISPlanHop;
class SISTranslationPlan;
class SISExplainRequest;
class SISPlanLive;
class SISExplainReply;
class SISTransactionLogRequest;
class SISTransactionLogReply;
class SISRelationSampleRequest;
class SISSampleInput;
class SISRelationSampleReply;

enum SISStatus : std::uint32_t;
enum SISReoptimisePolicy : std::uint32_t;
enum SISGraphQueryKind : std::uint32_t;
enum SRPatternExpansion : std::uint32_t;
enum SISTargetQueryKind : std::uint32_t;
enum SISTargetMutationKind : std::uint32_t;
enum SISStoreHealth : std::uint32_t;
enum SISGraphChangeKind : std::uint32_t;
enum SISOperatorMutationKind : std::uint32_t;
enum SISEvaluationMode : std::uint32_t;
enum SISTransactionLogAction : std::uint32_t;
enum SISRelationSampleInstant : std::uint32_t;
enum SISPlanOpKind : std::uint32_t;
enum SISPlanInputSource : std::uint32_t;
enum SISExplainKind : std::uint32_t;

}  // namespace rpc
}  // namespace sqe_msgs

namespace tcn {
namespace sqe {

// ---------------------------------------------------------------------------
// The encoding annotation
// ---------------------------------------------------------------------------

inline constexpr std::string_view kCdrMediaType = "application/cdr";
inline constexpr std::string_view kJsonMediaType = "application/json";

/// Separates the media type from the schema, and the schema from each
/// parameter after it. One character, no space after it.
inline constexpr char kEncodingSeparator = ';';

/// The `tcn_schema` release this library's wire types are, as `major.minor`
/// (B76).
///
/// Appended to every annotation this library produces as
/// `;tcn_schema=0.11`, and compared against the one a peer declares. Only
/// major.minor: a patch release of tcn_schema changes no wire layout by the
/// repository's own versioning rule, so a patch difference is not a reason to
/// refuse a payload, and a reader that compared it would refuse peers for a
/// difference that cannot matter.
///
/// It moves with `TCN_SCHEMA_VERSION`'s minor and nowhere else. Why a type
/// name was not enough: since 0.9.0 two trailing fields were appended to
/// SISMutationReply and one to StreamDescriptorMessage, under unchanged names.
/// CDR decodes a short payload into a long struct, or a long one into a short
/// struct, without an error on either side -- the name matched and the
/// meaning did not. Mandatory since 0.11 (V1, §6): `check_declared_type`
/// refuses an annotation without it.
inline constexpr std::string_view kWireSchemaVersion = "0.11";

/// The parameter that carries it. Matched exactly (after trimming): the
/// engine does the same, and a case-folded key would be a second spelling the
/// engine does not accept.
inline constexpr std::string_view kSchemaVersionParam = "tcn_schema";

/// A message type that names itself on the wire.
///
/// **There is no primary definition on purpose.** An unregistered type is a
/// compile error at the call site, which is where a name the engine refuses —
/// `SISJoinMessage`, say, which 0.3.0 deleted and which still appears in older
/// prose — should be caught. The alternative, a helper taking a
/// `std::string_view`, lets a caller pass a string that is merely wrong, and
/// before B73 the daemon did not answer mutations, so nothing would ever have
/// said so -- and a publish on a stream status key still is answered by
/// nobody.
template <class T>
struct WireType;

#define TCN_SQE_REGISTER_WIRE_TYPE(TYPE, NAME)             \
    template <>                                            \
    struct WireType<TYPE>                                  \
    {                                                      \
        static constexpr std::string_view name = NAME;     \
    }

// tcnart_msgs/msg/PoseTracking.idl
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::Pose6DMessage,          "tcnart_msgs::msg::Pose6DMessage");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::TargetTrackingMessage,  "tcnart_msgs::msg::TargetTrackingMessage");

// tcnart_msgs/msg/StreamDescriptor.idl
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::StreamDescriptorMessage, "tcnart_msgs::msg::StreamDescriptorMessage");

// tcn_types/msg/*.idl: the frame reference every request names frames with
// (#35 N2a), and the target kind, moved here from sqe_msgs::rpc in 0.11. The
// namespace is part of the wire name, so the move is a rename on the wire.
TCN_SQE_REGISTER_WIRE_TYPE(::tcn_types::msg::SRFrameRef,    "tcn_types::msg::SRFrameRef");
TCN_SQE_REGISTER_WIRE_TYPE(::tcn_types::msg::SISTargetKind, "tcn_types::msg::SISTargetKind");

// sqe_msgs/msg/Graph.idl
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::msg::SRGraph, "sqe_msgs::msg::SRGraph");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::msg::SRNode,  "sqe_msgs::msg::SRNode");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::msg::SREdge,  "sqe_msgs::msg::SREdge");
// The read side of the same file: a declaration qualified by the fragment that
// made it. `::msg::` and `SR*`, not `::rpc::` and `SIS*`, because these are
// graph types and the namespace decides the wire name a peer must match.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::msg::SRNodeView, "sqe_msgs::msg::SRNodeView");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::msg::SREdgeView, "sqe_msgs::msg::SREdgeView");

// sqe_msgs/rpc/Reply.idl (#35 R1-R3): the header every reply and notification
// opens with, and its status. Nested, never published alone, and registered
// for the reason SISBlockingRelation is: a tool that logs or forwards a header
// passes it around by itself. Every reply opens with the same header, so the
// annotation is the only thing that tells two replies apart.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISReplyHeader, "sqe_msgs::rpc::SISReplyHeader");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISStatus,      "sqe_msgs::rpc::SISStatus");

// sqe_msgs/rpc/Session.idl. The join is answered with its own reply since 0.11
// (#35 R5), and the unscoped `sqe/{engine}/info` (topics.h) with the engine
// summary; both carry SISEngineCapabilities (capabilities.h).
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISJoinRequest,                "sqe_msgs::rpc::SISJoinRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISJoinReply,                  "sqe_msgs::rpc::SISJoinReply");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISEngineCapabilities,         "sqe_msgs::rpc::SISEngineCapabilities");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISEngineSummaryReply,         "sqe_msgs::rpc::SISEngineSummaryReply");

// sqe_msgs/rpc/Mutations.idl and Streams.idl
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISLeaveRequest,               "sqe_msgs::rpc::SISLeaveRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISNodeUpdateRequest,          "sqe_msgs::rpc::SISNodeUpdateRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISNodeRemoveRequest,          "sqe_msgs::rpc::SISNodeRemoveRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISEdgeUpdateRequest,          "sqe_msgs::rpc::SISEdgeUpdateRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISEdgeRemoveRequest,          "sqe_msgs::rpc::SISEdgeRemoveRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationStreamStartRequest, "sqe_msgs::rpc::SISRelationStreamStartRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationStreamStopRequest,  "sqe_msgs::rpc::SISRelationStreamStopRequest");
// The four topics a start reply assigns (#35 H1, H2). Nested in the reply,
// registered because a client hands it around on its own.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISStreamHandle,               "sqe_msgs::rpc::SISStreamHandle");
// The re-optimisation policy a start request carries (B51). Nested inside that
// request rather than published alone, and registered anyway: it is the one
// term of the request that is *not* stream identity on the engine, a caller
// reasons about it on its own, and a name spelled by hand is a name that can
// be spelled wrong.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISReoptimisePolicy,           "sqe_msgs::rpc::SISReoptimisePolicy");
// The relation a pending stream is waiting on. Nested inside the reply and the
// notification below rather than published alone, and registered anyway for the
// same reason SRNodeView and SREdgeView are: a calibration tool passes one of
// these around on its own, and a name it has to spell by hand is a name it can
// spell wrong.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISBlockingRelation,           "sqe_msgs::rpc::SISBlockingRelation");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationStreamReply,        "sqe_msgs::rpc::SISRelationStreamReply");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationStreamStatusNotification,
                           "sqe_msgs::rpc::SISRelationStreamStatusNotification");
// What a stream publishes on its `data_topic` since 0.11 (#35 H3): one per
// evaluation, with no attachment. A subscriber still decoding the data topic
// as TargetTrackingMessage is refused by this name rather than misreading it.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationOutput,   "sqe_msgs::rpc::SISRelationOutput");
// What a LOCAL or BOTH stream publishes on its `plan_topic` (#35 R6).
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanNotification, "sqe_msgs::rpc::SISPlanNotification");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISMutationReply,    "sqe_msgs::rpc::SISMutationReply");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISGraphQueryKind,   "sqe_msgs::rpc::SISGraphQueryKind");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISGraphQueryRequest,
                           "sqe_msgs::rpc::SISGraphQueryRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISGraphQueryReply,
                           "sqe_msgs::rpc::SISGraphQueryReply");
// The pattern query (B63), on `/sis/pattern/query`. The engine answered it
// since B63 and this table could not name either half until 0.9.0 -- the same
// gap the graph read sat in before B46 was registered. The annotation is the
// only thing that separates a pattern reply from a graph reply: both open with
// SISReplyHeader.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SRPatternExpansion,     "sqe_msgs::rpc::SRPatternExpansion");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPatternQueryRequest, "sqe_msgs::rpc::SISPatternQueryRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPatternQueryReply,   "sqe_msgs::rpc::SISPatternQueryReply");
// The global target registry (B78). `SISTargetAlias` and `SISGlobalTarget` are
// nested, never published alone, and registered for the reason
// SISBlockingRelation is: a tool that seeds or exports the registry passes
// them around by themselves. `SISTargetChange` is published by the engine on
// `sqe/{engine_id}/targets/changed` (topics.h); the mutation is answered with
// the ordinary SISMutationReply, not a registry-specific reply. The target
// kind lives in tcn_types since 0.11 (registered above).
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTargetAlias,           "sqe_msgs::rpc::SISTargetAlias");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISGlobalTarget,          "sqe_msgs::rpc::SISGlobalTarget");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTargetQueryKind,       "sqe_msgs::rpc::SISTargetQueryKind");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTargetQueryRequest,    "sqe_msgs::rpc::SISTargetQueryRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTargetQueryReply,      "sqe_msgs::rpc::SISTargetQueryReply");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTargetMutationKind,    "sqe_msgs::rpc::SISTargetMutationKind");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTargetMutationRequest, "sqe_msgs::rpc::SISTargetMutationRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTargetChange,          "sqe_msgs::rpc::SISTargetChange");
// The operator surface (B82, 0.9.2): sqe/{engine_id}/op/* (topics.h). The
// graph read there answers the ordinary SISGraphQueryReply, the update the
// ordinary SISMutationReply.
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISOperatorRef,               "sqe_msgs::rpc::SISOperatorRef");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISEngineInfoRequest,         "sqe_msgs::rpc::SISEngineInfoRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISStoreHealth,               "sqe_msgs::rpc::SISStoreHealth");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISReadLimits,                "sqe_msgs::rpc::SISReadLimits");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISEngineInfoReply,           "sqe_msgs::rpc::SISEngineInfoReply");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISFragmentListRequest,       "sqe_msgs::rpc::SISFragmentListRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISFragmentSummary,           "sqe_msgs::rpc::SISFragmentSummary");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISFragmentListReply,         "sqe_msgs::rpc::SISFragmentListReply");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISOperatorGraphQueryRequest, "sqe_msgs::rpc::SISOperatorGraphQueryRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISGraphChangeKind,           "sqe_msgs::rpc::SISGraphChangeKind");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISGraphChangeNotification,   "sqe_msgs::rpc::SISGraphChangeNotification");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISOperatorMutationKind,      "sqe_msgs::rpc::SISOperatorMutationKind");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISOperatorMutation,          "sqe_msgs::rpc::SISOperatorMutation");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISOperatorMutationRequest,   "sqe_msgs::rpc::SISOperatorMutationRequest");
// Explain (B89, 0.9.3): sqe/{engine_id}/op/explain (topics.h).
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISEvaluationMode,           "sqe_msgs::rpc::SISEvaluationMode");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanOpKind,                "sqe_msgs::rpc::SISPlanOpKind");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanDynamicOp,             "sqe_msgs::rpc::SISPlanDynamicOp");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanCoordSwapOp,           "sqe_msgs::rpc::SISPlanCoordSwapOp");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanOp,                    "sqe_msgs::rpc::SISPlanOp");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanInstance,              "sqe_msgs::rpc::SISPlanInstance");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanInputSource,           "sqe_msgs::rpc::SISPlanInputSource");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanInput,                 "sqe_msgs::rpc::SISPlanInput");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanHop,                   "sqe_msgs::rpc::SISPlanHop");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTranslationPlan,           "sqe_msgs::rpc::SISTranslationPlan");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISExplainKind,               "sqe_msgs::rpc::SISExplainKind");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISExplainRequest,            "sqe_msgs::rpc::SISExplainRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISPlanLive,                  "sqe_msgs::rpc::SISPlanLive");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISExplainReply,              "sqe_msgs::rpc::SISExplainReply");
// Transaction log (B92, 0.10.1): sqe/{engine_id}/op/txlog (topics.h).
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTransactionLogAction,      "sqe_msgs::rpc::SISTransactionLogAction");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTransactionLogRequest,     "sqe_msgs::rpc::SISTransactionLogRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISTransactionLogReply,       "sqe_msgs::rpc::SISTransactionLogReply");
// Relation sample (B93, 0.10.6): {client}/sis/stream/sample since 0.11 (client_id.h, session.h).
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationSampleInstant,     "sqe_msgs::rpc::SISRelationSampleInstant");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationSampleRequest,     "sqe_msgs::rpc::SISRelationSampleRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISSampleInput,               "sqe_msgs::rpc::SISSampleInput");
TCN_SQE_REGISTER_WIRE_TYPE(::sqe_msgs::rpc::SISRelationSampleReply,       "sqe_msgs::rpc::SISRelationSampleReply");

namespace detail {

inline std::string join_encoding(std::string_view media, std::string_view type_name)
{
    std::string out;
    out.reserve(media.size() + type_name.size() + kSchemaVersionParam.size()
                + kWireSchemaVersion.size() + 3);
    out.append(media.data(), media.size());
    out += kEncodingSeparator;
    out.append(type_name.data(), type_name.size());
    out += kEncodingSeparator;
    out.append(kSchemaVersionParam.data(), kSchemaVersionParam.size());
    out += '=';
    out.append(kWireSchemaVersion.data(), kWireSchemaVersion.size());
    return out;
}

/// Strips leading and trailing ASCII whitespace, matching the engine's
/// `str::trim` on the schema half of an annotation for the characters that
/// can legally appear there.
inline std::string_view trim_ascii(std::string_view s) noexcept
{
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e)
    {
        const unsigned char c = static_cast<unsigned char>(s[b]);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') { ++b; } else { break; }
    }
    while (e > b)
    {
        const unsigned char c = static_cast<unsigned char>(s[e - 1]);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') { --e; } else { break; }
    }
    return s.substr(b, e - b);
}

/// True when `v` is `major.minor` or `major.minor.patch`, every part one or
/// more ASCII digits, and its `major.minor` is exactly `want`. Anything else --
/// `abc`, `0.9.`, `0`, `0.9.1.2` -- is not a version, and is a mismatch rather
/// than "absent": a peer that wrote the parameter meant to say something, and
/// a reader that cannot read it cannot vouch for the layout either.
inline bool schema_version_matches(std::string_view v, std::string_view want) noexcept
{
    std::size_t parts = 0;
    std::size_t minor_end = std::string_view::npos;
    std::size_t i = 0;
    while (true)
    {
        const std::size_t start = i;
        while (i < v.size() && v[i] >= '0' && v[i] <= '9') { ++i; }
        if (i == start) { return false; }   // an empty or non-numeric part
        ++parts;
        if (parts == 2) { minor_end = i; }
        if (i == v.size()) { break; }
        if (v[i] != '.' || parts == 3) { return false; }
        ++i;
    }
    if (parts < 2) { return false; }
    return v.substr(0, minor_end) == want;
}

}  // namespace detail

/// `application/cdr;{TYPE_NAME};tcn_schema=0.11` for `T`.
///
/// The caller passes a type, not a string, so payload and annotation cannot
/// disagree -- and since B76, neither can the release the payload's layout
/// belongs to.
template <class T>
inline std::string cdr_encoding()
{
    return detail::join_encoding(kCdrMediaType, WireType<T>::name);
}

/// `application/json;{TYPE_NAME};tcn_schema=0.11` for `T`.
template <class T>
inline std::string json_encoding()
{
    return detail::join_encoding(kJsonMediaType, WireType<T>::name);
}

/// An annotation split into its parts.
struct WireEncoding
{
    /// The media-type half, verbatim.
    std::string_view media;
    /// The type name: the segment between the first and second `;`, trimmed.
    /// Empty when the publisher declared none. Never includes a parameter.
    std::string_view type_name;
    /// Whether a non-empty type name was declared.
    bool has_type_name;
    /// The value of the `tcn_schema=` parameter, trimmed. **Empty when the
    /// parameter is absent**, which a pre-0.9 peer's annotation always is;
    /// `check_declared_type` refuses that as `MissingSchemaVersion`.
    std::string_view schema_version;
    /// Whether a `tcn_schema=` parameter was present at all (even with an
    /// empty value -- which `check_declared_type` refuses).
    bool has_schema_version;
};

/// Splits a Zenoh encoding annotation: media type up to the first `;`, type
/// name up to the second, then `;key=value` parameters.
///
/// The rules are the engine's, so a peer is refused here exactly when it would
/// be refused there:
///   * the type name, and a parameter's key and value (separately), are
///     trimmed of ASCII whitespace. The media type is **not** trimmed: it is
///     returned verbatim and `check_declared_type` compares it byte for byte,
///     so `" application/cdr"` is not CDR;
///   * a parameter whose key is not `tcn_schema` is ignored, as is one with no
///     `=` -- a later release may add parameters, and a reader that refused
///     them would turn every addition into a break;
///   * if `tcn_schema` appears twice, the first one counts.
inline WireEncoding parse_encoding(std::string_view encoding) noexcept
{
    WireEncoding out{encoding, std::string_view(), false, std::string_view(), false};
    const std::size_t at = encoding.find(kEncodingSeparator);
    if (at == std::string_view::npos) { return out; }
    out.media = encoding.substr(0, at);

    std::string_view rest = encoding.substr(at + 1);
    std::size_t next = rest.find(kEncodingSeparator);
    out.type_name = detail::trim_ascii(rest.substr(0, next));
    out.has_type_name = !out.type_name.empty();

    while (next != std::string_view::npos)
    {
        rest = rest.substr(next + 1);
        next = rest.find(kEncodingSeparator);
        const std::string_view param = rest.substr(0, next);
        const std::size_t eq = param.find('=');
        if (eq == std::string_view::npos) { continue; }
        if (detail::trim_ascii(param.substr(0, eq)) != kSchemaVersionParam) { continue; }
        if (out.has_schema_version) { continue; }
        out.schema_version = detail::trim_ascii(param.substr(eq + 1));
        out.has_schema_version = true;
    }
    return out;
}

/// The C++ counterpart of the engine's `decode_typed` type check: the whole
/// annotation, `application/cdr;{TYPE};tcn_schema=0.11`, is mandatory (V1,
/// §6). In order, first failure wins:
///
///   1. the media type is exactly `kCdrMediaType` or `kJsonMediaType` --
///      byte for byte, not a substring and not case-folded, so neither
///      `application/x-cdr` nor `APPLICATION/CDR` is read as CDR ->
///      `UnknownEncoding`;
///   2. a type name is declared -> `MissingDeclaredType`. CDR is not
///      self-describing, so a payload that does not name its type may parse as
///      `T` and mean something else entirely;
///   3. it is `T`'s -> `WrongDeclaredType`;
///   4. a `tcn_schema=` parameter is present -> `MissingSchemaVersion`. Every
///      pre-0.9 peer omits it; 0.9 and 0.10 accepted that so as not to make a
///      flag day, and 0.11 is a break anyway (XCDR2), so an unversioned
///      payload is no longer decoded;
///   5. its major.minor is `kWireSchemaVersion` -> `SchemaVersionMismatch`,
///      as is a value that is not a version at all.
///
/// The type is checked before the version because it is the more fundamental
/// refusal: a wrong type with a wrong version is a wrong type.
template <class T>
inline Result<void> check_declared_type(std::string_view encoding) noexcept
{
    const WireEncoding parsed = parse_encoding(encoding);
    if (parsed.media != kCdrMediaType && parsed.media != kJsonMediaType)
    {
        return Result<void>::fail(Error::UnknownEncoding);
    }
    if (!parsed.has_type_name) { return Result<void>::fail(Error::MissingDeclaredType); }
    if (parsed.type_name != WireType<T>::name) { return Result<void>::fail(Error::WrongDeclaredType); }
    if (!parsed.has_schema_version) { return Result<void>::fail(Error::MissingSchemaVersion); }
    if (!detail::schema_version_matches(parsed.schema_version, kWireSchemaVersion))
    {
        return Result<void>::fail(Error::SchemaVersionMismatch);
    }
    return Result<void>::ok();
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_WIRE_TYPE_H
