// SPDX-License-Identifier: internal
//
// NodeName — a node or edge name, by the identifier rule.
//
// The composed node ids of 0.10 (`{fragment}::{name}`, `@{domain}::{name}`)
// are gone (#35 N1): a frame is named by scope, owner and name in separate
// fields, which is `FrameRef` (frame_ref.h).
//
// See docs/design/2026-08-31-sqe-contract-library-design.md §4.3.
#ifndef TCN_SQE_NODE_NAME_H
#define TCN_SQE_NODE_NAME_H

#include <string>
#include <string_view>
#include <utility>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/validate.h"

namespace tcn {
namespace sqe {

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

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_NODE_NAME_H
