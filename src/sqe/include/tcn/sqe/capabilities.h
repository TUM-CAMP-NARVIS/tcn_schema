// SPDX-License-Identifier: internal
//
// Capabilities — what an engine says it is and offers (#35 R5, D4).
//
// Read from `sqe_msgs::rpc::SISEngineCapabilities`, which the join reply, the
// operator info reply and the unscoped `sqe/{engine}/info` summary carry. A
// client gates an optional feature on `has(capability::kSample)` rather than
// on a protocol number, and checks the one protocol floor with `meets_floor()`.
#ifndef TCN_SQE_CAPABILITIES_H
#define TCN_SQE_CAPABILITIES_H

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tcn/sqe/protocol.h"
#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/scope.h"

namespace tcn {
namespace sqe {

/// The capability names this library knows. An engine may announce others; a
/// client keeps them (`has` answers for any name) and ignores what it does not
/// use.
namespace capability {

/// `/sis/stream/sample`: one value of a relation at one instant.
inline constexpr std::string_view kSample = "sample";
/// DOMAIN-scoped frames (`SR_NODE_SCOPE_DOMAIN`, `domain:<domain>/<name>`).
inline constexpr std::string_view kDomain = "domain";
/// The engine accepts and applies the OpenCV rational distortion model
/// (`SR_DISTORTION_MODEL_OPENCV_RATIONAL`); without it, a camera edge using that
/// model is refused (REJECTED + UNSUPPORTED).
inline constexpr std::string_view kRationalLens = "rational_lens";
/// The engine reads and writes marker domains: a marker id unique within a
/// domain rather than engine-wide (`SRMaterialization.marker_domain`, and the
/// `SISGraphFilter.marker_domain` filter).
inline constexpr std::string_view kMarkerDomain = "marker_domain";

}  // namespace capability

/// Whether `name` is a well-formed capability name: `[a-z0-9_]{1,64}`.
constexpr bool valid_capability_name(std::string_view name) noexcept
{
    if (name.empty() || name.size() > 64) { return false; }
    for (std::size_t i = 0; i < name.size(); ++i)
    {
        const char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) { return false; }
    }
    return true;
}

/// An engine's protocol break, capability names and client scope, validated.
class Capabilities
{
public:
    /// From a generated `SISEngineCapabilities` (or anything with its
    /// `protocol_break()`, `capabilities()` and `client_scope()`).
    ///
    /// A malformed capability name -> `MalformedCapability`: the engine is
    /// not speaking the contract, and a client must not guess what it meant.
    /// A malformed scope -> the fragment rule's error. Duplicate names are
    /// kept once, in first-seen order.
    template <class E>
    static Result<Capabilities> from_wire(const E& engine)
    {
        std::vector<std::string> names;
        for (const auto& n : engine.capabilities())
        {
            const std::string_view name(n);
            if (!valid_capability_name(name)) { return Result<Capabilities>::fail(Error::MalformedCapability); }
            if (std::find(names.begin(), names.end(), name) == names.end()) { names.emplace_back(name); }
        }
        Result<ClientScope> scope = ClientScope::parse(engine.client_scope());
        if (!scope) { return Result<Capabilities>::fail(scope.error()); }
        return Result<Capabilities>::ok(Capabilities(static_cast<std::uint32_t>(engine.protocol_break()),
                                                     std::move(names), std::move(scope.value())));
    }

    /// The highest compatibility break (B-number) the engine implements.
    std::uint32_t protocol_break() const noexcept { return protocol_break_; }

    bool has(std::string_view name) const noexcept
    {
        return std::find(names_.begin(), names_.end(), name) != names_.end();
    }

    /// Every announced name, unique, in the engine's order.
    const std::vector<std::string>& names() const noexcept { return names_; }

    const ClientScope& scope() const noexcept { return scope_; }

    /// Whether the engine is at or above this library's floor, `kProtocolBreak`.
    bool meets_floor() const noexcept { return protocol_break_ >= kProtocolBreak; }

private:
    Capabilities(std::uint32_t pb, std::vector<std::string> names, ClientScope scope)
        : protocol_break_(pb), names_(std::move(names)), scope_(std::move(scope)) {}

    std::uint32_t protocol_break_;
    std::vector<std::string> names_;
    ClientScope scope_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_CAPABILITIES_H
