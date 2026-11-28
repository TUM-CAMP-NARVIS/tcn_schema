// SPDX-License-Identifier: internal
//
// Handle — constructible only from a reply, only from an OK one, and only from
// one that agrees with itself. Its topics are the reply's (#35 H1, H2).
#include <cstdint>
#include <string>
#include <type_traits>

#include "check.h"
#include "stub_msgs.h"
#include "tcn/sqe/handle.h"

using namespace tcn::sqe;
using tcn::sqe::test::StubOptional;

namespace rpc = ::sqe_msgs::rpc;

namespace {

rpc::SISStreamHandle topics(const char* handle, const char* data, const char* status, const char* plan)
{
    rpc::SISStreamHandle h;
    h.handle_ = handle;
    h.data_topic_ = data;
    h.status_topic_ = status;
    h.plan_topic_ = plan;
    return h;
}

rpc::SISStreamHandle some_topics()
{
    return topics("h42", "sqe/lab1/rel/h42", "sqe/lab1/rel/h42/status", "sqe/lab1/rel/h42/plan");
}

rpc::SISRelationStreamReply reply(rpc::SISStatus status, rpc::SISRelationState state, bool with_handle,
                                  rpc::SISStreamHandle h = some_topics())
{
    rpc::SISRelationStreamReply r;
    r.header_.status_ = status;
    r.relation_.state_ = state;
    if (with_handle) { r.handle_.set(h); }
    return r;
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
static_assert(!detail::has_reply_shape<rpc::SISMutationReply>::value, "a mutation reply carries no handle");
static_assert(detail::has_reply_shape<rpc::SISRelationStreamReply>::value, "");

// ---------------------------------------------------------------------------
// The four topics are the reply's
// ---------------------------------------------------------------------------

TCN_SQE_TEST(an_ok_pending_reply_with_a_handle_gives_the_topics_as_given)
{
    const Result<Handle> h = Handle::from_reply(
        reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_PENDING, true,
              topics("opaque-7", "sqe/lab1/out/7", "sqe/lab1/st/7", "sqe/lab1/pl/7")));
    CHECK_OK(h);
    if (!h) { return; }
    CHECK_STR_EQ(h.value().str(), "opaque-7");
    CHECK_STR_EQ(h.value().data_topic(), "sqe/lab1/out/7");
    CHECK_STR_EQ(h.value().status_topic(), "sqe/lab1/st/7");
    CHECK_STR_EQ(h.value().plan_topic(), "sqe/lab1/pl/7");
    CHECK(h.value().state() == RelationState::Pending);
}

TCN_SQE_TEST(an_ok_active_reply_gives_an_active_handle)
{
    const Result<Handle> h = Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true));
    CHECK_OK(h);
    if (!h) { return; }
    CHECK(h.value().state() == RelationState::Active);
    CHECK_STR_EQ(h.value().data_topic(), "sqe/lab1/rel/h42");
}

// H2: the status topic is whatever the engine said. A client that composes
// `{data_topic}/status` subscribes to a key the engine may not publish on.
TCN_SQE_TEST(the_status_topic_is_the_reply_s_even_when_it_is_not_data_topic_slash_status)
{
    const Result<Handle> h = Handle::from_reply(
        reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true,
              topics("h42", "sqe/lab1/rel/h42", "sqe/lab1/notify/h42", "sqe/lab1/rel/h42/plan")));
    CHECK_OK(h);
    if (!h) { return; }
    CHECK_STR_EQ(h.value().status_topic(), "sqe/lab1/notify/h42");
    CHECK(h.value().status_topic() != h.value().data_topic() + "/status");
}

// ---------------------------------------------------------------------------
// Refusals and violations
// ---------------------------------------------------------------------------

// A refusal has no handle, and its header says why -- read it with
// `reply_header`. The relation state of a non-OK reply is UNKNOWN.
TCN_SQE_TEST(a_refused_reply_is_reply_refused)
{
    rpc::SISRelationStreamReply r = reply(rpc::SIS_STATUS_REJECTED, rpc::SIS_RELATION_STATE_UNKNOWN, false);
    r.header_.reason_ = rpc::SIS_REASON_NO_PATH;
    CHECK_ERR(Handle::from_reply(r), Error::ReplyRefused);
    CHECK(reply_header(r).reason == Reason::NoPath);

    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_UNKNOWN_FRAME, rpc::SIS_RELATION_STATE_UNKNOWN, false)),
              Error::ReplyRefused);
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_CONFLICT, rpc::SIS_RELATION_STATE_UNKNOWN, false)),
              Error::ReplyRefused);
    // A zeroed reply: status UNKNOWN is not OK.
    CHECK_ERR(Handle::from_reply(rpc::SISRelationStreamReply()), Error::ReplyRefused);
}

// The header is checked first: a refusal that also carries a handle is still
// a refusal, not a handle.
TCN_SQE_TEST(a_refusal_with_a_handle_is_still_a_refusal)
{
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_REJECTED, rpc::SIS_RELATION_STATE_ACTIVE, true)),
              Error::ReplyRefused);
}

// Reported, not papered over: either half may be the wrong one.
TCN_SQE_TEST(an_ok_reply_whose_state_and_handle_disagree_is_a_violation)
{
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, false)),
              Error::HandleInvariantViolated);
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_PENDING, false)),
              Error::HandleInvariantViolated);
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_INACTIVE, true)),
              Error::HandleInvariantViolated);
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_UNKNOWN, true)),
              Error::HandleInvariantViolated);
}

// The answer to a stop: OK, INACTIVE, no handle. Not a fault.
TCN_SQE_TEST(an_ok_inactive_reply_without_a_handle_is_absent)
{
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_INACTIVE, false)),
              Error::HandleAbsent);
}

// An empty string standing in for "absent" is a legal topic-shaped value: it
// would subscribe to "" and be told nothing by anything.
TCN_SQE_TEST(any_empty_handle_string_is_refused)
{
    const char* s = "sqe/x/s";
    const char* d = "sqe/x/d";
    const char* p = "sqe/x/p";
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true,
                                       topics("h", d, "", p))),
              Error::EmptyHandleTopic);
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true,
                                       topics("", d, s, p))),
              Error::EmptyHandleTopic);
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true,
                                       topics("h", "", s, p))),
              Error::EmptyHandleTopic);
    CHECK_ERR(Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true,
                                       topics("h", d, s, ""))),
              Error::EmptyHandleTopic);
}

// fast-cdr's `optional::value()` throws on an absent member; this library
// asks `has_value()` first, on every path.
TCN_SQE_TEST(an_absent_handle_is_never_dereferenced)
{
    StubOptional<rpc::SISStreamHandle>::touched_while_absent() = false;
    (void)Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_INACTIVE, false));
    (void)Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, false));
    (void)Handle::from_reply(reply(rpc::SIS_STATUS_REJECTED, rpc::SIS_RELATION_STATE_UNKNOWN, false));
    CHECK_MSG(!StubOptional<rpc::SISStreamHandle>::touched_while_absent(),
              "an absent optional handle was dereferenced");
}

TCN_SQE_TEST(handles_compare_by_the_handle_the_engine_issued)
{
    const Result<Handle> a = Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true,
                                                      topics("h42", "d", "s", "p")));
    const Result<Handle> b = Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_PENDING, true,
                                                      topics("h42", "d", "s", "p")));
    const Result<Handle> c = Handle::from_reply(reply(rpc::SIS_STATUS_OK, rpc::SIS_RELATION_STATE_ACTIVE, true,
                                                      topics("h43", "d", "s", "p")));
    CHECK_OK(a); CHECK_OK(b); CHECK_OK(c);
    if (!a || !b || !c) { return; }
    CHECK(a.value() == b.value());
    CHECK(a.value() != c.value());
}
