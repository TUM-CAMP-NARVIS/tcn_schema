// SPDX-License-Identifier: internal
//
// Transport — the whole of this library's contact with a network, in five
// operations.
//
// `sqe_session` implements the SIS protocol and links **no** Zenoh. That is a
// deliberate reversal of §2's "layer 2" and it exists for one reason: the two
// consumers move to new zenoh-cpp versions on their own schedules, and a
// zenoh-version-dependent header shipped from `tcn_schema` would make every
// such move a coordinated release of three repositories. What ships instead is
// this interface. It names what the protocol needs and nothing else, so the
// per-consumer adapter is the only thing a zenoh major version can break, it
// lives in the repository that owns that zenoh pin, and it is five functions
// long.
//
// Why each operation is here, and what was deliberately left out, is argued in
// the block above each declaration. The count is the design: every operation
// added here is added to two adapters and re-verified on every zenoh bump.
#ifndef TCN_SQE_TRANSPORT_H
#define TCN_SQE_TRANSPORT_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"

namespace tcn {
namespace sqe {

/// A borrowed, contiguous byte range.
///
/// Not `std::span` (C++20) and not `std::vector` (an owner). Borrowed because
/// every use here is a call argument that the callee either writes to the wire
/// or copies; making it an owner would force an allocation per publish on a
/// path that a tracking loop runs at frame rate.
struct BytesView
{
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;

    BytesView() = default;
    BytesView(const std::uint8_t* d, std::size_t n) noexcept : data(d), size(n) {}

    /// From any contiguous byte container: `std::vector<std::uint8_t>`, and
    /// also the `std::vector<char>` some CDR buffers hand back.
    template <class Container>
    static BytesView of(const Container& c) noexcept
    {
        return BytesView(reinterpret_cast<const std::uint8_t*>(c.data()), c.size());
    }

    bool empty() const noexcept { return size == 0; }
};

/// What a query answered with: the reply's own encoding annotation and its
/// bytes.
///
/// The encoding is carried because it is the *only* thing that distinguishes
/// "CDR carrying the type I asked for" from "CDR carrying something else", and
/// dropping it here is precisely the read-side hole `check_declared_type`
/// exists to close. An adapter that cannot produce the reply's encoding must
/// leave it empty rather than invent one; the empty string fails the check,
/// which is the correct outcome.
struct QueryReply
{
    std::string encoding;
    std::vector<std::uint8_t> payload;
};

/// One sample delivered to a subscriber. Every field is borrowed and valid
/// only for the duration of the callback.
struct Sample
{
    std::string_view key;
    std::string_view encoding;
    BytesView payload;
};

/// Called on whatever thread the adapter delivers samples on. The library
/// makes no claim about which thread that is and holds no lock across it.
using SampleFn = std::function<void(const Sample&)>;

/// An opaque receipt for something the transport declared on this library's
/// behalf. Zero is "nothing"; every other value is the adapter's to choose.
///
/// One id space for both subscribers and liveliness tokens, so that
/// `undeclare` is one operation rather than two. An adapter that needs to tell
/// them apart can encode a tag in the high bits — that is its business, not
/// this interface's.
struct Registration
{
    std::uint64_t id = 0;
    bool valid() const noexcept { return id != 0; }
};

/// Everything the SIS protocol asks of a network.
///
/// Implement this over the zenoh session the consuming repository already has.
/// It is an abstract class rather than a template parameter on purpose: the
/// adapter is written once per repository and the protocol code is not
/// performance-critical at this boundary — one indirect call per protocol
/// message, none per pose — so the cost of a vtable buys the ability to
/// substitute a fake in a unit test with no daemon, which is what makes the
/// lease table testable at all.
///
/// **Thread safety.** Nothing in `sqe_session` is internally synchronized. The
/// owner serializes calls, exactly as the contract core requires (§3.2).
/// `SampleFn` is the one exception and is documented above.
///
/// **No exceptions.** Every operation returns a `Result`. An adapter whose
/// underlying API throws must catch at this boundary; nothing above it is
/// written to survive an exception.
class Transport
{
public:
    virtual ~Transport() = default;

    Transport() = default;
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;

    // -- 1 -------------------------------------------------------------------
    /// Put `payload` on `key`, annotated `encoding`.
    ///
    /// Six of the eight SIS requests are one-way puts the daemon never answers
    /// (B27), so a publish that could not also return a reply is exactly the
    /// right shape for them: it makes "published, not accepted" the only thing
    /// this call can mean.
    ///
    /// `encoding` is `application/cdr;<type>` and is derived from the message
    /// type by `cdr_encoding<T>()`, never spelled by a caller. An adapter must
    /// set it verbatim on the wire: the engine reads it, and an adapter that
    /// drops it into a Zenoh *attachment* instead — as one consumer does today
    /// — is the B2 failure, unobservable from either end.
    ///
    /// Deliberately no congestion, priority, reliability or express parameter.
    /// Those are the most zenoh-version-unstable part of the API, the protocol
    /// does not depend on any of them, and an adapter is free to choose them.
    virtual Result<void> publish(std::string_view key,
                                 std::string_view encoding,
                                 BytesView payload) = 0;

    // -- 2 -------------------------------------------------------------------
    /// Issue `payload` as a query on `key` and wait for one reply.
    ///
    /// The two relation-stream requests are the only SIS operations that are
    /// answered, and a client cannot subscribe to anything without the handle
    /// the answer carries, so a blocking single-reply query is the whole
    /// requirement. Streaming replies, reply handlers and futures are all
    /// zenoh-version-unstable and none is needed.
    ///
    /// `timeout_ms` is a parameter and not an adapter default because getting
    /// it wrong is a named failure: the engine's resolve bound is 30 s, and one
    /// consumer's query wrapper defaults to 5 s. Giving up early does not undo
    /// the request — the engine may already have registered a stream and
    /// attached this client id to it, leaving a stream running that the client
    /// holds no handle for and cannot stop until it leaves.
    ///
    /// Returns `Error::Timeout` when the bound elapsed with no reply, and
    /// `Error::TransportFailed` for anything else.
    virtual Result<void> query(std::string_view key,
                               std::string_view encoding,
                               BytesView payload,
                               std::uint32_t timeout_ms,
                               QueryReply& out) = 0;

    // -- 3 -------------------------------------------------------------------
    /// Declare a liveliness token on `key` and hold it until `undeclare`.
    ///
    /// B41. Nothing is ever put on `{client}/sis/alive`: declaring the token
    /// *is* the message, and a client that publishes there instead is invisible
    /// to the engine while looking, locally, like it announced itself. This is
    /// a distinct operation from `publish` because it cannot be expressed as
    /// one — the payload is the declaration's existence, not any bytes.
    ///
    /// It is separate from `declare_subscriber` because the two are different
    /// Zenoh primitives with different failure modes, and folding them into a
    /// generic `declare(kind, key)` would only move the switch into the adapter.
    virtual Result<Registration> declare_liveliness_token(std::string_view key) = 0;

    // -- 4 -------------------------------------------------------------------
    /// Subscribe to `key`, delivering samples to `on_sample` until `undeclare`.
    ///
    /// A relation stream's `{handle}/status` is the only channel on which a
    /// client learns that a stream it holds went `NoPath` or that the engine
    /// stopped answering, and those two are not distinguishable any other way.
    /// The stream's *data* topic is also subscribed with this, though a
    /// consumer that already routes pose data through its own subscriber may
    /// use its own — the library does not decode payloads.
    ///
    /// Owning the subscription here is what lets a lease's retirement undeclare
    /// it: without it, the last release stops the stream and leaves a
    /// subscriber alive on a key nothing publishes on any more.
    virtual Result<Registration> declare_subscriber(std::string_view key,
                                                    SampleFn on_sample) = 0;

    // -- 5 -------------------------------------------------------------------
    /// Drop whatever `r` names. A no-op for an invalid or already-dropped
    /// registration, and never fails: it runs from destructors.
    ///
    /// One operation rather than `undeclare_token` plus `undeclare_subscriber`
    /// because the caller always knows which it holds and the adapter always
    /// knows what it issued, so a second entry point would carry no information
    /// either side did not already have.
    virtual void undeclare(Registration r) noexcept = 0;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_TRANSPORT_H
