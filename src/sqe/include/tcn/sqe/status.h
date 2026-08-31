// SPDX-License-Identifier: internal
//
// Status — the relation-stream statuses, with the ordinals pinned and the
// semantics named.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.5.
#ifndef TCN_SQE_STATUS_H
#define TCN_SQE_STATUS_H

#include <cstdint>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"

namespace tcn {
namespace sqe {

/// The answer to a relation-stream start or stop.
///
/// The ordinals are the wire's, taken from `tcnart_msgs/rpc/SIS.idl` and
/// `sqe-core/src/messages.rs`. They are spelled out rather than left implicit
/// so that a renumbering upstream is a visible diff here rather than an
/// unwalkable edge nobody notices.
enum class Status : std::int32_t
{
    /// Running.
    Active = 0,
    /// Not running: stopped, or never started. Also the answer to a stop —
    /// and it means only *"you are no longer attached"*, never "the stream
    /// stopped": another process may still hold the same handle.
    Inactive = 1,
    /// The graph does not connect these two frames. Waiting does not help.
    NoPath = 2,
    /// A named client or node does not exist. Usually a typo.
    UnknownFrame = 3,
    /// The path crosses unsynchronized clocks. Reserved; the engine does not
    /// emit it today.
    ClockDomainMismatch = 4,
    /// Malformed request.
    Rejected = 5,
    /// **Accepted**, not producing right now, and it will proceed on its own
    /// once the missing part arrives. The client does nothing. Appended last
    /// so no existing ordinal moved.
    Pending = 6,
};

/// The wire spelling of each status, for a log line. Never null.
inline const char* status_name(Status s) noexcept
{
    switch (s)
    {
        case Status::Active:              return "SIS_RS_ACTIVE";
        case Status::Inactive:            return "SIS_RS_INACTIVE";
        case Status::NoPath:              return "SIS_RS_NO_PATH";
        case Status::UnknownFrame:        return "SIS_RS_UNKNOWN_FRAME";
        case Status::ClockDomainMismatch: return "SIS_RS_CLOCK_DOMAIN_MISMATCH";
        case Status::Rejected:            return "SIS_RS_REJECTED";
        case Status::Pending:             return "SIS_RS_PENDING";
    }
    return "SIS_RS_UNKNOWN";
}

/// The request was **accepted**. Do not retry.
///
/// A function rather than a paragraph, because "treat `Pending` as
/// acceptance" is precisely the sentence consumers have read and not applied:
/// retrying against `Pending` is the failure the status split was introduced
/// to end.
constexpr bool is_acceptance(Status s) noexcept
{
    return s == Status::Active || s == Status::Pending;
}

/// A reply with this status carries an assigned handle — and a reply with any
/// other status does not. This is the reply invariant, as a predicate.
constexpr bool carries_handle(Status s) noexcept
{
    return s == Status::Active || s == Status::Pending;
}

/// Nothing about this will change while the graph does not. Retrying the same
/// request against the same graph gets the same answer.
constexpr bool is_final_for_this_graph(Status s) noexcept
{
    return s == Status::NoPath || s == Status::UnknownFrame || s == Status::Rejected;
}

/// The highest ordinal this library knows.
inline constexpr std::int32_t kMaxStatusOrdinal = 6;

/// Maps a wire ordinal onto a `Status`, refusing one this library does not
/// know rather than casting an unknown integer into the enum.
inline Result<Status> status_from_ordinal(std::int32_t ordinal) noexcept
{
    if (ordinal < 0 || ordinal > kMaxStatusOrdinal)
    {
        return Result<Status>::fail(Error::IllegalCharacter);
    }
    return Result<Status>::ok(static_cast<Status>(ordinal));
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_STATUS_H
