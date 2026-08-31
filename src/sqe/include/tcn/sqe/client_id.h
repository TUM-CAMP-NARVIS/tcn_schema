// SPDX-License-Identifier: internal
//
// ClientId — a validated Zenoh key prefix, and the nine keys built from it.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.2.
#ifndef TCN_SQE_CLIENT_ID_H
#define TCN_SQE_CLIENT_ID_H

#include <string>
#include <string_view>
#include <utility>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/validate.h"

namespace tcn {
namespace sqe {

/// The nine key suffixes. A client's key is `{client_id}{SUFFIX}`; every
/// suffix begins with `/`, which is why a client id must not end with one.
namespace keys {

inline constexpr std::string_view kJoin            = "/sis/join";
inline constexpr std::string_view kLeave           = "/sis/leave";
/// Not a publish key. A client declares a Zenoh **liveliness token** here and
/// holds it; nothing is ever `put` on it. Declaring it *is* the message.
inline constexpr std::string_view kAlive           = "/sis/alive";
inline constexpr std::string_view kNodeUpdate      = "/sis/component/update";
inline constexpr std::string_view kNodeRemove      = "/sis/component/remove";
inline constexpr std::string_view kEdgeUpdate      = "/sis/relation/update";
inline constexpr std::string_view kEdgeRemove      = "/sis/relation/remove";
inline constexpr std::string_view kStreamStart     = "/sis/stream/start";
inline constexpr std::string_view kStreamStop      = "/sis/stream/stop";

}  // namespace keys

namespace detail {

inline std::string concat(std::string_view a, std::string_view b)
{
    std::string out;
    out.reserve(a.size() + b.size());
    out.append(a.data(), a.size());
    out.append(b.data(), b.size());
    return out;
}

}  // namespace detail

/// A client id: a Zenoh key **prefix**, validated by the *fragment* rule.
///
/// It contains `/` by design — for the documented default topic prefix it is
/// `tcn/loc/pcpd/{name}` — so it is not an identifier and must never be
/// validated as one. There is exactly one way to make a `ClientId`, and it
/// validates.
class ClientId
{
public:
    /// The only constructor. Applies `check_fragment`.
    static Result<ClientId> parse(std::string_view s)
    {
        const Result<void> checked = check_fragment(s);
        if (!checked) { return Result<ClientId>::fail(checked.error()); }
        return Result<ClientId>::ok(ClientId(std::string(s)));
    }

    const std::string& str() const noexcept { return id_; }

    // -- the nine keys ------------------------------------------------------
    //
    // Nine named accessors, and no `key(std::string_view suffix)` overload.
    // That is the point: a free-form suffix parameter re-opens exactly the
    // failure a rename causes, where the publisher writes to a key nothing is
    // subscribed to and is told nothing by anything.

    std::string join_key()         const { return detail::concat(id_, keys::kJoin); }
    std::string leave_key()        const { return detail::concat(id_, keys::kLeave); }
    std::string alive_key()        const { return detail::concat(id_, keys::kAlive); }
    std::string node_update_key()  const { return detail::concat(id_, keys::kNodeUpdate); }
    std::string node_remove_key()  const { return detail::concat(id_, keys::kNodeRemove); }
    std::string edge_update_key()  const { return detail::concat(id_, keys::kEdgeUpdate); }
    std::string edge_remove_key()  const { return detail::concat(id_, keys::kEdgeRemove); }
    std::string stream_start_key() const { return detail::concat(id_, keys::kStreamStart); }
    std::string stream_stop_key()  const { return detail::concat(id_, keys::kStreamStop); }

    friend bool operator==(const ClientId& a, const ClientId& b) noexcept { return a.id_ == b.id_; }
    friend bool operator!=(const ClientId& a, const ClientId& b) noexcept { return !(a == b); }

private:
    explicit ClientId(std::string id) noexcept : id_(std::move(id)) {}

    std::string id_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_CLIENT_ID_H
