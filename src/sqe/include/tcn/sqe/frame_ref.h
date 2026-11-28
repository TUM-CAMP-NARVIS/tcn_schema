// SPDX-License-Identifier: internal
//
// FrameRef — a validated `tcn_types::msg::SRFrameRef`, and its N2a text form.
//
// The text form (#35 N2a) is
//
//     local:<fragment>/<name>    global:<name>    domain:<domain>/<name>
//
// each optionally followed by `[instances]`. It is what a configuration file,
// a log line or a command line names a frame by, and it is parsed identically
// by this library and by the engine: both run the shared vector file
// `src/sqe/vectors/frame_ref.tsv` (installed as
// `share/tcn_schema/sqe/frame_ref.tsv`), which states the parse order and
// every refusal with its `code()`.
#ifndef TCN_SQE_FRAME_REF_H
#define TCN_SQE_FRAME_REF_H

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/validate.h"

namespace tcn {
namespace sqe {

/// `tcn_types::msg::SRNodeScope`, ordinals pinned to the IDL's declaration
/// order. `Unknown` is what a zeroed or newer value reads as (E6); no
/// `FrameRef` holds it.
enum class NodeScope : std::int32_t
{
    Unknown = 0,
    Local = 1,    ///< owned by one fragment; the owner is its client id
    Global = 2,   ///< no owner; one node per name, engine-wide
    Domain = 3,   ///< owned by a domain, whichever client declared it
};

/// `tcn_types::msg::SRFrameSelector`: the node itself, or every instance
/// materialized from it. Ordinals pinned to the IDL's declaration order.
enum class FrameSelector : std::int32_t
{
    Unknown = 0,
    Frame = 1,
    Instances = 2,
};

/// A frame reference: scope, owner, name and selector, validated.
///
/// The owner takes the **fragment** rule (it is a client id or a domain, both
/// key prefixes) and the name the **identifier** rule; a GLOBAL reference has
/// no owner. There is no public constructor: a `FrameRef` comes from a named
/// constructor, `parse`, or `from_wire`, and each validates.
///
/// A LOCAL reference with an empty owner ("this fragment") is legal on the
/// wire inside a join only. It has no text form, and a `FrameRef` never holds
/// one: `from_wire` refuses it with `MissingOwner`.
class FrameRef
{
public:
    static constexpr std::string_view kLocalScheme = "local";
    static constexpr std::string_view kGlobalScheme = "global";
    static constexpr std::string_view kDomainScheme = "domain";
    static constexpr std::string_view kInstancesSuffix = "[instances]";

    /// A node `name` in the fragment `fragment`.
    static Result<FrameRef> local(std::string_view fragment, std::string_view name)
    {
        return owned(NodeScope::Local, fragment, name, FrameSelector::Frame);
    }

    /// The global node `name`.
    static Result<FrameRef> global(std::string_view name)
    {
        const Result<void> n = check_identifier(name);
        if (!n) { return Result<FrameRef>::fail(n.error()); }
        return Result<FrameRef>::ok(FrameRef(NodeScope::Global, std::string(), std::string(name),
                                             FrameSelector::Frame));
    }

    /// The node `name` in the domain `domain`.
    static Result<FrameRef> domain(std::string_view domain, std::string_view name)
    {
        return owned(NodeScope::Domain, domain, name, FrameSelector::Frame);
    }

    /// The N2a text form. In this order, the first failure wins:
    ///   1. empty -> `Empty`;
    ///   2. a `[` anywhere: the text must end with `[instances]` and contain
    ///      no other `[` -> else `MalformedSelector`; strip it;
    ///   3. the scheme is the text before the **first** `:`; none ->
    ///      `MissingScheme`; not exactly `local`/`global`/`domain` ->
    ///      `UnknownScheme`;
    ///   4. `global`: a `/` in the body -> `UnexpectedOwner`; else the
    ///      identifier rule;
    ///   5. `local`/`domain`: split at the **last** `/`; none ->
    ///      `MissingOwner`; the fragment rule on the owner, then the
    ///      identifier rule on the name.
    static Result<FrameRef> parse(std::string_view text)
    {
        if (text.empty()) { return Result<FrameRef>::fail(Error::Empty); }

        FrameSelector selector = FrameSelector::Frame;
        const std::size_t bracket = text.find('[');
        if (bracket != std::string_view::npos)
        {
            const bool suffixed = text.size() >= kInstancesSuffix.size()
                && text.substr(text.size() - kInstancesSuffix.size()) == kInstancesSuffix;
            if (!suffixed || bracket != text.size() - kInstancesSuffix.size())
            {
                return Result<FrameRef>::fail(Error::MalformedSelector);
            }
            text.remove_suffix(kInstancesSuffix.size());
            selector = FrameSelector::Instances;
        }

        const std::size_t colon = text.find(':');
        if (colon == std::string_view::npos) { return Result<FrameRef>::fail(Error::MissingScheme); }
        const std::string_view scheme = text.substr(0, colon);
        const std::string_view body = text.substr(colon + 1);

        if (scheme == kGlobalScheme)
        {
            if (body.find('/') != std::string_view::npos) { return Result<FrameRef>::fail(Error::UnexpectedOwner); }
            const Result<void> n = check_identifier(body);
            if (!n) { return Result<FrameRef>::fail(n.error()); }
            return Result<FrameRef>::ok(FrameRef(NodeScope::Global, std::string(), std::string(body), selector));
        }

        NodeScope scope = NodeScope::Unknown;
        if (scheme == kLocalScheme) { scope = NodeScope::Local; }
        else if (scheme == kDomainScheme) { scope = NodeScope::Domain; }
        else { return Result<FrameRef>::fail(Error::UnknownScheme); }

        const std::size_t slash = body.rfind('/');
        if (slash == std::string_view::npos) { return Result<FrameRef>::fail(Error::MissingOwner); }
        return owned(scope, body.substr(0, slash), body.substr(slash + 1), selector);
    }

    /// The same frame, selecting every instance materialized from it.
    FrameRef instances() const
    {
        return FrameRef(scope_, owner_, name_, FrameSelector::Instances);
    }

    /// The N2a text form; `parse(f.format()) == f` for every `FrameRef`.
    std::string format() const
    {
        std::string out;
        out.reserve(kGlobalScheme.size() + 1 + owner_.size() + 1 + name_.size() + kInstancesSuffix.size());
        switch (scope_)
        {
            case NodeScope::Local:  out.append(kLocalScheme.data(), kLocalScheme.size()); break;
            case NodeScope::Global: out.append(kGlobalScheme.data(), kGlobalScheme.size()); break;
            case NodeScope::Domain: out.append(kDomainScheme.data(), kDomainScheme.size()); break;
            case NodeScope::Unknown: break;   // unreachable: no FrameRef holds it
        }
        out += ':';
        if (scope_ != NodeScope::Global)
        {
            out += owner_;
            out += '/';
        }
        out += name_;
        if (selector_ == FrameSelector::Instances) { out.append(kInstancesSuffix.data(), kInstancesSuffix.size()); }
        return out;
    }

    NodeScope scope() const noexcept { return scope_; }
    /// The fragment (LOCAL), the domain (DOMAIN), or `""` (GLOBAL).
    const std::string& owner() const noexcept { return owner_; }
    const std::string& name() const noexcept { return name_; }
    FrameSelector selector() const noexcept { return selector_; }

    /// From a generated `SRFrameRef` (or anything with its four accessors).
    ///
    /// `UNKNOWN` or an unknown ordinal -> `UnknownScope` / `UnknownSelector`;
    /// an empty LOCAL or DOMAIN owner -> `MissingOwner`; a GLOBAL reference
    /// with an owner -> `UnexpectedOwner`; then the owner and name rules.
    template <class W>
    static Result<FrameRef> from_wire(const W& w)
    {
        const NodeScope scope = scope_from_ordinal(static_cast<std::int64_t>(w.scope()));
        if (scope == NodeScope::Unknown) { return Result<FrameRef>::fail(Error::UnknownScope); }
        const FrameSelector selector = selector_from_ordinal(static_cast<std::int64_t>(w.selector()));
        if (selector == FrameSelector::Unknown) { return Result<FrameRef>::fail(Error::UnknownSelector); }

        const std::string& owner = w.owner();
        if (scope == NodeScope::Global)
        {
            if (!owner.empty()) { return Result<FrameRef>::fail(Error::UnexpectedOwner); }
            const Result<void> n = check_identifier(w.name());
            if (!n) { return Result<FrameRef>::fail(n.error()); }
            return Result<FrameRef>::ok(FrameRef(NodeScope::Global, std::string(), std::string(w.name()), selector));
        }
        if (owner.empty()) { return Result<FrameRef>::fail(Error::MissingOwner); }
        return owned(scope, owner, w.name(), selector);
    }

    /// Into a generated `SRFrameRef`; every field is written.
    template <class W>
    void to_wire(W& w) const
    {
        using WireScope = typename std::decay<decltype(w.scope())>::type;
        using WireSelector = typename std::decay<decltype(w.selector())>::type;
        w.scope(static_cast<WireScope>(static_cast<std::int32_t>(scope_)));
        w.owner(owner_);
        w.name(name_);
        w.selector(static_cast<WireSelector>(static_cast<std::int32_t>(selector_)));
    }

    friend bool operator==(const FrameRef& a, const FrameRef& b) noexcept
    {
        return a.scope_ == b.scope_ && a.owner_ == b.owner_ && a.name_ == b.name_ && a.selector_ == b.selector_;
    }
    friend bool operator!=(const FrameRef& a, const FrameRef& b) noexcept { return !(a == b); }

    /// Total order (scope, owner, name, selector), so a `FrameRef` pair can key
    /// a `std::map` without a hash.
    friend bool operator<(const FrameRef& a, const FrameRef& b) noexcept
    {
        if (a.scope_ != b.scope_) { return a.scope_ < b.scope_; }
        if (a.owner_ != b.owner_) { return a.owner_ < b.owner_; }
        if (a.name_ != b.name_) { return a.name_ < b.name_; }
        return a.selector_ < b.selector_;
    }

private:
    FrameRef(NodeScope scope, std::string owner, std::string name, FrameSelector selector) noexcept
        : scope_(scope), owner_(std::move(owner)), name_(std::move(name)), selector_(selector) {}

    static Result<FrameRef> owned(NodeScope scope, std::string_view owner, std::string_view name,
                                  FrameSelector selector)
    {
        const Result<void> o = check_fragment(owner);
        if (!o) { return Result<FrameRef>::fail(o.error()); }
        const Result<void> n = check_identifier(name);
        if (!n) { return Result<FrameRef>::fail(n.error()); }
        return Result<FrameRef>::ok(FrameRef(scope, std::string(owner), std::string(name), selector));
    }

    static NodeScope scope_from_ordinal(std::int64_t v) noexcept
    {
        switch (v)
        {
            case 1: return NodeScope::Local;
            case 2: return NodeScope::Global;
            case 3: return NodeScope::Domain;
            default: return NodeScope::Unknown;
        }
    }

    static FrameSelector selector_from_ordinal(std::int64_t v) noexcept
    {
        switch (v)
        {
            case 1: return FrameSelector::Frame;
            case 2: return FrameSelector::Instances;
            default: return FrameSelector::Unknown;
        }
    }

    NodeScope scope_;
    std::string owner_;
    std::string name_;
    FrameSelector selector_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_FRAME_REF_H
