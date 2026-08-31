// SPDX-License-Identifier: internal
//
// Handle — constructible only from a reply, and only from one that agrees
// with itself.
#include <cstdint>
#include <string>
#include <type_traits>

#include "check.h"
#include "tcn/sqe/handle.h"

using namespace tcn::sqe;

namespace {

/// Records whether the case accessor was touched.
///
/// A generated union's case accessor *throws* `BadParamException` when the
/// requested member is not the current selection, and this library must never
/// let that escape. A test cannot assert "did not throw" in a build with
/// exceptions disabled, so the stub asserts the stronger and more useful
/// property instead: the accessor is never *reached* unless the discriminator
/// said ASSIGNED.
bool g_topic_touched_while_none = false;

class StubUnion
{
public:
    StubUnion(std::int32_t d, std::string topic) : d_(d), topic_(std::move(topic)) {}

    std::int32_t _d() const { return d_; }

    const std::string& topic() const
    {
        if (d_ != kHandleKindAssigned) { g_topic_touched_while_none = true; }
        return topic_;
    }

private:
    std::int32_t d_;
    std::string topic_;
};

class StubReply
{
public:
    StubReply(std::int32_t status, std::int32_t kind, std::string topic)
        : status_(status), handle_(kind, std::move(topic)) {}

    std::int32_t status() const { return status_; }
    const StubUnion& handle() const { return handle_; }

private:
    std::int32_t status_;
    StubUnion handle_;
};

StubReply assigned(Status s, const char* topic)
{
    return StubReply(static_cast<std::int32_t>(s), kHandleKindAssigned, topic);
}

StubReply without_handle(Status s)
{
    return StubReply(static_cast<std::int32_t>(s), kHandleKindNone, std::string());
}

}  // namespace

// ---------------------------------------------------------------------------
// A Handle is not a string you have
// ---------------------------------------------------------------------------

static_assert(!std::is_default_constructible<Handle>::value, "");
static_assert(!std::is_constructible<Handle, std::string>::value,
              "a Handle must not be constructible from a bare string");
static_assert(!std::is_constructible<Handle, const char*>::value, "");
static_assert(!std::is_constructible<Handle, std::string_view>::value, "");
static_assert(std::is_copy_constructible<Handle>::value, "");

// A bare string does not have the shape of a reply either, so
// `from_reply(some_string)` is ill-formed rather than merely wrong.
static_assert(!detail::has_reply_shape<std::string>::value, "");
static_assert(!detail::has_reply_shape<const char*>::value, "");
static_assert(detail::has_reply_shape<StubReply>::value, "");

TCN_SQE_TEST(a_handle_comes_from_an_active_reply_verbatim)
{
    const StubReply r = assigned(Status::Active, "sqe/lab1/rel/0123456789abcdef");
    const Result<Handle> h = Handle::from_reply(r);
    CHECK_OK(h);
    if (!h) { return; }

    CHECK_STR_EQ(h.value().data_topic(), "sqe/lab1/rel/0123456789abcdef");
    CHECK_STR_EQ(h.value().status_topic(), "sqe/lab1/rel/0123456789abcdef/status");
    CHECK(h.value().status() == Status::Active);
}

// Pending is acceptance, so a Pending reply carries a handle and subscribing
// to it is correct: the stream resumes on its own.
TCN_SQE_TEST(a_pending_reply_yields_a_handle_because_pending_is_acceptance)
{
    const Result<Handle> h = Handle::from_reply(assigned(Status::Pending, "sqe/lab1/rel/h42"));
    CHECK_OK(h);
    if (!h) { return; }
    CHECK(h.value().status() == Status::Pending);
    CHECK(is_acceptance(h.value().status()));
    CHECK_STR_EQ(h.value().data_topic(), "sqe/lab1/rel/h42");
}

TCN_SQE_TEST(a_refusal_carries_no_handle_and_says_so)
{
    CHECK_ERR(Handle::from_reply(without_handle(Status::NoPath)), Error::HandleAbsent);
    CHECK_ERR(Handle::from_reply(without_handle(Status::UnknownFrame)), Error::HandleAbsent);
    CHECK_ERR(Handle::from_reply(without_handle(Status::Rejected)), Error::HandleAbsent);
    CHECK_ERR(Handle::from_reply(without_handle(Status::ClockDomainMismatch)), Error::HandleAbsent);
    // The answer to a stop.
    CHECK_ERR(Handle::from_reply(without_handle(Status::Inactive)), Error::HandleAbsent);
}

TCN_SQE_TEST(the_discriminator_is_read_before_any_case_accessor)
{
    g_topic_touched_while_none = false;
    (void)Handle::from_reply(without_handle(Status::NoPath));
    (void)Handle::from_reply(without_handle(Status::Inactive));
    (void)Handle::from_reply(StubReply(static_cast<std::int32_t>(Status::Active),
                                       kHandleKindNone, std::string()));
    CHECK_MSG(!g_topic_touched_while_none,
              "a union case accessor was reached without checking the discriminator");
}

// Reported, not papered over: either half may be the wrong one, and a client
// that silently trusted one of them would be guessing.
TCN_SQE_TEST(a_reply_whose_status_and_handle_disagree_is_a_violation_not_an_absence)
{
    CHECK_ERR(Handle::from_reply(StubReply(static_cast<std::int32_t>(Status::Active),
                                           kHandleKindNone, std::string())),
              Error::HandleInvariantViolated);
    CHECK_ERR(Handle::from_reply(StubReply(static_cast<std::int32_t>(Status::Pending),
                                           kHandleKindNone, std::string())),
              Error::HandleInvariantViolated);
    CHECK_ERR(Handle::from_reply(assigned(Status::NoPath, "sqe/lab1/rel/h42")),
              Error::HandleInvariantViolated);
    CHECK_ERR(Handle::from_reply(assigned(Status::Inactive, "sqe/lab1/rel/h42")),
              Error::HandleInvariantViolated);
}

// An empty string standing in for "absent" is a legal topic-shaped value: it
// would subscribe to "" and be told nothing by anything.
TCN_SQE_TEST(an_assigned_handle_with_an_empty_topic_is_refused)
{
    CHECK_ERR(Handle::from_reply(assigned(Status::Active, "")), Error::EmptyHandleTopic);
}

TCN_SQE_TEST(an_unknown_status_ordinal_is_refused_before_anything_else)
{
    const StubReply r(99, kHandleKindAssigned, "sqe/lab1/rel/h42");
    CHECK(!Handle::from_reply(r).is_ok());
    CHECK(!reply_status(r).is_ok());
}

TCN_SQE_TEST(reply_status_reads_a_refusal_s_status_without_a_handle)
{
    const Result<Status> s = reply_status(without_handle(Status::NoPath));
    CHECK_OK(s);
    if (!s) { return; }
    CHECK(s.value() == Status::NoPath);
    CHECK(is_final_for_this_graph(s.value()));
}

TCN_SQE_TEST(handles_compare_by_the_topic_the_engine_issued)
{
    const Result<Handle> a = Handle::from_reply(assigned(Status::Active, "sqe/lab1/rel/h42"));
    const Result<Handle> b = Handle::from_reply(assigned(Status::Pending, "sqe/lab1/rel/h42"));
    const Result<Handle> c = Handle::from_reply(assigned(Status::Active, "sqe/lab1/rel/h43"));
    CHECK_OK(a); CHECK_OK(b); CHECK_OK(c);
    if (!a || !b || !c) { return; }
    CHECK(a.value() == b.value());
    CHECK(a.value() != c.value());
}
