// SPDX-License-Identifier: internal
//
// WireType<T> — the `application/cdr;{TYPE};tcn_schema={M.m}` annotation,
// derived from the message type so it cannot be forgotten or misspelled.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.7.
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
// outcome.

namespace tcnart_msgs {
namespace msg {

class Pose6DMessage;
class Pose6DMapMessage;
class TargetTrackingMessage;
class StreamDescriptorMessage;
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
class SISRelationStreamHandle;
class SISBlockingRelation;
class SISRelationStreamReply;
class SISRelationStreamStatusNotification;
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

enum SISRelationStreamStatus : std::uint32_t;
enum SISReoptimisePolicy : std::uint32_t;
enum SISMutationStatus : std::uint32_t;
enum SISGraphQueryKind : std::uint32_t;
enum SISGraphQueryStatus : std::uint32_t;
enum SRPatternExpansion : std::uint32_t;
enum SISTargetKind : std::uint32_t;
enum SISTargetQueryKind : std::uint32_t;
enum SISTargetQueryStatus : std::uint32_t;
enum SISTargetMutationKind : std::uint32_t;

}  // namespace rpc
}  // namespace tcnart_msgs

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
/// `;tcn_schema=0.9`, and compared against the one a peer declares. Only
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
/// meaning did not.
inline constexpr std::string_view kWireSchemaVersion = "0.9";

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
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::Pose6DMapMessage,       "tcnart_msgs::msg::Pose6DMapMessage");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::TargetTrackingMessage,  "tcnart_msgs::msg::TargetTrackingMessage");

// tcnart_msgs/msg/StreamDescriptor.idl
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::StreamDescriptorMessage, "tcnart_msgs::msg::StreamDescriptorMessage");

// tcnart_msgs/msg/SpatialRelations.idl
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::SRGraph, "tcnart_msgs::msg::SRGraph");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::SRNode,  "tcnart_msgs::msg::SRNode");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::SREdge,  "tcnart_msgs::msg::SREdge");
// The read side of the same file: a declaration qualified by the fragment that
// made it. `::msg::` and `SR*`, not `::rpc::` and `SIS*`, because these are
// graph types and the namespace decides the wire name a peer must match.
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::SRNodeView, "tcnart_msgs::msg::SRNodeView");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::msg::SREdgeView, "tcnart_msgs::msg::SREdgeView");

// tcnart_msgs/rpc/SIS.idl
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISJoinRequest,                "tcnart_msgs::rpc::SISJoinRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISLeaveRequest,               "tcnart_msgs::rpc::SISLeaveRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISNodeUpdateRequest,          "tcnart_msgs::rpc::SISNodeUpdateRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISNodeRemoveRequest,          "tcnart_msgs::rpc::SISNodeRemoveRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISEdgeUpdateRequest,          "tcnart_msgs::rpc::SISEdgeUpdateRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISEdgeRemoveRequest,          "tcnart_msgs::rpc::SISEdgeRemoveRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISRelationStreamStartRequest, "tcnart_msgs::rpc::SISRelationStreamStartRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISRelationStreamStopRequest,  "tcnart_msgs::rpc::SISRelationStreamStopRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISRelationStreamStatus,       "tcnart_msgs::rpc::SISRelationStreamStatus");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISRelationStreamHandle,       "tcnart_msgs::rpc::SISRelationStreamHandle");
// The re-optimisation policy a start request carries (B51). Nested inside that
// request rather than published alone, and registered for the same reason
// SISRelationStreamStatus is: it is the one field of the request that is *not*
// stream identity, a caller reasons about it on its own, and a name spelled by
// hand is a name that can be spelled wrong.
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISReoptimisePolicy,           "tcnart_msgs::rpc::SISReoptimisePolicy");
// The relation a pending stream is waiting on. Nested inside the reply and the
// notification below rather than published alone, and registered anyway for the
// same reason SRNodeView and SREdgeView are: a calibration tool passes one of
// these around on its own, and a name it has to spell by hand is a name it can
// spell wrong.
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISBlockingRelation,           "tcnart_msgs::rpc::SISBlockingRelation");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISRelationStreamReply,        "tcnart_msgs::rpc::SISRelationStreamReply");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISRelationStreamStatusNotification,
                           "tcnart_msgs::rpc::SISRelationStreamStatusNotification");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISMutationStatus,   "tcnart_msgs::rpc::SISMutationStatus");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISMutationReply,    "tcnart_msgs::rpc::SISMutationReply");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISGraphQueryKind,   "tcnart_msgs::rpc::SISGraphQueryKind");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISGraphQueryStatus, "tcnart_msgs::rpc::SISGraphQueryStatus");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISGraphQueryRequest,
                           "tcnart_msgs::rpc::SISGraphQueryRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISGraphQueryReply,
                           "tcnart_msgs::rpc::SISGraphQueryReply");
// The pattern query (B63), on `/sis/pattern/query`. The engine answered it
// since B63 and this table could not name either half until 0.9.0 -- the same
// gap the graph read sat in before B46 was registered. The reply reuses
// SISGraphQueryStatus, so the annotation is the only thing that separates a
// pattern reply from a graph reply: both open with that enum.
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SRPatternExpansion,     "tcnart_msgs::rpc::SRPatternExpansion");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISPatternQueryRequest, "tcnart_msgs::rpc::SISPatternQueryRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISPatternQueryReply,   "tcnart_msgs::rpc::SISPatternQueryReply");
// The global target registry (B78). `SISTargetAlias` and `SISGlobalTarget` are
// nested, never published alone, and registered for the reason
// SISBlockingRelation is: a tool that seeds or exports the registry passes
// them around by themselves. `SISTargetChange` is published by the engine on
// `sqe/{engine_id}/targets/changed` (topics.h); the mutation is answered with
// the ordinary SISMutationReply, not a registry-specific reply.
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetKind,            "tcnart_msgs::rpc::SISTargetKind");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetAlias,           "tcnart_msgs::rpc::SISTargetAlias");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISGlobalTarget,          "tcnart_msgs::rpc::SISGlobalTarget");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetQueryKind,       "tcnart_msgs::rpc::SISTargetQueryKind");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetQueryRequest,    "tcnart_msgs::rpc::SISTargetQueryRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetQueryStatus,     "tcnart_msgs::rpc::SISTargetQueryStatus");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetQueryReply,      "tcnart_msgs::rpc::SISTargetQueryReply");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetMutationKind,    "tcnart_msgs::rpc::SISTargetMutationKind");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetMutationRequest, "tcnart_msgs::rpc::SISTargetMutationRequest");
TCN_SQE_REGISTER_WIRE_TYPE(::tcnart_msgs::rpc::SISTargetChange,          "tcnart_msgs::rpc::SISTargetChange");

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

/// ASCII-only lowering. Not `std::tolower`: that one consults the global
/// locale and is undefined for a negative `char`.
constexpr char ascii_lower(char c) noexcept
{
    const unsigned char u = static_cast<unsigned char>(c);
    return (u >= 'A' && u <= 'Z') ? static_cast<char>(u - 'A' + 'a') : c;
}

/// True when `haystack`, lowered, contains `needle` (which must already be
/// lower case).
inline bool contains_ascii_ci(std::string_view haystack, std::string_view needle) noexcept
{
    if (needle.size() > haystack.size()) { return false; }
    for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i)
    {
        std::size_t j = 0;
        for (; j < needle.size(); ++j)
        {
            if (ascii_lower(haystack[i + j]) != needle[j]) { break; }
        }
        if (j == needle.size()) { return true; }
    }
    return false;
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

/// `application/cdr;{TYPE_NAME};tcn_schema=0.9` for `T`.
///
/// The caller passes a type, not a string, so payload and annotation cannot
/// disagree -- and since B76, neither can the release the payload's layout
/// belongs to.
template <class T>
inline std::string cdr_encoding()
{
    return detail::join_encoding(kCdrMediaType, WireType<T>::name);
}

/// `application/json;{TYPE_NAME};tcn_schema=0.9` for `T`.
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
    /// parameter is absent**, which a pre-0.9 peer's annotation always is.
    /// `check_declared_type` accepts that; a caller that wants to warn about
    /// an old peer tests `has_schema_version`.
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
///   * every segment is trimmed of ASCII whitespace, key and value separately;
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

/// The C++ counterpart of the engine's `decode_typed` type check.
///
/// The declaration is mandatory. CDR is not self-describing, so a payload that
/// does not name its type may parse as `T` and mean something else entirely —
/// refusing is the only sound option, and refusing here is local and loud.
///
/// The schema version (B76) is checked after the type: a declared
/// `tcn_schema=` whose major.minor is not `kWireSchemaVersion` is refused with
/// `SchemaVersionMismatch`, and so is one that is not a version at all. An
/// **absent** one is accepted, because every pre-0.9 peer omits it and
/// refusing them would make 0.9.0 a flag day; that is the engine's rule too.
/// Use `parse_encoding(...).has_schema_version` to warn about such a peer.
///
/// A reader built before 0.9.0 compared everything after the first `;` with
/// the type name, so it refuses every 0.9.0 annotation as
/// `WrongDeclaredType`. That is not fixable from this side; see
/// MIGRATION-0.9.0.md.
template <class T>
inline Result<void> check_declared_type(std::string_view encoding) noexcept
{
    const WireEncoding parsed = parse_encoding(encoding);
    if (!detail::contains_ascii_ci(parsed.media, "cdr")
        && !detail::contains_ascii_ci(parsed.media, "json"))
    {
        return Result<void>::fail(Error::UnknownEncoding);
    }
    if (!parsed.has_type_name) { return Result<void>::fail(Error::MissingDeclaredType); }
    if (parsed.type_name != WireType<T>::name) { return Result<void>::fail(Error::WrongDeclaredType); }
    if (parsed.has_schema_version
        && !detail::schema_version_matches(parsed.schema_version, kWireSchemaVersion))
    {
        return Result<void>::fail(Error::SchemaVersionMismatch);
    }
    return Result<void>::ok();
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_WIRE_TYPE_H
