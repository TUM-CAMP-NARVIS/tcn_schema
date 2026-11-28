// SPDX-License-Identifier: internal
//
// ClientScope — the client ids an engine serves (#35 R5).
//
// An engine announces its scope in `SISEngineCapabilities.client_scope`, in the
// join reply and in the unscoped `sqe/{engine}/info` summary. A client whose id
// the scope does not contain is not served by that engine, and a frame,
// fragment or domain outside it is answered `SIS_STATUS_NOT_IN_SCOPE`.
#ifndef TCN_SQE_SCOPE_H
#define TCN_SQE_SCOPE_H

#include <string>
#include <string_view>
#include <utility>

#include "tcn/sqe/client_id.h"
#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/validate.h"

namespace tcn {
namespace sqe {

/// A client-id prefix, or "every client".
///
/// `""` is unscoped and contains every client id. Otherwise the scope takes
/// the fragment rule and contains a client id when it **equals** it or is a
/// prefix of it **ending at a `/` chunk boundary**: `tcn/loc/ch` contains
/// `tcn/loc/ch` and `tcn/loc/ch/room`, never `tcn/loc/chx`. A plain string
/// prefix test would hand a client another site's engine.
class ClientScope
{
public:
    /// The only constructor: `""`, or a value that passes `check_fragment`.
    static Result<ClientScope> parse(std::string_view s)
    {
        if (!s.empty())
        {
            const Result<void> checked = check_fragment(s);
            if (!checked) { return Result<ClientScope>::fail(checked.error()); }
        }
        return Result<ClientScope>::ok(ClientScope(std::string(s)));
    }

    /// Whether this is `""`, which contains every client.
    bool unscoped() const noexcept { return prefix_.empty(); }

    bool contains(const ClientId& id) const noexcept
    {
        if (prefix_.empty()) { return true; }
        const std::string& s = id.str();
        if (s.size() < prefix_.size()) { return false; }
        if (s.compare(0, prefix_.size(), prefix_) != 0) { return false; }
        return s.size() == prefix_.size() || s[prefix_.size()] == '/';
    }

    const std::string& str() const noexcept { return prefix_; }

    friend bool operator==(const ClientScope& a, const ClientScope& b) noexcept { return a.prefix_ == b.prefix_; }
    friend bool operator!=(const ClientScope& a, const ClientScope& b) noexcept { return !(a == b); }

private:
    explicit ClientScope(std::string prefix) noexcept : prefix_(std::move(prefix)) {}
    std::string prefix_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_SCOPE_H
