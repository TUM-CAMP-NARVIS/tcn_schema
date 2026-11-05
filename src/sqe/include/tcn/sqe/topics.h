// SPDX-License-Identifier: internal
//
// The engine-issued topic shapes.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.4.
#ifndef TCN_SQE_TOPICS_H
#define TCN_SQE_TOPICS_H

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace tcn {
namespace sqe {

/// The top-level namespace every engine-issued key lives under.
inline constexpr std::string_view kEngineNamespace = "sqe";
/// The middle segment of a relation-stream output topic.
inline constexpr std::string_view kRelationFamily = "rel";
/// Appended to an output topic to get its status key.
inline constexpr std::string_view kStatusSuffix = "/status";
/// The middle segment of the engine's own liveliness token (B74).
inline constexpr std::string_view kEngineAliveFamily = "alive";
/// The key family the registry announces its changes under (B78).
inline constexpr std::string_view kTargetsChangedSuffix = "targets/changed";

namespace detail {

/// `sqe/{engine_id}/{tail}`. The one place the engine prefix is spelled.
inline std::string engine_key(std::string_view engine_id, std::string_view tail)
{
    std::string out;
    out.reserve(kEngineNamespace.size() + engine_id.size() + tail.size() + 2);
    out.append(kEngineNamespace.data(), kEngineNamespace.size());
    out += '/';
    out.append(engine_id.data(), engine_id.size());
    out += '/';
    out.append(tail.data(), tail.size());
    return out;
}

}  // namespace detail

/// `{output_topic}/status` — the key a relation stream's
/// `SISRelationStreamStatusNotification`s arrive on.
///
/// This one derivation *is* blessed: the IDL states it, and the engine
/// derives it the same way from the same value.
inline std::string status_topic_for(std::string_view output_topic)
{
    std::string out;
    out.reserve(output_topic.size() + kStatusSuffix.size());
    out.append(output_topic.data(), output_topic.size());
    out.append(kStatusSuffix.data(), kStatusSuffix.size());
    return out;
}

/// `sqe/{engine_id}/rel/{handle}` — the key an engine-issued relation stream
/// publishes on.
///
/// Provided so a *test harness or an engine-side tool* can spell the shape
/// once. **A client does not use this to find its stream.** A client reads
/// the topic out of the reply (see `Handle`), because the value is the
/// engine's to choose and a computed one subscribes to a key nothing
/// publishes on.
inline std::string engine_relation_topic(std::string_view engine_id, std::string_view handle)
{
    std::string out;
    out.reserve(kEngineNamespace.size() + engine_id.size() + kRelationFamily.size() + handle.size() + 3);
    out.append(kEngineNamespace.data(), kEngineNamespace.size());
    out += '/';
    out.append(engine_id.data(), engine_id.size());
    out += '/';
    out.append(kRelationFamily.data(), kRelationFamily.size());
    out += '/';
    out.append(handle.data(), handle.size());
    return out;
}

/// `sqe/{engine_id}/rel/{handle}/status`. Derived from
/// `engine_relation_topic`, so the shape is spelled out in one place only.
inline std::string engine_relation_status_topic(std::string_view engine_id, std::string_view handle)
{
    return status_topic_for(engine_relation_topic(engine_id, handle));
}

// ---------------------------------------------------------------------------
// Engine presence (B74)
// ---------------------------------------------------------------------------
//
// The engine holds a liveliness token on `sqe/{engine_id}/alive/{epoch}` for
// as long as it runs; `epoch` is its process start in Unix milliseconds,
// written in decimal. Both values come back in every SISMutationReply, so a
// client learns which engine to watch from its own join reply rather than by
// configuration.
//
// Unlike a relation topic, a client *does* compose this one, and that is not
// the mistake `engine_relation_topic` warns about: the reply carries the two
// parts and the IDL states the shape, so the composition is blessed in the
// same way `status_topic_for` is. The epoch is in the key rather than in a
// payload because a token has no payload -- and it is what distinguishes "the
// engine is gone" (the token is withdrawn) from "the engine restarted" (a token
// with another epoch appears). Joined fragments survive a restart; relation
// streams do not, so the second case obliges the client to re-start them.

/// `sqe/{engine_id}/alive/{epoch}` — the engine's liveliness token.
///
/// For a test harness, or for a client that wants the exact token its join
/// reply named. A client watching for a restart subscribes to
/// `engine_alive_pattern` instead.
inline std::string engine_alive_key(std::string_view engine_id, std::uint64_t epoch)
{
    std::string tail(kEngineAliveFamily);
    tail += '/';
    tail += std::to_string(epoch);
    return detail::engine_key(engine_id, tail);
}

/// `sqe/{engine_id}/alive/*` — what a client declares a liveliness subscriber
/// on, **with history**: without history a client that subscribes after the
/// engine came up sees no token at all, and cannot tell that from "no engine".
inline std::string engine_alive_pattern(std::string_view engine_id)
{
    std::string tail(kEngineAliveFamily);
    tail += "/*";
    return detail::engine_key(engine_id, tail);
}

/// The epoch out of an engine liveliness key, or nothing if `key` is not one.
///
/// Strict, because a liveliness subscriber on a pattern sees every key the
/// pattern matches and the caller compares the result against the epoch its
/// reply carried: `sqe/{id}/alive/` followed by one or more ASCII digits that
/// fit a `uint64`, and nothing after. No leading `+`, no sign, no whitespace —
/// the engine writes none, so a key with one is not the engine's.
///
/// If `engine_id` is non-empty, the key must also belong to that engine.
/// Pass an empty `engine_id` to accept any engine (a tool listing all of them).
inline std::optional<std::uint64_t> parse_engine_alive_key(std::string_view key,
                                                           std::string_view engine_id = {})
{
    // sqe/
    if (key.substr(0, kEngineNamespace.size()) != kEngineNamespace) { return std::nullopt; }
    key.remove_prefix(kEngineNamespace.size());
    if (key.empty() || key.front() != '/') { return std::nullopt; }
    key.remove_prefix(1);

    // {engine_id}/ -- one chunk, as every engine-issued key has it.
    const std::size_t slash = key.find('/');
    if (slash == std::string_view::npos || slash == 0) { return std::nullopt; }
    if (!engine_id.empty() && key.substr(0, slash) != engine_id) { return std::nullopt; }
    key.remove_prefix(slash + 1);

    // alive/
    if (key.substr(0, kEngineAliveFamily.size()) != kEngineAliveFamily) { return std::nullopt; }
    key.remove_prefix(kEngineAliveFamily.size());
    if (key.empty() || key.front() != '/') { return std::nullopt; }
    key.remove_prefix(1);

    // {epoch}
    if (key.empty()) { return std::nullopt; }
    std::uint64_t epoch = 0;
    for (const char c : key)
    {
        if (c < '0' || c > '9') { return std::nullopt; }
        const std::uint64_t digit = static_cast<std::uint64_t>(c - '0');
        if (epoch > (std::numeric_limits<std::uint64_t>::max() - digit) / 10u) { return std::nullopt; }   // overflow
        epoch = epoch * 10u + digit;
    }
    return epoch;
}

// ---------------------------------------------------------------------------
// The target registry (B78)
// ---------------------------------------------------------------------------

/// `sqe/{engine_id}/targets/changed` — where the engine publishes a
/// `SISTargetChange` after every accepted registry mutation.
///
/// Composed by a client for the same reason the presence key is: the
/// `engine_id` comes from a reply and the IDL states the shape. A client that
/// resolved names from the registry subscribes here and re-resolves (and
/// re-joins) when a target it uses changes; nothing else tells it.
inline std::string engine_targets_changed_topic(std::string_view engine_id)
{
    return detail::engine_key(engine_id, kTargetsChangedSuffix);
}

// There is deliberately no `engine_descriptor_topic` here. The IDL says to
// treat the descriptor key as opaque and read it from the reply, and the reply
// carries only the data topic — so the library does not bless a derivation the
// engine has not. If the engine adds the field, this becomes a two-line
// addition; guessing it now would be the same mistake as computing a handle.
//
// `tracker.h`'s `descriptor_key` is not a counter-example and not an
// exception. This file is about keys the **engine** issues, whose values are
// the engine's to choose. That one composes a key from a *client's own* client
// id and its own device name, and the engine composes the identical string
// from the identical two parts when it resolves a marker announcement.

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_TOPICS_H
