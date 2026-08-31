// SPDX-License-Identifier: internal
//
// NodeName / LocalNodeId / GlobalNodeId — the two alphabets, kept apart.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.3.
#ifndef TCN_SQE_NODE_NAME_H
#define TCN_SQE_NODE_NAME_H

#include <string>
#include <string_view>
#include <utility>

#include "tcn/sqe/client_id.h"
#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/validate.h"

namespace tcn {
namespace sqe {

/// The separator between a composed node id's two halves.
inline constexpr std::string_view kNodeIdSeparator = "::";

/// A node or edge name: the **identifier** rule, whole.
class NodeName
{
public:
    static Result<NodeName> parse(std::string_view s)
    {
        const Result<void> checked = check_identifier(s);
        if (!checked) { return Result<NodeName>::fail(checked.error()); }
        return Result<NodeName>::ok(NodeName(std::string(s)));
    }

    const std::string& str() const noexcept { return name_; }

    friend bool operator==(const NodeName& a, const NodeName& b) noexcept { return a.name_ == b.name_; }
    friend bool operator!=(const NodeName& a, const NodeName& b) noexcept { return !(a == b); }

private:
    explicit NodeName(std::string n) noexcept : name_(std::move(n)) {}
    std::string name_;
};

/// A local node's composed id: `{fragment}::{name}`.
///
/// The fragment half is a client id and takes the fragment rule; the name half
/// takes the identifier rule. This is the only value in the contract that
/// mixes the two alphabets, and it does so because its left half *is* a key
/// prefix.
class LocalNodeId
{
public:
    static LocalNodeId compose(const ClientId& fragment, const NodeName& name)
    {
        std::string id;
        id.reserve(fragment.str().size() + kNodeIdSeparator.size() + name.str().size());
        id += fragment.str();
        id.append(kNodeIdSeparator.data(), kNodeIdSeparator.size());
        id += name.str();
        return LocalNodeId(std::move(id));
    }

    /// Splits at the **first** `::`, matching the engine.
    ///
    /// `:` is a legal identifier character, so any other split is wrong:
    /// `a::b::c` is the node `b::c` in fragment `a`, not the node `c` in
    /// fragment `a::b`.
    static Result<LocalNodeId> parse(std::string_view s)
    {
        const std::size_t at = s.find(kNodeIdSeparator);
        if (at == std::string_view::npos) { return Result<LocalNodeId>::fail(Error::IllegalCharacter); }

        const Result<void> frag = check_fragment(s.substr(0, at));
        if (!frag) { return Result<LocalNodeId>::fail(frag.error()); }

        const Result<void> name = check_identifier(s.substr(at + kNodeIdSeparator.size()));
        if (!name) { return Result<LocalNodeId>::fail(name.error()); }

        return Result<LocalNodeId>::ok(LocalNodeId(std::string(s)));
    }

    const std::string& str() const noexcept { return id_; }

    friend bool operator==(const LocalNodeId& a, const LocalNodeId& b) noexcept { return a.id_ == b.id_; }
    friend bool operator!=(const LocalNodeId& a, const LocalNodeId& b) noexcept { return !(a == b); }

private:
    explicit LocalNodeId(std::string id) noexcept : id_(std::move(id)) {}
    std::string id_;
};

/// A global node's id: a bare name, and nothing about it is a key prefix.
///
/// A distinct type rather than a `using`, so that nobody applies the fragment
/// rule to it "for symmetry" and admits a `/` into a bare global name.
class GlobalNodeId
{
public:
    static GlobalNodeId of(const NodeName& name) { return GlobalNodeId(name.str()); }

    static Result<GlobalNodeId> parse(std::string_view s)
    {
        const Result<NodeName> n = NodeName::parse(s);
        if (!n) { return Result<GlobalNodeId>::fail(n.error()); }
        return Result<GlobalNodeId>::ok(GlobalNodeId(n.value().str()));
    }

    const std::string& str() const noexcept { return id_; }

    friend bool operator==(const GlobalNodeId& a, const GlobalNodeId& b) noexcept { return a.id_ == b.id_; }
    friend bool operator!=(const GlobalNodeId& a, const GlobalNodeId& b) noexcept { return !(a == b); }

private:
    explicit GlobalNodeId(std::string id) noexcept : id_(std::move(id)) {}
    std::string id_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_NODE_NAME_H
