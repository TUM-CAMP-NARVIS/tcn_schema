// SPDX-License-Identifier: internal
//
// Handle — constructible only from a reply.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.4.
#ifndef TCN_SQE_HANDLE_H
#define TCN_SQE_HANDLE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/status.h"
#include "tcn/sqe/topics.h"

namespace tcn {
namespace sqe {

/// The `SISRelationStreamHandleKind` discriminator values.
inline constexpr std::int32_t kHandleKindNone = 0;
inline constexpr std::int32_t kHandleKindAssigned = 1;

namespace detail {

/// Whether `R` has the shape of a generated `SISRelationStreamReply`.
///
/// `Handle::from_reply` is a template rather than a function taking the
/// generated type by name, for two reasons that both matter. First, it keeps
/// this library genuinely dependency-free: nothing here includes a generated
/// header, so the contract core compiles, and its tests run, on a machine with
/// no fastddsgen toolchain. Second, it lets the lifecycle be exercised against
/// a stub reply in a unit test — which is the only way the union-discriminator
/// discipline below gets tested at all without a daemon.
///
/// It costs nothing in safety: the shape required here is one no string,
/// topic, or hand-rolled identifier has.
template <class R, class = void>
struct has_reply_shape : std::false_type {};

template <class R>
struct has_reply_shape<R, std::void_t<
    decltype(std::declval<const R&>().status()),
    decltype(std::declval<const R&>().handle()._d()),
    decltype(std::declval<const R&>().handle().topic())>> : std::true_type {};

}  // namespace detail

/// The status a reply reports, read without touching the handle union.
///
/// Separate from `Handle::from_reply` because a refusal has a status worth
/// reading and no handle to carry it: `from_reply` on a `NoPath` reply
/// correctly fails, and the caller still needs to know *why*.
template <class Reply>
Result<Status> reply_status(const Reply& reply) noexcept
{
    static_assert(detail::has_reply_shape<Reply>::value,
                  "reply_status expects a generated SISRelationStreamReply");
    return status_from_ordinal(static_cast<std::int32_t>(reply.status()));
}

/// An engine-issued relation-stream handle.
///
/// There is no constructor taking a string, no `Handle::compute(observer,
/// target)`, and no FNV-1a anywhere in this library. The hash *is* specified
/// in the engine's sources, and that is not a reason to implement it: a
/// computed handle stops nothing and subscribes to a topic nothing publishes
/// on, and a correct implementation would pin this side to a hash the engine
/// is free to change. The compile error is the only unambiguous signal there
/// is.
class Handle
{
public:
    /// The only way to obtain one.
    ///
    /// Reads the union's discriminator before any case accessor — a generated
    /// union's case accessor *throws* `BadParamException` when the requested
    /// member is not the current selection, and this library adds no exception
    /// requirement of its own. Also enforces the reply invariant: `ASSIGNED`
    /// occurs exactly when the status is `Active` or `Pending`. A violation is
    /// reported, not papered over, because either half may be the wrong one.
    template <class Reply>
    static Result<Handle> from_reply(const Reply& reply)
    {
        static_assert(detail::has_reply_shape<Reply>::value,
                      "Handle::from_reply expects a generated SISRelationStreamReply; "
                      "a handle is never built from a topic string");

        const Result<Status> status = reply_status(reply);
        if (!status) { return Result<Handle>::fail(status.error()); }

        const bool assigned =
            static_cast<std::int32_t>(reply.handle()._d()) == kHandleKindAssigned;

        if (assigned != carries_handle(status.value()))
        {
            return Result<Handle>::fail(Error::HandleInvariantViolated);
        }
        if (!assigned)
        {
            return Result<Handle>::fail(Error::HandleAbsent);
        }

        std::string topic(reply.handle().topic());
        if (topic.empty()) { return Result<Handle>::fail(Error::EmptyHandleTopic); }

        return Result<Handle>::ok(Handle(std::move(topic), status.value()));
    }

    /// Spelled out so the attempt produces "use of deleted function" rather
    /// than "no matching constructor". A handle is not a string you have.
    explicit Handle(std::string_view) = delete;

    /// Verbatim from the reply. Never recomputed, never rewritten.
    const std::string& data_topic() const noexcept { return topic_; }

    /// `{data_topic}/status`.
    std::string status_topic() const { return status_topic_for(topic_); }

    /// The status the reply carried alongside this handle: `Active` or
    /// `Pending`, and `Pending` is acceptance.
    Status status() const noexcept { return status_; }

    friend bool operator==(const Handle& a, const Handle& b) noexcept { return a.topic_ == b.topic_; }
    friend bool operator!=(const Handle& a, const Handle& b) noexcept { return !(a == b); }

private:
    Handle(std::string topic, Status status) noexcept
        : topic_(std::move(topic)), status_(status) {}

    std::string topic_;
    Status status_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_HANDLE_H
