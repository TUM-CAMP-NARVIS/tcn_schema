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
#include <type_traits>
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
static_assert(RequestKey<rpc::SISRelationStreamStartRequest>::mode == RequestMode::Query, "");

// Every mode, pinned, because a wrong one is silent on the wire: a `put` to a
// key that carries only a queryable reaches nothing at all and nothing reports
// it. `SISEdgeUpdateRequest` was registered as `Publish` until 2026-09-02 --
// correct when written, wrong from B47 onward -- so the value being right today
// is not evidence that it will stay right, and only an assertion per key makes a
// future change deliberate rather than accidental. B73 then moved four more.
static_assert(RequestKey<rpc::SISLeaveRequest>::mode       == RequestMode::Publish, "");
static_assert(RequestKey<rpc::SISRelationStreamStopRequest>::mode == RequestMode::Query, "");
// The one B47 moved.
static_assert(RequestKey<rpc::SISEdgeUpdateRequest>::mode  == RequestMode::Query, "");
// The four B73 moved. A put on any of these keys is ignored by the engine, so a
// `Publish` here would be a client that never joins and is told so by nothing
// but an engine log line.
static_assert(RequestKey<rpc::SISJoinRequest>::mode        == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISNodeUpdateRequest>::mode  == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISNodeRemoveRequest>::mode  == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISEdgeRemoveRequest>::mode  == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISNodeRemoveRequest>::suffix == keys::kNodeRemove, "");
static_assert(RequestKey<rpc::SISEdgeRemoveRequest>::suffix == keys::kEdgeRemove, "");

// And the reply each one is answered with, pinned the same way. Every write --
// including the registry's -- answers SISMutationReply; each read has its own.
static_assert(std::is_void<RequestKey<rpc::SISLeaveRequest>::Reply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISJoinRequest>::Reply,       rpc::SISMutationReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISNodeUpdateRequest>::Reply, rpc::SISMutationReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISNodeRemoveRequest>::Reply, rpc::SISMutationReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISEdgeUpdateRequest>::Reply, rpc::SISMutationReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISEdgeRemoveRequest>::Reply, rpc::SISMutationReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISTargetMutationRequest>::Reply, rpc::SISMutationReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISRelationStreamStartRequest>::Reply,
                           rpc::SISRelationStreamReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISRelationStreamStopRequest>::Reply,
                           rpc::SISRelationStreamReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISGraphQueryRequest>::Reply,   rpc::SISGraphQueryReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISPatternQueryRequest>::Reply, rpc::SISPatternQueryReply>::value, "");
static_assert(std::is_same<RequestKey<rpc::SISTargetQueryRequest>::Reply,  rpc::SISTargetQueryReply>::value, "");

// The two reads and the registry write added in 0.9.0.
static_assert(RequestKey<rpc::SISPatternQueryRequest>::suffix == keys::kPatternQuery, "");
static_assert(RequestKey<rpc::SISTargetQueryRequest>::suffix == keys::kTargetQuery, "");
static_assert(RequestKey<rpc::SISTargetMutationRequest>::suffix == keys::kTargetUpdate, "");
static_assert(RequestKey<rpc::SISPatternQueryRequest>::mode == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISTargetQueryRequest>::mode == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISTargetMutationRequest>::mode == RequestMode::Query, "");
static_assert(RequestKey<rpc::SISRelationStreamStopRequest>::suffix == keys::kStreamStop, "");
static_assert(RequestKey<rpc::SISGraphQueryRequest>::suffix == keys::kGraphQuery, "");
// The read is answered. If this ever reads Publish again, `query_request` on it
// stops compiling and the only read the engine offers becomes unreachable from
// this library -- which is how it stayed unreachable until now.
static_assert(RequestKey<rpc::SISGraphQueryRequest>::mode == RequestMode::Query, "");

// The floor a consumer static_asserts against.
static_assert(kSessionBLevel >= 41, "");
static_assert(kSessionBLevel > kContractBLevel, "");
// The revision is a description, not a floor, and it is at least the entry that
// made every mutation a query: the table above is what it describes.
static_assert(kProtocolBLevel >= 78, "");

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
// Since B73 the leave is the only SIS request left to publish.
TCN_SQE_TEST(a_published_request_names_its_own_key_and_its_own_type)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    rpc::SISLeaveRequest leave;
    CHECK_OK(s.value().publish_request(leave));

    CHECK_INT_EQ(t.published.size(), 1);
    if (t.published.size() != 1) { return; }

    CHECK_STR_EQ(t.published[0].key, "tcn/loc/pcpd/hl2-01/sis/leave");
    CHECK_STR_EQ(t.published[0].encoding,
                 "application/cdr;tcnart_msgs::rpc::SISLeaveRequest;tcn_schema=0.9");
    CHECK_INT_EQ(t.published[0].payload.size(), 4);
    CHECK_INT_EQ(t.queried.size(), 0);
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
    rpc::SISLeaveRequest leave;
    CHECK_ERR(s.value().publish_request(leave), Error::TransportFailed);
}

TCN_SQE_TEST(a_request_the_codec_cannot_encode_never_reaches_the_wire)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    StubCodec::encode_fails = true;
    rpc::SISLeaveRequest leave;
    CHECK_ERR(s.value().publish_request(leave), Error::EncodeFailed);
    rpc::SISJoinRequest join;
    rpc::SISMutationReply reply;
    CHECK_ERR(s.value().query_request(join, reply), Error::EncodeFailed);
    StubCodec::encode_fails = false;
    CHECK_INT_EQ(t.published.size(), 0);
    CHECK_INT_EQ(t.queried.size(), 0);
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

QueryReply mutation_reply(std::uint8_t status, std::uint64_t epoch, const std::string& engine_id,
                          const char* encoding =
                              "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.9")
{
    QueryReply r;
    r.encoding = encoding;
    r.payload = rpc::SISMutationReply::stub_encode(status, epoch, engine_id);
    return r;
}

}  // namespace

// ---------------------------------------------------------------------------
// B73: every mutation is a query
// ---------------------------------------------------------------------------

// The join is the one that matters most. As a put it raced the first
// stream/start; as a query, ACCEPTED means stored and the start after it sees
// the fragment. The test asserts the whole round trip: the key, the
// annotation, that nothing was published, and that the reply handed back is
// the engine's -- including B74's engine id and epoch, which is how a client
// learns which liveliness token to watch.
TCN_SQE_TEST(a_join_goes_out_as_a_query_and_returns_the_decoded_reply)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.scripted_replies.push_back(mutation_reply(0, 1762300800123ull, "lab1"));

    rpc::SISJoinRequest join;
    rpc::SISMutationReply reply;
    CHECK_OK(s.value().query_request(join, reply));

    CHECK_INT_EQ(t.published.size(), 0);
    CHECK_INT_EQ(t.queried.size(), 1);
    if (t.queried.empty()) { return; }
    CHECK_STR_EQ(t.queried[0].key, "tcn/loc/pcpd/hl2-01/sis/join");
    CHECK_STR_EQ(t.queried[0].encoding,
                 "application/cdr;tcnart_msgs::rpc::SISJoinRequest;tcn_schema=0.9");
    CHECK_INT_EQ(t.queried[0].timeout_ms, kDefaultQueryTimeoutMs);

    CHECK_INT_EQ(reply.status(), 0);
    CHECK_STR_EQ(reply.engine_id(), "lab1");
    CHECK(reply.engine_epoch() == 1762300800123ull);
}

// A refused join is still a successful *query*: the transport delivered an
// answer. The session hands it back undisturbed, and reading the status is the
// caller's -- this library cannot name the generated enum's values.
TCN_SQE_TEST(a_rejected_join_is_a_delivered_reply_not_a_transport_failure)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.scripted_replies.push_back(mutation_reply(1, 42, "lab1"));
    rpc::SISJoinRequest join;
    rpc::SISMutationReply reply;
    CHECK_OK(s.value().query_request(join, reply));
    CHECK_INT_EQ(reply.status(), 1);
}

TCN_SQE_TEST(every_mutation_request_is_a_query_to_its_own_key)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    for (int i = 0; i < 6; ++i) { t.scripted_replies.push_back(mutation_reply(0, 7, "e")); }

    rpc::SISMutationReply reply;
    rpc::SISNodeUpdateRequest node_update;
    rpc::SISNodeRemoveRequest node_remove;
    rpc::SISEdgeUpdateRequest edge_update;
    rpc::SISEdgeRemoveRequest edge_remove;
    rpc::SISTargetMutationRequest target_update;
    rpc::SISJoinRequest join;
    CHECK_OK(s.value().query_request(node_update, reply));
    CHECK_OK(s.value().query_request(node_remove, reply));
    CHECK_OK(s.value().query_request(edge_update, reply));
    CHECK_OK(s.value().query_request(edge_remove, reply));
    CHECK_OK(s.value().query_request(target_update, reply));
    CHECK_OK(s.value().query_request(join, reply));

    CHECK_INT_EQ(t.published.size(), 0);
    CHECK_INT_EQ(t.queried.size(), 6);
    if (t.queried.size() != 6) { return; }
    CHECK_STR_EQ(t.queried[0].key, "tcn/loc/pcpd/hl2-01/sis/component/update");
    CHECK_STR_EQ(t.queried[1].key, "tcn/loc/pcpd/hl2-01/sis/component/remove");
    CHECK_STR_EQ(t.queried[2].key, "tcn/loc/pcpd/hl2-01/sis/relation/update");
    CHECK_STR_EQ(t.queried[3].key, "tcn/loc/pcpd/hl2-01/sis/relation/remove");
    CHECK_STR_EQ(t.queried[4].key, "tcn/loc/pcpd/hl2-01/sis/target/update");
    CHECK_STR_EQ(t.queried[5].key, "tcn/loc/pcpd/hl2-01/sis/join");
    CHECK_STR_EQ(t.queried[4].encoding,
                 "application/cdr;tcnart_msgs::rpc::SISTargetMutationRequest;tcn_schema=0.9");
}

// A mutation reply from an engine built against another tcn_schema minor is
// refused before it is decoded: SISMutationReply grew two fields in 0.9.0
// under an unchanged name, and that is exactly the change a name cannot carry.
// One from an engine that predates B76 and declares no version is accepted.
TCN_SQE_TEST(a_reply_from_another_schema_minor_is_refused_and_one_without_a_version_is_not)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    rpc::SISJoinRequest join;
    rpc::SISMutationReply reply;

    t.scripted_replies.push_back(mutation_reply(
        0, 1, "e", "application/cdr;tcnart_msgs::rpc::SISMutationReply;tcn_schema=0.8"));
    CHECK_ERR(s.value().query_request(join, reply), Error::SchemaVersionMismatch);
    CHECK_INT_EQ(reply.status(), -1);   // not decoded

    t.scripted_replies.push_back(mutation_reply(
        0, 1, "e", "application/cdr;tcnart_msgs::rpc::SISMutationReply"));
    CHECK_OK(s.value().query_request(join, reply));
    CHECK_INT_EQ(reply.status(), 0);
}

// The 0.9.0 reads: the pattern query (B63) and the registry read (B78), each to
// its own key and each with its own reply.
TCN_SQE_TEST(the_pattern_and_target_reads_go_to_their_own_keys)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.scripted_replies.push_back(
        reply_bytes("application/cdr;tcnart_msgs::rpc::SISPatternQueryReply;tcn_schema=0.9"));
    t.scripted_replies.push_back(
        reply_bytes("application/cdr;tcnart_msgs::rpc::SISTargetQueryReply;tcn_schema=0.9"));

    rpc::SISPatternQueryRequest pattern;
    rpc::SISPatternQueryReply pattern_reply;
    CHECK_OK(s.value().query_request(pattern, pattern_reply));

    rpc::SISTargetQueryRequest target;
    rpc::SISTargetQueryReply target_reply;
    CHECK_OK(s.value().query_request(target, target_reply));

    CHECK_INT_EQ(t.queried.size(), 2);
    if (t.queried.size() != 2) { return; }
    CHECK_STR_EQ(t.queried[0].key, "tcn/loc/pcpd/hl2-01/sis/pattern/query");
    CHECK_STR_EQ(t.queried[0].encoding,
                 "application/cdr;tcnart_msgs::rpc::SISPatternQueryRequest;tcn_schema=0.9");
    CHECK_STR_EQ(t.queried[1].key, "tcn/loc/pcpd/hl2-01/sis/target/query");
    CHECK_STR_EQ(t.queried[1].encoding,
                 "application/cdr;tcnart_msgs::rpc::SISTargetQueryRequest;tcn_schema=0.9");

    // A graph reply is not a pattern reply, though both open with
    // SISGraphQueryStatus and either decodes from the other's bytes.
    t.scripted_replies.push_back(
        reply_bytes("application/cdr;tcnart_msgs::rpc::SISGraphQueryReply;tcn_schema=0.9"));
    CHECK_ERR(s.value().query_request(pattern, pattern_reply), Error::WrongDeclaredType);
}

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
                 "application/cdr;tcnart_msgs::rpc::SISRelationStreamStartRequest;tcn_schema=0.9");

    // The engine's resolve bound is 30 s. A 5 s timeout -- one consumer's
    // wrapper default -- gives up on a request the engine will still answer, by
    // which time it may have registered a stream and attached this client id to
    // it, leaving a stream the client holds no handle for.
    CHECK_INT_EQ(t.queried[0].timeout_ms, kDefaultQueryTimeoutMs);
    CHECK_MSG(kDefaultQueryTimeoutMs > 30000, "the query timeout is below the engine's resolve bound");
}

// B46, and the reason this registration was worth adding: the engine has
// answered reads since B46, and this library could not reach that key at all --
// no suffix, no RequestKey entry, so `query_request` on it did not compile. The
// test is the usage, not the constant: it asserts the key the read goes to and
// the type it is annotated with, which is the whole of what the contract owns.
TCN_SQE_TEST(the_graph_read_goes_to_its_own_key_and_names_its_own_type)
{
    FakeTransport t;
    Result<Session> s = Session::open(t, client());
    CHECK_OK(s);
    if (!s) { return; }

    t.scripted_replies.push_back(reply_bytes("application/cdr;tcnart_msgs::rpc::SISGraphQueryReply"));

    rpc::SISGraphQueryRequest request;
    rpc::SISGraphQueryReply reply;
    CHECK_OK(s.value().query_request(request, reply));

    CHECK_INT_EQ(t.queried.size(), 1);
    if (t.queried.empty()) { return; }
    CHECK_STR_EQ(t.queried[0].key, "tcn/loc/pcpd/hl2-01/sis/graph/query");
    CHECK_STR_EQ(t.queried[0].encoding,
                 "application/cdr;tcnart_msgs::rpc::SISGraphQueryRequest;tcn_schema=0.9");

    // A read is answered, so it must not have gone out as a publish -- the
    // failure that would leave a caller waiting on a key nothing replies to.
    CHECK_INT_EQ(t.published.size(), 0);
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
