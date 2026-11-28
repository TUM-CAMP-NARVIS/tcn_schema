// SPDX-License-Identifier: internal
//
// ReplyHeader — the first member of every reply and notification (#35 R1-R3),
// read into plain C++ values.
#ifndef TCN_SQE_REPLY_H
#define TCN_SQE_REPLY_H

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "tcn/sqe/status.h"

namespace tcn {
namespace sqe {

/// `sqe_msgs::rpc::SISReplyHeader`, with its enums mirrored.
///
/// `engine_id` is the second chunk of every key that engine issues;
/// `engine_epoch` is its process start in Unix milliseconds, and a different
/// epoch from the same engine id means it restarted (every relation stream
/// must be re-started; joined fragments survive).
struct ReplyHeader
{
    ReplyStatus status = ReplyStatus::Unknown;
    Reason reason = Reason::Unknown;
    /// Human-readable; for a log line, not for parsing.
    std::string detail;
    std::string engine_id;
    std::uint64_t engine_epoch = 0;

    /// The request was carried out. Only `Ok` is: `Truncated` was carried out
    /// but is not the whole answer, and a caller that accepts it says so.
    bool ok() const noexcept { return status == ReplyStatus::Ok; }
};

namespace detail {

/// Whether `R` has a generated reply's `header()`.
template <class R, class = void>
struct has_reply_header : std::false_type {};

template <class R>
struct has_reply_header<R, std::void_t<
    decltype(std::declval<const R&>().header().status()),
    decltype(std::declval<const R&>().header().reason()),
    decltype(std::declval<const R&>().header().detail()),
    decltype(std::declval<const R&>().header().engine_id()),
    decltype(std::declval<const R&>().header().engine_epoch())>> : std::true_type {};

}  // namespace detail

/// The header of any generated reply or notification, read without naming the
/// generated type -- a template for the reason `Handle::from_reply` is one:
/// this library includes no generated header.
///
/// A status or reason ordinal this library does not know reads as `Unknown`
/// (`from_ordinal`), so a reply from a newer engine is never read as success
/// by accident.
template <class Reply>
ReplyHeader reply_header(const Reply& reply)
{
    static_assert(detail::has_reply_header<Reply>::value,
                  "reply_header expects a generated reply or notification (one with header())");
    const auto& h = reply.header();
    ReplyHeader out;
    out.status = from_ordinal<ReplyStatus>(static_cast<std::int32_t>(h.status()));
    out.reason = from_ordinal<Reason>(static_cast<std::int32_t>(h.reason()));
    out.detail = h.detail();
    out.engine_id = h.engine_id();
    out.engine_epoch = static_cast<std::uint64_t>(h.engine_epoch());
    return out;
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_REPLY_H
