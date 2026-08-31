// SPDX-License-Identifier: internal
//
// The session: presence before anything else, and a request type that cannot
// reach the wrong key or carry the wrong annotation.
//
// The generated message types are *defined by this test* (stub_msgs.h) rather
// than included, because the library only forward-declares them. That is the
// dependency-free claim, asserted by construction: if any of this needed a
// generated header, none of it would compile.
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "check.h"
#include "fake_transport.h"
#include "stub_msgs.h"
#include "tcn/sqe/session.h"

using namespace tcn::sqe;
using tcn::sqe::test::DeclaredThing;
using tcn::sqe::test::FakeTransport;
using tcn::sqe::test::StubCodec;

namespace rpc = ::tcnart_msgs::rpc;

namespace {

using Session = SisSession<StubCodec>;

ClientId client() { return ClientId::parse("tcn/loc/pcpd/hl2-01").value(); }

}  // namespace

// ---------------------------------------------------------------------------
// Compile-time: a request goes to exactly one key, and only in its own mode
// ---------------------------------------------------------------------------

static_assert(RequestKey<rpc::SISJoinRequest>::suffix == keys::kJoin, "");
static_assert(RequestKey<rpc::SISLeaveRequest>::suffix == keys::kLeave, "");
static_assert(RequestKey<rpc::SISNodeUpdateRequest>::suffix == keys::kNodeUpdate, "");
static_assert(RequestKey<rpc::SISEdgeUpdateRequest>::suffix == keys::kEdgeUpdate, "");
static_assert(RequestKey<rpc::SISJoinRequest>::mode == RequestMode::Publish, "");
static_assert(RequestKey<rpc::SISRelationStreamStartRequest>::mode == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISRelationStreamStopRequest>::suffix == keys::kStreamStop, "");

// The floor a consumer static_asserts against.
static_assert(kSessionBLevel >= 41, "");
static_assert(kSessionBLevel > kContractBLevel, "");

// ---------------------------------------------------------------------------
// B41: the token, and nothing before it
// ---------------------------------------------------------------------------

// Declaring the token IS the message. A client that publishes on
// `{client}/sis/alive` instead is invisible to the engine while looking,
// locally, like it announced itself -- so the test asserts both halves: the
// token is declared, and nothing was published.
TCN_SQE_TEST(opening_a_session_declares_a_liveliness_token_and_publishes_nothing)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    CHECK_INT_EQ(t.published.size(), 0);
    CHECK_INT_EQ(t.live_count(DeclaredThing::Token), 1);

    const DeclaredThing* d = t.find_declared("tcn/loc/pcpd/hl2-01/sis/alive");
    CHECK(d != nullptr);
    if (d) { CHECK(d->kind == DeclaredThing::Token); }
    CHECK(s.value().is_present());
}

// There is no session without a token, and therefore no code path in this
// library that speaks to the engine without one.
TCN_SQE_TEST(a_session_cannot_be_opened_without_a_token)
{
    FakeTransport t;
    t.token_fails = true;
    const Result<Session> s = Session::open(t, client());
    CHECK_ERR(s, Error::PresenceUnavailable);
    CHECK_INT_EQ(t.live_count(DeclaredThing::Token), 0);
}

TCN_SQE_TEST(closing_a_session_undeclares_its_token)
{
    FakeTransport t;
    {
        Result<Session> s = Session::open(t, client());
        CHECK_OK(s);
        CHECK_INT_EQ(t.live_count(DeclaredThing::Token), 1);
    }
    CHECK_INT_EQ(t.live_count(DeclaredThing::Token), 0);
    CHECK_INT_EQ(t.declared.size(), 1);   // undeclared, not re-declared
}

// Moving a session must not drop the token: the token's lifetime is the
// client's presence, not any particular object's address.
TCN_SQE_TEST(moving_a_session_carries_the_token_and_drops_it_once)
{
    FakeTransport t;
    {
        Result<Session> s = Session::open(t, client());
        CHECK_OK(s);
        if (!s) { return; }
        Session moved(std::move(s.value()));
        CHECK(moved.is_present());
        CHECK_INT_EQ(t.live_count(DeclaredThing::Token), 1);
    }
    CHECK_INT_EQ(t.live_count(DeclaredThing::Token), 0);
}

// ---------------------------------------------------------------------------
// The key and the annotation come from the type
// ---------------------------------------------------------------------------

// B2 and B3 together: the caller passes a type, so it cannot spell the key and
// cannot spell the annotation, and payload and annotation cannot disagree.
TCN_SQE_TEST(a_published_request_names_its_own_key_and_its_own_type)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    rpc::SISJoinRequest join;
    CHECK_OK(s.value().publish_request(join));

    rpc::SISLeaveRequest leave;
    CHECK_OK(s.value().publish_request(leave));

    CHECK_INT_EQ(t.published.size(), 2);
    if (t.published.size() != 2) { return; }

    CHECK_STR_EQ(t.published[0].key, "tcn/loc/pcpd/hl2-01/sis/join");
    CHECK_STR_EQ(t.published[0].encoding, "application/cdr;tcnart_msgs::rpc::SISJoinRequest");
    CHECK_INT_EQ(t.published[0].payload.size(), 4);

    CHECK_STR_EQ(t.published[1].key, "tcn/loc/pcpd/hl2-01/sis/leave");
    CHECK_STR_EQ(t.published[1].encoding, "application/cdr;tcnart_msgs::rpc::SISLeaveRequest");
}

TCN_SQE_TEST(each_mutation_request_reaches_its_own_key)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    rpc::SISNodeUpdateRequest node;
    rpc::SISEdgeUpdateRequest edge;
    CHECK_OK(s.value().publish_request(node));
    CHECK_OK(s.value().publish_request(edge));

    CHECK_INT_EQ(t.published.size(), 2);
    if (t.published.size() != 2) { return; }
    CHECK_STR_EQ(t.published[0].key, "tcn/loc/pcpd/hl2-01/sis/component/update");
    CHECK_STR_EQ(t.published[1].key, "tcn/loc/pcpd/hl2-01/sis/relation/update");
}

// A publish that the transport refused is reported, not swallowed. It still
// means only "published" on success -- the daemon does not answer these (B27).
TCN_SQE_TEST(a_publish_reports_the_transports_refusal)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.publish_error = Error::TransportFailed;
    rpc::SISJoinRequest join;
    CHECK_ERR(s.value().publish_request(join), Error::TransportFailed);
}

TCN_SQE_TEST(a_request_the_codec_cannot_encode_never_reaches_the_wire)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    StubCodec::encode_fails = true;
    rpc::SISJoinRequest join;
    CHECK_ERR(s.value().publish_request(join), Error::EncodeFailed);
    StubCodec::encode_fails = false;
    CHECK_INT_EQ(t.published.size(), 0);
}

// ---------------------------------------------------------------------------
// The query path, and the reply's declared type
// ---------------------------------------------------------------------------

namespace {

QueryReply reply_bytes(const char* encoding)
{
    QueryReply r;
    r.encoding = encoding;
    r.payload = std::vector<std::uint8_t>{1, 2, 3, 4};
    return r;
}

}  // namespace

TCN_SQE_TEST(a_query_goes_to_the_stream_key_with_a_timeout_above_the_engines_resolve_bound)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.scripted_replies.push_back(reply_bytes("application/cdr;tcnart_msgs::rpc::SISRelationStreamReply"));

    rpc::SISRelationStreamStartRequest start;
    rpc::SISRelationStreamReply reply;
    CHECK_OK(s.value().query_request(start, reply));

    CHECK_INT_EQ(t.queried.size(), 1);
    if (t.queried.empty()) { return; }
    CHECK_STR_EQ(t.queried[0].key, "tcn/loc/pcpd/hl2-01/sis/stream/start");
    CHECK_STR_EQ(t.queried[0].encoding,
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStartRequest");

    // The engine's resolve bound is 30 s. A 5 s timeout -- one consumer's
    // wrapper default -- gives up on a request the engine will still answer, by
    // which time it may have registered a stream and attached this client id to
    // it, leaving a stream the client holds no handle for.
    CHECK_INT_EQ(t.queried[0].timeout_ms, kDefaultQueryTimeoutMs);
    CHECK_MSG(kDefaultQueryTimeoutMs > 30000, "the query timeout is below the engine's resolve bound");
}

// CDR is not self-describing: a payload of the wrong type may decode into the
// reply struct cleanly and mean something else. Treating any `application/cdr`
// as acceptable -- which one consumer's wrapper does today -- cannot tell those
// two apart at all.
TCN_SQE_TEST(a_reply_that_declares_another_type_is_refused_before_it_is_decoded)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    rpc::SISRelationStreamStartRequest start;
    rpc::SISRelationStreamReply reply;

    t.scripted_replies.push_back(reply_bytes("application/cdr;tcnart_msgs::rpc::SISJoinRequest"));
    CHECK_ERR(s.value().query_request(start, reply), Error::WrongDeclaredType);

    // An unannotated CDR payload is refused too. The declaration is mandatory.
    t.scripted_replies.push_back(reply_bytes("application/cdr"));
    CHECK_ERR(s.value().query_request(start, reply), Error::MissingDeclaredType);

    // ... including when the adapter could not produce an encoding at all.
    t.scripted_replies.push_back(reply_bytes(""));
    CHECK_ERR(s.value().query_request(start, reply), Error::UnknownEncoding);
}

TCN_SQE_TEST(a_query_that_timed_out_reports_a_timeout_and_not_a_failure)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    rpc::SISRelationStreamStopRequest stop;
    rpc::SISRelationStreamReply reply;
    CHECK_ERR(s.value().query_request(stop, reply), Error::Timeout);   // no scripted reply
}

TCN_SQE_TEST(a_reply_of_the_right_type_that_will_not_decode_is_reported_as_such)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.scripted_replies.push_back(reply_bytes("application/cdr;tcnart_msgs::rpc::SISRelationStreamReply"));
    StubCodec::decode_fails = true;
    rpc::SISRelationStreamStartRequest start;
    rpc::SISRelationStreamReply reply;
    CHECK_ERR(s.value().query_request(start, reply), Error::DecodeFailed);
    StubCodec::decode_fails = false;
}

// ---------------------------------------------------------------------------
// Subscriptions
// ---------------------------------------------------------------------------

// The subscriber's lifetime is the `Declaration`'s. Without that, the last
// lease release stops the stream and leaves a subscriber alive on a key nothing
// publishes on any more.
TCN_SQE_TEST(a_subscription_lives_exactly_as_long_as_its_declaration)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    int samples = 0;
    {
        Result<Declaration> sub = s.value().subscribe(
            "sqe/eng/rel/h42/status", [&samples](const Sample&) { ++samples; });
        CHECK_OK(sub);
        CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 1);
        CHECK(t.deliver("sqe/eng/rel/h42/status", "application/cdr;x", "payload"));
        CHECK_INT_EQ(samples, 1);
    }
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 0);
    CHECK(!t.deliver("sqe/eng/rel/h42/status", "application/cdr;x", "payload"));
    CHECK_INT_EQ(samples, 1);
}

TCN_SQE_TEST(a_subscriber_the_transport_refused_is_not_reported_as_held)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.subscriber_fails = true;
    const Result<Declaration> sub = s.value().subscribe("sqe/eng/rel/h42/status", SampleFn());
    CHECK_ERR(sub, Error::TransportFailed);
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 0);
}

TCN_SQE_TEST(a_declaration_undeclares_at_most_once)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    Result<Declaration> sub = s.value().subscribe("k", SampleFn());
    CHECK_OK(sub);
    if (!sub) { return; }
    CHECK(sub.value().held());
    sub.value().undeclare();
    CHECK(!sub.value().held());
    sub.value().undeclare();
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 0);
}
