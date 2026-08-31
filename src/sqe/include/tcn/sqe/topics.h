// SPDX-License-Identifier: internal
//
// The engine-issued topic shapes.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.4.
#ifndef TCN_SQE_TOPICS_H
#define TCN_SQE_TOPICS_H

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

// There is deliberately no `engine_descriptor_topic` here. The IDL says to
// treat the descriptor key as opaque and read it from the reply, and the reply
// carries only the data topic — so the library does not bless a derivation the
// engine has not. If the engine adds the field, this becomes a two-line
// addition; guessing it now would be the same mistake as computing a handle.

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_TOPICS_H
