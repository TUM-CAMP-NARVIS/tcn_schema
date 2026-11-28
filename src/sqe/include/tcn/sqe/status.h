// SPDX-License-Identifier: internal
//
// The reply status space (#35 R1-R3), as C++ mirrors with the ordinals pinned
// and the semantics named.
//
// One status for the whole protocol: `ReplyStatus` says whether a request was
// carried out, `Reason` why not, in a form a client branches on. What a reply
// says about its *subject* -- a relation's `RelationState`, why it is parked --
// is a domain field of that reply, never a status. See reply.h for the header
// these are read from.
#ifndef TCN_SQE_STATUS_H
#define TCN_SQE_STATUS_H

#include <cstdint>

namespace tcn {
namespace sqe {

// Every mirror is `std::int32_t`, starts with `Unknown = 0` (E6), and spells
// every ordinal out, so a renumbering upstream is a visible diff here (and
// tools/idl/tests/test_contract_enums.py compares them with the IDL). `0` is
// never success: a zeroed or truncated header reads as `Unknown`, not `Ok`.

/// `sqe_msgs::rpc::SISStatus`: whether the request was carried out.
enum class ReplyStatus : std::int32_t
{
    Unknown = 0,
    /// Carried out; the reply's domain fields say what came of it.
    Ok = 1,
    /// Refused as asked; `Reason` says why. Retrying unchanged is refused again.
    Rejected = 2,
    /// Not carried out inside its bound. For a write this means *unknown*, not
    /// "did not land": retrying is safe because every write is an upsert.
    Unavailable = 3,
    /// No such fragment; join before mutating it.
    UnknownClient = 4,
    /// A named frame does not exist; which one is in `detail`.
    UnknownFrame = 5,
    /// The named handle, target or other non-frame object does not exist. The
    /// answer to a stop of a handle the engine does not know (R4).
    NotFound = 6,
    /// Carried out, but the answer was cut at a limit.
    Truncated = 7,
    /// Contradicts state the engine holds (`Reason::TermsConflict`).
    Conflict = 8,
    /// Outside the answering engine's client scope.
    NotInScope = 9,
};

/// `sqe_msgs::rpc::SISReason`: why, in a form a client branches on. An OK
/// reply carries `Unknown` (none stated).
enum class Reason : std::int32_t
{
    Unknown = 0,
    Malformed = 1,
    SchemaMismatch = 2,
    Unsupported = 3,
    TermsConflict = 4,
    NotPermitted = 5,
    NoPath = 6,
    ClockDomainMismatch = 7,
    LimitExceeded = 8,
    StoreUnavailable = 9,
};

/// `sqe_msgs::rpc::SISRelationState`: what a relation stream is doing.
enum class RelationState : std::int32_t
{
    /// What every non-OK reply carries: the request was not carried out, so
    /// there is no state to report.
    Unknown = 0,
    /// Running: producing values on the data topic.
    Active = 1,
    /// **Accepted**, not producing yet; `ParkReason` says why. Resumes on its
    /// own once the missing part arrives, so the client does nothing -- a
    /// retry loop against `Pending` is the failure this state exists to end.
    Pending = 2,
    /// Not running: stopped, or never started. The answer to a stop, and it
    /// means only *"you are no longer attached"*, never "the stream stopped":
    /// another process may still hold the same handle.
    Inactive = 3,
};

/// `sqe_msgs::rpc::SISParkReason`: why a `Pending` relation is parked.
enum class ParkReason : std::int32_t
{
    Unknown = 0,
    Uncalibrated = 1,
    AwaitingDescriptor = 2,
    InputStarved = 3,
    Resolving = 4,
    /// A frame on the route was withdrawn: a running stream that lost its route.
    Withdrawn = 5,
    ClockDomainMismatch = 6,
};

namespace detail {

constexpr std::int32_t max_ordinal(ReplyStatus) noexcept { return 9; }
constexpr std::int32_t max_ordinal(Reason) noexcept { return 9; }
constexpr std::int32_t max_ordinal(RelationState) noexcept { return 3; }
constexpr std::int32_t max_ordinal(ParkReason) noexcept { return 6; }

}  // namespace detail

/// Maps a wire ordinal onto a mirror. An ordinal this library does not know --
/// one a newer engine appended, or a negative one -- is `E::Unknown`: never
/// cast into the enum, and never a separate error, because `Unknown` already
/// is "not OK" and "no state" (E6).
template <class E>
constexpr E from_ordinal(std::int32_t ordinal) noexcept
{
    return (ordinal < 0 || ordinal > detail::max_ordinal(E{})) ? E::Unknown : static_cast<E>(ordinal);
}

/// The IDL spelling of each value, for a log line. Never null; a value outside
/// the enumerators names itself `..._UNKNOWN`.
inline const char* wire_name(ReplyStatus s) noexcept
{
    switch (s)
    {
        case ReplyStatus::Unknown:       break;
        case ReplyStatus::Ok:            return "SIS_STATUS_OK";
        case ReplyStatus::Rejected:      return "SIS_STATUS_REJECTED";
        case ReplyStatus::Unavailable:   return "SIS_STATUS_UNAVAILABLE";
        case ReplyStatus::UnknownClient: return "SIS_STATUS_UNKNOWN_CLIENT";
        case ReplyStatus::UnknownFrame:  return "SIS_STATUS_UNKNOWN_FRAME";
        case ReplyStatus::NotFound:      return "SIS_STATUS_NOT_FOUND";
        case ReplyStatus::Truncated:     return "SIS_STATUS_TRUNCATED";
        case ReplyStatus::Conflict:      return "SIS_STATUS_CONFLICT";
        case ReplyStatus::NotInScope:    return "SIS_STATUS_NOT_IN_SCOPE";
    }
    return "SIS_STATUS_UNKNOWN";
}

inline const char* wire_name(Reason r) noexcept
{
    switch (r)
    {
        case Reason::Unknown:             break;
        case Reason::Malformed:           return "SIS_REASON_MALFORMED";
        case Reason::SchemaMismatch:      return "SIS_REASON_SCHEMA_MISMATCH";
        case Reason::Unsupported:         return "SIS_REASON_UNSUPPORTED";
        case Reason::TermsConflict:       return "SIS_REASON_TERMS_CONFLICT";
        case Reason::NotPermitted:        return "SIS_REASON_NOT_PERMITTED";
        case Reason::NoPath:              return "SIS_REASON_NO_PATH";
        case Reason::ClockDomainMismatch: return "SIS_REASON_CLOCK_DOMAIN_MISMATCH";
        case Reason::LimitExceeded:       return "SIS_REASON_LIMIT_EXCEEDED";
        case Reason::StoreUnavailable:    return "SIS_REASON_STORE_UNAVAILABLE";
    }
    return "SIS_REASON_UNKNOWN";
}

inline const char* wire_name(RelationState s) noexcept
{
    switch (s)
    {
        case RelationState::Unknown:  break;
        case RelationState::Active:   return "SIS_RELATION_STATE_ACTIVE";
        case RelationState::Pending:  return "SIS_RELATION_STATE_PENDING";
        case RelationState::Inactive: return "SIS_RELATION_STATE_INACTIVE";
    }
    return "SIS_RELATION_STATE_UNKNOWN";
}

inline const char* wire_name(ParkReason r) noexcept
{
    switch (r)
    {
        case ParkReason::Unknown:             break;
        case ParkReason::Uncalibrated:        return "SIS_PARK_REASON_UNCALIBRATED";
        case ParkReason::AwaitingDescriptor:  return "SIS_PARK_REASON_AWAITING_DESCRIPTOR";
        case ParkReason::InputStarved:        return "SIS_PARK_REASON_INPUT_STARVED";
        case ParkReason::Resolving:           return "SIS_PARK_REASON_RESOLVING";
        case ParkReason::Withdrawn:           return "SIS_PARK_REASON_WITHDRAWN";
        case ParkReason::ClockDomainMismatch: return "SIS_PARK_REASON_CLOCK_DOMAIN_MISMATCH";
    }
    return "SIS_PARK_REASON_UNKNOWN";
}

/// An OK stream reply in this state carries a handle -- and one in any other
/// state does not (H1). The reply invariant, as a predicate: `Active` and
/// `Pending` are the two states that mean "there is a stream on these topics
/// for you".
constexpr bool carries_handle(RelationState s) noexcept
{
    return s == RelationState::Active || s == RelationState::Pending;
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_STATUS_H
