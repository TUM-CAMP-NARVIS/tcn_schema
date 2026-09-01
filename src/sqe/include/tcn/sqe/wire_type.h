// SPDX-License-Identifier: internal
//
// WireType<T> — the `application/cdr;{TYPE}` annotation, derived from the
// message type so it cannot be forgotten or misspelled.
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

enum SISRelationStreamStatus : std::uint32_t;
enum SISMutationStatus : std::uint32_t;
enum SISGraphQueryKind : std::uint32_t;
enum SISGraphQueryStatus : std::uint32_t;

}  // namespace rpc
}  // namespace tcnart_msgs

namespace tcn {
namespace sqe {

// ---------------------------------------------------------------------------
// The encoding annotation
// ---------------------------------------------------------------------------

inline constexpr std::string_view kCdrMediaType = "application/cdr";
inline constexpr std::string_view kJsonMediaType = "application/json";

/// Separates the media type from the schema. One character, no space after it.
inline constexpr char kEncodingSeparator = ';';

/// A message type that names itself on the wire.
///
/// **There is no primary definition on purpose.** An unregistered type is a
/// compile error at the call site, which is where a name the engine refuses —
/// `SISJoinMessage`, say, which 0.3.0 deleted and which still appears in older
/// prose — should be caught. The alternative, a helper taking a
/// `std::string_view`, lets a caller pass a string that is merely wrong, and
/// the daemon does not answer mutations, so nothing would ever say so.
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

namespace detail {

inline std::string join_encoding(std::string_view media, std::string_view type_name)
{
    std::string out;
    out.reserve(media.size() + 1 + type_name.size());
    out.append(media.data(), media.size());
    out += kEncodingSeparator;
    out.append(type_name.data(), type_name.size());
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

}  // namespace detail

/// `application/cdr;{TYPE_NAME}` for `T`.
///
/// The caller passes a type, not a string, so payload and annotation cannot
/// disagree.
template <class T>
inline std::string cdr_encoding()
{
    return detail::join_encoding(kCdrMediaType, WireType<T>::name);
}

/// `application/json;{TYPE_NAME}` for `T`.
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
    /// The schema half, trimmed. Empty when the publisher declared none.
    std::string_view type_name;
    /// Whether a `;` was present at all.
    bool has_type_name;
};

/// Splits a Zenoh encoding annotation at the first `;`.
inline WireEncoding parse_encoding(std::string_view encoding) noexcept
{
    const std::size_t at = encoding.find(kEncodingSeparator);
    if (at == std::string_view::npos)
    {
        return WireEncoding{encoding, std::string_view(), false};
    }
    const std::string_view schema = detail::trim_ascii(encoding.substr(at + 1));
    return WireEncoding{encoding.substr(0, at), schema, !schema.empty()};
}

/// The C++ counterpart of the engine's `decode_typed` type check.
///
/// The declaration is mandatory. CDR is not self-describing, so a payload that
/// does not name its type may parse as `T` and mean something else entirely —
/// refusing is the only sound option, and refusing here is local and loud.
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
    return Result<void>::ok();
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_WIRE_TYPE_H
