// SPDX-License-Identifier: internal
//
// The terms of a relation stream, as plain C++ values.
//
// These mirror `SISFrameRef`, `SRStreamTrigger` and
// `SISRelationStreamStartRequest` from `tcnart_msgs/rpc/SIS.idl`, and they are
// *not* those types. Two reasons, both load-bearing:
//
//   * the generated trigger is an IDL union whose case accessors throw when the
//     discriminator does not match (§3.3), and the lease table compares
//     triggers on every acquire — a comparison that throws is not a comparison
//     this library can make;
//   * mirroring keeps `sqe_session` compilable and testable with no fastddsgen
//     toolchain present, which is how the lease arithmetic gets tested at all.
//
// The consumer converts these into the generated request in its own start
// function, which is the same place it serialises. That is a dozen lines it
// writes once.
#ifndef TCN_SQE_RELATION_H
#define TCN_SQE_RELATION_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/validate.h"

namespace tcn {
namespace sqe {

/// `SRStreamTriggerKind`, with the ordinals pinned to the IDL's declaration
/// order for the same reason `Status`'s are.
enum class TriggerKind : std::int32_t
{
    FixedRate = 0,  ///< a timer; sample at "now"
    OnStream = 1,   ///< a named stream; sample at its stamp
    OnAny = 2,      ///< any input on the path; sample at the arriving stamp
};

/// When a relation stream is evaluated.
///
/// A closed set with three named constructors and no default: a
/// default-constructed trigger would be a fixed rate of zero hertz, which is
/// a request the engine rejects and a value nothing in a call site draws the
/// eye to.
class StreamTrigger
{
public:
    /// A timer at `hz`. Every edge on the path is extrapolated to the present.
    static Result<StreamTrigger> fixed_rate(double hz)
    {
        // Written as a positive test so that NaN — which compares false against
        // everything — is refused rather than admitted by a `<= 0` test.
        if (!(hz > 0.0) || !(hz < 1e9)) { return Result<StreamTrigger>::fail(Error::InvalidTriggerRate); }
        return Result<StreamTrigger>::ok(StreamTrigger(TriggerKind::FixedRate, hz, std::string()));
    }

    /// Evaluated whenever `topic` produces, at that sample's stamp; the other
    /// edges are interpolated to it.
    static Result<StreamTrigger> on_stream(std::string_view topic)
    {
        if (topic.empty()) { return Result<StreamTrigger>::fail(Error::EmptyTriggerTopic); }
        return Result<StreamTrigger>::ok(StreamTrigger(TriggerKind::OnStream, 0.0, std::string(topic)));
    }

    /// Evaluated on any input on the path.
    static StreamTrigger on_any() { return StreamTrigger(TriggerKind::OnAny, 0.0, std::string()); }

    TriggerKind kind() const noexcept { return kind_; }

    /// Meaningful only when `kind() == FixedRate`; zero otherwise. Returned
    /// rather than refused, because unlike the generated union there is no
    /// exception to avoid and a caller that reads the wrong member gets a
    /// value it can see is wrong.
    double hz() const noexcept { return hz_; }

    /// Meaningful only when `kind() == OnStream`; empty otherwise.
    const std::string& topic() const noexcept { return topic_; }

    /// Exact equality, including on the `double`.
    ///
    /// Not an epsilon. "The same terms" here means "the same request", and two
    /// components asking for 30 Hz and 30.000001 Hz have asked for different
    /// streams — the engine compiles them differently. An epsilon would silently
    /// hand the second component the first one's stream.
    friend bool operator==(const StreamTrigger& a, const StreamTrigger& b) noexcept
    {
        if (a.kind_ != b.kind_) { return false; }
        switch (a.kind_)
        {
            case TriggerKind::FixedRate: return a.hz_ == b.hz_;
            case TriggerKind::OnStream:  return a.topic_ == b.topic_;
            case TriggerKind::OnAny:     return true;
        }
        return false;
    }
    friend bool operator!=(const StreamTrigger& a, const StreamTrigger& b) noexcept { return !(a == b); }

    /// The wire spelling, for a log line. Never null.
    const char* kind_name() const noexcept
    {
        switch (kind_)
        {
            case TriggerKind::FixedRate: return "SRG_TRIGGER_FIXED_RATE";
            case TriggerKind::OnStream:  return "SRG_TRIGGER_ON_STREAM";
            case TriggerKind::OnAny:     return "SRG_TRIGGER_ON_ANY";
        }
        return "SRG_TRIGGER_UNKNOWN";
    }

private:
    StreamTrigger(TriggerKind k, double hz, std::string topic) noexcept
        : kind_(k), hz_(hz), topic_(std::move(topic)) {}

    TriggerKind kind_;
    double hz_;
    std::string topic_;
};

/// `SISFrameRef`: a node within an owning graph.
///
/// The two halves take *different* rules and that is the whole reason this is
/// a validated type rather than two strings: `client` is a key prefix and takes
/// the fragment rule, `node` is a bare name and takes the identifier rule.
/// Validating the client half as an identifier is B39 and it took the daemon
/// down.
class FrameRef
{
public:
    static Result<FrameRef> parse(std::string_view client, std::string_view node)
    {
        const Result<void> c = check_fragment(client);
        if (!c) { return Result<FrameRef>::fail(c.error()); }
        const Result<void> n = check_identifier(node);
        if (!n) { return Result<FrameRef>::fail(n.error()); }
        return Result<FrameRef>::ok(FrameRef(std::string(client), std::string(node)));
    }

    const std::string& client() const noexcept { return client_; }
    const std::string& node() const noexcept { return node_; }

    friend bool operator==(const FrameRef& a, const FrameRef& b) noexcept
    {
        return a.client_ == b.client_ && a.node_ == b.node_;
    }
    friend bool operator!=(const FrameRef& a, const FrameRef& b) noexcept { return !(a == b); }

    /// Total order, so a `FrameRef` pair can key a `std::map` without a hash.
    friend bool operator<(const FrameRef& a, const FrameRef& b) noexcept
    {
        if (a.client_ != b.client_) { return a.client_ < b.client_; }
        return a.node_ < b.node_;
    }

private:
    FrameRef(std::string c, std::string n) noexcept : client_(std::move(c)), node_(std::move(n)) {}
    std::string client_;
    std::string node_;
};

/// The default `freshness_bias`: prefer reuse.
///
/// The IDL's own reading — a derived edge is normally preferred because it is
/// fewer hops — so the library's default is the engine's default behaviour and
/// not a number this library invented.
inline constexpr double kDefaultFreshnessBias = 0.0;

/// `SISRelationStreamStartRequest`, validated.
///
/// There is one constructor and it validates. `freshness_bias` is refused
/// outside [0, 1] rather than clamped: a caller who passed 30 meant something,
/// and clamping to 1 would silently give them a stream that ignores every
/// derived edge in the graph.
class RelationStreamRequest
{
public:
    static Result<RelationStreamRequest> make(FrameRef observer,
                                              FrameRef target,
                                              StreamTrigger trigger,
                                              double freshness_bias = kDefaultFreshnessBias)
    {
        // Positive test again: NaN fails it.
        if (!(freshness_bias >= 0.0 && freshness_bias <= 1.0))
        {
            return Result<RelationStreamRequest>::fail(Error::FreshnessBiasOutOfRange);
        }
        return Result<RelationStreamRequest>::ok(
            RelationStreamRequest(std::move(observer), std::move(target),
                                  std::move(trigger), freshness_bias));
    }

    const FrameRef& observer() const noexcept { return observer_; }
    const FrameRef& target() const noexcept { return target_; }
    const StreamTrigger& trigger() const noexcept { return trigger_; }
    double freshness_bias() const noexcept { return bias_; }

    /// The two frames, and nothing else.
    ///
    /// This is what "the same relation" means, and it deliberately excludes the
    /// trigger and the bias. `RelationLeaseTable` keys its request memo on this
    /// so that a second acquire for the same relation on *different* terms is
    /// caught locally as `Error::ConflictingLease` rather than sent to the
    /// engine, which answers it `Rejected` with no way for the caller to tell
    /// which of the two requests was the odd one.
    struct Relation
    {
        FrameRef observer;
        FrameRef target;

        friend bool operator<(const Relation& a, const Relation& b) noexcept
        {
            if (a.observer < b.observer) { return true; }
            if (b.observer < a.observer) { return false; }
            return a.target < b.target;
        }
        friend bool operator==(const Relation& a, const Relation& b) noexcept
        {
            return a.observer == b.observer && a.target == b.target;
        }
    };

    Relation relation() const { return Relation{observer_, target_}; }

    /// The terms two acquires of the same relation must agree on.
    bool same_terms_as(const RelationStreamRequest& o) const noexcept
    {
        return trigger_ == o.trigger_ && bias_ == o.bias_;
    }

private:
    RelationStreamRequest(FrameRef o, FrameRef t, StreamTrigger tr, double b) noexcept
        : observer_(std::move(o)), target_(std::move(t)), trigger_(std::move(tr)), bias_(b) {}

    FrameRef observer_;
    FrameRef target_;
    StreamTrigger trigger_;
    double bias_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_RELATION_H
