// SPDX-License-Identifier: internal
//
// SisSession — presence, and the typed request/reply path.
//
// One include for the `sqe_session` component. It brings in the whole contract
// core, the transport interface, the relation terms and the lease table.
//
// This component links no Zenoh and names no zenoh type. See transport.h for
// why. The consumer implements `Transport` over the session it already has,
// supplies a codec, and gets the protocol from here.
#ifndef TCN_SQE_SESSION_H
#define TCN_SQE_SESSION_H

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "tcn/sqe/contract.h"
#include "tcn/sqe/lease.h"
#include "tcn/sqe/relation.h"
#include "tcn/sqe/transport.h"

namespace tcn {
namespace sqe {

/// Whether a request is answered.
enum class RequestMode
{
    /// A one-way put. The daemon does not answer it, and "published" is the
    /// strongest thing the return value may be read to mean (B27).
    Publish,
    /// A query with exactly one reply.
    Query,
};

/// Which key a request type goes to, and whether it is answered.
///
/// **No primary definition, on purpose**, the same discipline as `WireType`:
/// a request type nobody registered does not compile, rather than being
/// published to a key chosen by whichever overload matched. The pairing of type
/// to key is the thing worth making unforgeable — a `SISLeaveRequest` put on
/// the join key decodes cleanly at the daemon, means the opposite of what was
/// intended, and is answered by nothing.
template <class T>
struct RequestKey;

#define TCN_SQE_REGISTER_REQUEST_KEY(TYPE, SUFFIX, MODE)          \
    template <>                                                   \
    struct RequestKey<TYPE>                                       \
    {                                                             \
        static constexpr std::string_view suffix = SUFFIX;        \
        static constexpr RequestMode mode = MODE;                 \
    }

TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISJoinRequest,       keys::kJoin,       RequestMode::Publish);
TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISLeaveRequest,      keys::kLeave,      RequestMode::Publish);
TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISNodeUpdateRequest, keys::kNodeUpdate, RequestMode::Publish);
TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISNodeRemoveRequest, keys::kNodeRemove, RequestMode::Publish);
TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISEdgeUpdateRequest, keys::kEdgeUpdate, RequestMode::Publish);
TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISEdgeRemoveRequest, keys::kEdgeRemove, RequestMode::Publish);
TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISRelationStreamStartRequest, keys::kStreamStart, RequestMode::Query);
TCN_SQE_REGISTER_REQUEST_KEY(::tcnart_msgs::rpc::SISRelationStreamStopRequest,  keys::kStreamStop,  RequestMode::Query);

namespace detail {

/// Whether `C` can encode a `T` and decode a `T`.
///
/// Duck-typed rather than an abstract `Codec` base, because serialisation is a
/// template operation over the generated types and a virtual cannot be. It is
/// the second and last customisation point: transport, and bytes.
template <class C, class T, class = void>
struct has_codec_shape : std::false_type {};

template <class C, class T>
struct has_codec_shape<C, T, std::void_t<
    decltype(C::encode(std::declval<const T&>(), std::declval<std::vector<std::uint8_t>&>())),
    decltype(C::decode(std::declval<BytesView>(), std::declval<T&>()))>> : std::true_type {};

}  // namespace detail

/// The **floor** for the session layer, read the same way as
/// `kContractBLevel`: the highest entry in the engine's
/// `compatibility-breaks.md` such that every obligation at or below it that
/// falls to a client is met by `sqe_contract` plus `sqe_session`.
///
/// It is 41, not 43:
///   * B36 — the local refcount for shared handles — is implemented here, by
///     `RelationLeaseTable`, which is what let the floor move past 35;
///   * B41 — the liveliness token — is implemented here, by `declare_presence`,
///     and `SisSession` cannot be opened without one;
///   * B42 concerns the engine's own example clients and CLI, so no client-side
///     obligation exists to meet and the floor stops below it rather than
///     claiming something this library does not do;
///   * B43 — the ASCII naming rule — *is* met, in `validate.h`, above the
///     floor. A floor is not a boast.
inline constexpr int kSessionBLevel = 41;

/// The query timeout, in milliseconds.
///
/// The engine's resolve bound is 30 s. This is deliberately above it, and
/// deliberately not the 5 s one consumer's query wrapper defaults to: a client
/// that gives up early does not withdraw its request, so the engine may still
/// register the stream and attach this client id to it — leaving a stream
/// running that the client holds no handle for and cannot stop until it leaves.
inline constexpr std::uint32_t kDefaultQueryTimeoutMs = 35000;

/// RAII over one thing the transport declared for us: a liveliness token or a
/// subscriber.
///
/// One class for both, because the lifetime is the whole of what this library
/// does with either — declare it, hold it, drop it exactly once — and a second
/// class would differ only in the factory that produced it. Move-only; there is
/// no way to construct one except by declaring.
class Declaration
{
public:
    Declaration() noexcept : t_(nullptr), reg_() {}

    Declaration(const Declaration&) = delete;
    Declaration& operator=(const Declaration&) = delete;

    Declaration(Declaration&& o) noexcept : t_(o.t_), reg_(o.reg_) { o.t_ = nullptr; o.reg_ = Registration{}; }

    Declaration& operator=(Declaration&& o) noexcept
    {
        if (this != &o)
        {
            release();
            t_ = o.t_; reg_ = o.reg_;
            o.t_ = nullptr; o.reg_ = Registration{};
        }
        return *this;
    }

    ~Declaration() { release(); }

    bool held() const noexcept { return t_ != nullptr && reg_.valid(); }

    /// Drop it now rather than at the end of the scope. Idempotent.
    void undeclare() noexcept { release(); }

private:
    friend Result<Declaration> declare_presence(Transport&, const ClientId&);
    friend Result<Declaration> declare_subscription(Transport&, std::string_view, SampleFn);

    Declaration(Transport* t, Registration r) noexcept : t_(t), reg_(r) {}

    void release() noexcept
    {
        if (t_ && reg_.valid()) { t_->undeclare(reg_); }
        t_ = nullptr;
        reg_ = Registration{};
    }

    Transport* t_;
    Registration reg_;
};

/// The liveliness token, held for as long as the client is present.
///
/// Nothing is ever put on `{client}/sis/alive`. Declaring the token *is* the
/// message (B41), and a client that publishes there instead is invisible to the
/// engine while looking, locally, like it announced itself.
///
/// `SisSession::open` calls this first, before anything else it does, so that
/// there is no code path in this library that speaks to the engine without a
/// token held.
inline Result<Declaration> declare_presence(Transport& t, const ClientId& id)
{
    const Result<Registration> r = t.declare_liveliness_token(id.alive_key());
    if (!r || !r.value().valid()) { return Result<Declaration>::fail(Error::PresenceUnavailable); }
    return Result<Declaration>::ok(Declaration(&t, r.value()));
}

/// A subscriber, held until the returned `Declaration` is destroyed.
///
/// The one this library exists to make correct is `{handle}/status`: it is the
/// only channel on which a client learns that a stream it holds went `NoPath`,
/// or that the engine stopped answering, and those two are not distinguishable
/// any other way. It belongs inside `RelationLeaseTable::StartFn` — which runs
/// exactly on the 0 -> 1 transition — with the `Declaration` kept beside the
/// lease and dropped by `StopFn`, so that the last release both stops the
/// stream and takes the subscriber down with it. Undeclaring it anywhere else
/// leaves a subscriber alive on a key nothing publishes on any more.
inline Result<Declaration> declare_subscription(Transport& t, std::string_view key, SampleFn on_sample)
{
    const Result<Registration> r = t.declare_subscriber(key, std::move(on_sample));
    if (!r) { return Result<Declaration>::fail(r.error()); }
    if (!r.value().valid()) { return Result<Declaration>::fail(Error::TransportFailed); }
    return Result<Declaration>::ok(Declaration(&t, r.value()));
}

/// A client's SIS session: a validated client id, a held liveliness token, and
/// the typed request path.
///
/// `Codec` is a class with two static member templates:
///
/// ```cpp
/// struct MyCodec {
///   template <class T> static Result<void> encode(const T&, std::vector<std::uint8_t>&);
///   template <class T> static Result<void> decode(BytesView, T&);
/// };
/// ```
///
/// Wiring the relation-stream half takes about a dozen lines and is not done
/// for the consumer, because it is the one place the generated IDL types have
/// to be named and this component names none:
///
/// ```cpp
/// RelationLeaseTable relations(
///   [&](const RelationStreamRequest& r) -> Result<Handle> {
///     tcnart_msgs::rpc::SISRelationStreamStartRequest req = to_idl(r);   // yours
///     tcnart_msgs::rpc::SISRelationStreamReply reply;
///     const auto q = session.query_request(req, reply);
///     if (!q) { return Result<Handle>::fail(q.error()); }
///     return Handle::from_reply(reply);           // enforces the invariant
///   },
///   [&](const Handle& h) -> Result<Status> {
///     tcnart_msgs::rpc::SISRelationStreamStopRequest req;
///     req.handle(h.data_topic());
///     tcnart_msgs::rpc::SISRelationStreamReply reply;
///     const auto q = session.query_request(req, reply);
///     if (!q) { return Result<Status>::fail(q.error()); }
///     return reply_status(reply);
///   });
/// ```
///
/// Not internally synchronized. The owner serializes.
template <class Codec>
class SisSession
{
public:
    /// The only way to open one. Declares the liveliness token *first*: there
    /// is no code path in this library that speaks to the engine without one.
    static Result<SisSession> open(Transport& t, ClientId id,
                                   std::uint32_t query_timeout_ms = kDefaultQueryTimeoutMs)
    {
        Result<Declaration> p = declare_presence(t, id);
        if (!p) { return Result<SisSession>::fail(p.error()); }
        return Result<SisSession>::ok(
            SisSession(&t, std::move(id), std::move(p.value()), query_timeout_ms));
    }

    SisSession(const SisSession&) = delete;
    SisSession& operator=(const SisSession&) = delete;
    SisSession(SisSession&&) = default;
    SisSession& operator=(SisSession&&) = default;

    const ClientId& client() const noexcept { return id_; }
    bool is_present() const noexcept { return presence_.held(); }
    std::uint32_t query_timeout_ms() const noexcept { return timeout_ms_; }
    Transport& transport() const noexcept { return *t_; }

    /// Publish a one-way request.
    ///
    /// The key comes from `RequestKey<Req>` and the encoding from
    /// `WireType<Req>`, so neither can be spelled by a caller and neither can
    /// disagree with the payload (B2, B3).
    ///
    /// Success means **published**, never accepted. The daemon does not answer
    /// these, and no return value of this function can be read to say it did
    /// (B27).
    template <class Req>
    Result<void> publish_request(const Req& req)
    {
        static_assert(detail::has_codec_shape<Codec, Req>::value,
                      "the Codec must provide encode(const T&, std::vector<uint8_t>&) "
                      "and decode(BytesView, T&)");
        static_assert(RequestKey<Req>::mode == RequestMode::Publish,
                      "this request is answered; use query_request");

        std::vector<std::uint8_t> bytes;
        const Result<void> enc = Codec::encode(req, bytes);
        if (!enc) { return Result<void>::fail(Error::EncodeFailed); }

        const std::string key = detail::concat(id_.str(), RequestKey<Req>::suffix);
        return t_->publish(key, cdr_encoding<Req>(), BytesView::of(bytes));
    }

    /// Send a request and decode its reply, refusing a reply that declares a
    /// type other than the one asked for.
    ///
    /// The declared-type check is not optional and not a warning. CDR is not
    /// self-describing: a payload of the wrong type may decode into `Reply`
    /// cleanly and mean something else entirely, and one consumer's query
    /// wrapper today treats *any* `application/cdr;...` as acceptable, which
    /// cannot distinguish those two cases at all.
    template <class Req, class Reply>
    Result<void> query_request(const Req& req, Reply& out)
    {
        static_assert(detail::has_codec_shape<Codec, Req>::value, "Codec cannot encode this request");
        static_assert(detail::has_codec_shape<Codec, Reply>::value, "Codec cannot decode this reply");
        static_assert(RequestKey<Req>::mode == RequestMode::Query,
                      "this request is not answered; use publish_request");

        std::vector<std::uint8_t> bytes;
        const Result<void> enc = Codec::encode(req, bytes);
        if (!enc) { return Result<void>::fail(Error::EncodeFailed); }

        const std::string key = detail::concat(id_.str(), RequestKey<Req>::suffix);
        QueryReply reply;
        const Result<void> q = t_->query(key, cdr_encoding<Req>(), BytesView::of(bytes),
                                         timeout_ms_, reply);
        if (!q) { return q; }

        const Result<void> typed = check_declared_type<Reply>(reply.encoding);
        if (!typed) { return typed; }

        const Result<void> dec = Codec::decode(BytesView::of(reply.payload), out);
        if (!dec) { return Result<void>::fail(Error::DecodeFailed); }
        return Result<void>::ok();
    }

    /// Declare a subscriber through this session's transport. See
    /// `declare_subscription`.
    Result<Declaration> subscribe(std::string_view key, SampleFn on_sample)
    {
        return declare_subscription(*t_, key, std::move(on_sample));
    }

private:
    SisSession(Transport* t, ClientId id, Declaration p, std::uint32_t timeout) noexcept
        : t_(t), id_(std::move(id)), presence_(std::move(p)), timeout_ms_(timeout) {}

    Transport* t_;
    ClientId id_;
    Declaration presence_;
    std::uint32_t timeout_ms_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_SESSION_H
