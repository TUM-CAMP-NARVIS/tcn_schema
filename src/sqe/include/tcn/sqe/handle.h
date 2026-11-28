// SPDX-License-Identifier: internal
//
// Handle — constructible only from a reply, with every topic the reply's.
//
// See docs/design/2026-08-31-sqe-contract-library-design.md §4.4 and #35 H1/H2.
#ifndef TCN_SQE_HANDLE_H
#define TCN_SQE_HANDLE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tcn/sqe/error.h"
#include "tcn/sqe/reply.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/status.h"

namespace tcn {
namespace sqe {

namespace detail {

/// Whether `R` has the shape of a generated `SISRelationStreamReply`: a
/// header, a relation state, and an optional `SISStreamHandle` with its four
/// strings.
///
/// `Handle::from_reply` is a template rather than a function taking the
/// generated type by name, for two reasons that both matter. First, it keeps
/// this library genuinely dependency-free: nothing here includes a generated
/// header, so the contract core compiles, and its tests run, on a machine with
/// no fastddsgen toolchain. Second, it lets the lifecycle be exercised against
/// a stub reply in a unit test -- which is the only way the optional-member
/// discipline below gets tested at all without a daemon.
///
/// It costs nothing in safety: the shape required here is one no string,
/// topic, or hand-rolled identifier has.
template <class R, class = void>
struct has_reply_shape : std::false_type {};

template <class R>
struct has_reply_shape<R, std::void_t<
    decltype(std::declval<const R&>().header().status()),
    decltype(std::declval<const R&>().relation().state()),
    decltype(std::declval<const R&>().handle().has_value()),
    decltype(std::declval<const R&>().handle().value().handle()),
    decltype(std::declval<const R&>().handle().value().data_topic()),
    decltype(std::declval<const R&>().handle().value().status_topic()),
    decltype(std::declval<const R&>().handle().value().plan_topic())>> : std::true_type {};

}  // namespace detail

/// An engine-issued relation-stream handle, and the topics the engine assigned
/// to it.
///
/// There is no constructor taking a string, no `Handle::compute(observer,
/// target)`, and no topic composed anywhere in this class. The handle is opaque
/// (it may equal the data topic today; nothing may rely on that), and the
/// status and plan topics are the reply's, not `{data_topic}/status` -- a
/// client that composes them subscribes to keys the engine is free to move.
/// The compile error is the only unambiguous signal there is.
class Handle
{
public:
    /// The only way to obtain one.
    ///
    /// In order, first failure wins:
    ///   * the header is not OK -> `ReplyRefused` (read *why* with
    ///     `reply_header`; a refusal carries no handle and its relation state
    ///     is `Unknown`);
    ///   * the handle's presence disagrees with the relation state -- present
    ///     exactly when the state is `Active` or `Pending` (H1) ->
    ///     `HandleInvariantViolated`, reported rather than papered over,
    ///     because either half may be the wrong one;
    ///   * no handle (an OK stop: `Inactive`) -> `HandleAbsent`;
    ///   * any of the four strings empty -> `EmptyHandleTopic`.
    ///
    /// `has_value()` is asked before `value()` on every path: fast-cdr's
    /// `optional::value()` *throws* on an absent member, and this library adds
    /// no exception requirement of its own.
    template <class Reply>
    static Result<Handle> from_reply(const Reply& reply)
    {
        static_assert(detail::has_reply_shape<Reply>::value,
                      "Handle::from_reply expects a generated SISRelationStreamReply; "
                      "a handle is never built from a topic string");

        if (!reply_header(reply).ok()) { return Result<Handle>::fail(Error::ReplyRefused); }

        const RelationState state =
            from_ordinal<RelationState>(static_cast<std::int32_t>(reply.relation().state()));
        const bool present = reply.handle().has_value();

        if (present != carries_handle(state))
        {
            return Result<Handle>::fail(Error::HandleInvariantViolated);
        }
        if (!present) { return Result<Handle>::fail(Error::HandleAbsent); }

        const auto& h = reply.handle().value();
        std::string handle(h.handle());
        std::string data(h.data_topic());
        std::string status(h.status_topic());
        std::string plan(h.plan_topic());
        if (handle.empty() || data.empty() || status.empty() || plan.empty())
        {
            return Result<Handle>::fail(Error::EmptyHandleTopic);
        }

        return Result<Handle>::ok(
            Handle(std::move(handle), std::move(data), std::move(status), std::move(plan), state));
    }

    /// Spelled out so the attempt produces "use of deleted function" rather
    /// than "no matching constructor". A handle is not a string you have.
    explicit Handle(std::string_view) = delete;

    /// The opaque handle, verbatim: what a stop names. Never parsed, never
    /// rewritten.
    const std::string& str() const noexcept { return handle_; }

    /// Where the stream's `SISRelationOutput`s arrive, one per evaluation.
    const std::string& data_topic() const noexcept { return data_topic_; }

    /// Where its `SISRelationStreamStatusNotification`s arrive. The reply's
    /// string -- not `{data_topic}/status`.
    const std::string& status_topic() const noexcept { return status_topic_; }

    /// Where its `SISPlanNotification`s arrive (LOCAL and BOTH streams).
    const std::string& plan_topic() const noexcept { return plan_topic_; }

    /// The state the reply carried alongside this handle: `Active` or
    /// `Pending`, and `Pending` is acceptance.
    RelationState state() const noexcept { return state_; }

    /// Two handles are the same stream when the engine issued the same handle.
    friend bool operator==(const Handle& a, const Handle& b) noexcept { return a.handle_ == b.handle_; }
    friend bool operator!=(const Handle& a, const Handle& b) noexcept { return !(a == b); }

private:
    Handle(std::string handle, std::string data, std::string status, std::string plan,
           RelationState state) noexcept
        : handle_(std::move(handle)),
          data_topic_(std::move(data)),
          status_topic_(std::move(status)),
          plan_topic_(std::move(plan)),
          state_(state) {}

    std::string handle_;
    std::string data_topic_;
    std::string status_topic_;
    std::string plan_topic_;
    RelationState state_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_HANDLE_H
