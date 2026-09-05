// SPDX-License-Identifier: internal
//
// The tracker key family: descriptor, data stream, and marker announcement.
//
// Three keys about one device, and the engine resolves each from the others:
// a marker announcement on `{client}/cfg/marker/{device}/{marker}` sends it
// looking for the descriptor at `{client}/cfg/desc/{device}`, and the
// descriptor names the data topic. Composed apart, they drift apart, and the
// failure is silent at every step -- an unmatched key is not an error to
// Zenoh, and the engine cannot report a descriptor it never saw.
#ifndef TCN_SQE_TRACKER_H
#define TCN_SQE_TRACKER_H

#include <cstdint>
#include <string>
#include <string_view>

#include "tcn/sqe/client_id.h"
#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/validate.h"

namespace tcn {
namespace sqe {

namespace keys {

/// Where a sensor announces its `StreamDescriptorMessage`.
inline constexpr std::string_view kDescriptorFamily = "/cfg/desc";
/// Where it publishes the poses that descriptor describes.
inline constexpr std::string_view kTrackerStreamFamily = "/str/trk";
/// Not a publish key. A tracker declares a liveliness token here, one per
/// marker it has seen, and holds it; nothing is ever `put` on it. Declaring
/// it *is* the message, exactly as `kAlive` is.
inline constexpr std::string_view kMarkerAnnounceFamily = "/cfg/marker";

}  // namespace keys

namespace detail {

inline Result<std::string> device_key(const ClientId& client, std::string_view family,
                                      std::string_view device)
{
    // The identifier rule, and not for tidiness: the engine parses a marker
    // announcement by splitting the key from the right twice, so a device
    // containing '/' makes the parse fail and the announcement is dropped
    // without a word. The descriptor key would be equally unfindable.
    const Result<void> checked = check_identifier(device);
    if (!checked) { return Result<std::string>::fail(checked.error()); }

    std::string out;
    out.reserve(client.str().size() + family.size() + device.size() + 1);
    out += client.str();
    out.append(family.data(), family.size());
    out += '/';
    out.append(device.data(), device.size());
    return Result<std::string>::ok(std::move(out));
}

}  // namespace detail

/// `{client}/cfg/desc/{device}` — where `device`'s stream descriptor goes.
inline Result<std::string> descriptor_key(const ClientId& client, std::string_view device)
{
    return detail::device_key(client, keys::kDescriptorFamily, device);
}

/// `{client}/str/trk/{device}` — where `device`'s poses go.
inline Result<std::string> tracker_stream_key(const ClientId& client, std::string_view device)
{
    return detail::device_key(client, keys::kTrackerStreamFamily, device);
}

/// `{client}/cfg/marker/{device}/{marker}` — the liveliness key that tells the
/// engine `device` has seen `marker`.
///
/// `marker` is the marker's **node name**, not its numeric id: the engine uses
/// the announced string verbatim as the node it creates, and deduplicates it
/// against the name the pose path derives for the same marker. Two spellings
/// of one marker are two nodes. Use `marker_node_name` to compose it.
inline Result<std::string> marker_announce_key(const ClientId& client, std::string_view device,
                                               std::string_view marker)
{
    const Result<std::string> head =
        detail::device_key(client, keys::kMarkerAnnounceFamily, device);
    if (!head) { return head; }

    const Result<void> checked = check_identifier(marker);
    if (!checked) { return Result<std::string>::fail(checked.error()); }

    std::string out = head.value();
    out += '/';
    out.append(marker.data(), marker.size());
    return Result<std::string>::ok(std::move(out));
}

// ---------------------------------------------------------------------------
// Marker node names
// ---------------------------------------------------------------------------

/// What a tracker reports, mirroring `tcnart_msgs::msg::MarkerType`.
///
/// A mirror rather than the IDL enum, because this library includes no
/// generated header and builds where fastddsgen does not exist. The ordinals
/// are the IDL's own and are pinned against it in `tcnart`, which has both.
enum class MarkerKind : std::uint32_t {
    None = 0,
    ArucoMarker = 1,
    ArucoBoard = 2,
    ArucoFractal = 3,
    IrTarget = 4,
    HmdPose = 5,
    AprilTag = 6,
};

/// Joins a tracker's prefix to a marker id.
///
/// Spelled with leading and trailing underscores on both halves, so the joined
/// name has three underscores in the middle (`_mtarucom___sis_marker__7`). That
/// is not a typo and must not be tidied: the engine composes the same two
/// halves the same way, and a name that differs by one character is a different
/// node.
inline constexpr std::string_view kMarkerNodeInfix = "__sis_marker__";

/// The node-name prefix the engine derives from a descriptor's `marker_type`.
///
/// Every marker a tracker of that kind reports becomes a node named
/// `{prefix}__sis_marker__{id}`. The values are the engine's
/// (`marker_prefix_from_type`), not a convention this library is free to
/// choose: a different prefix names a node nothing else refers to.
inline constexpr std::string_view marker_prefix(MarkerKind kind)
{
    switch (kind) {
        case MarkerKind::ArucoMarker:  return "_mtarucom_";
        case MarkerKind::ArucoBoard:   return "_mtarucob_";
        case MarkerKind::ArucoFractal: return "_mtarucof_";
        case MarkerKind::IrTarget:     return "_mtirt_";
        case MarkerKind::HmdPose:      return "_hmdvp_";
        case MarkerKind::AprilTag:     return "_mtapril_";
        case MarkerKind::None:         break;
    }
    return "_mtnone_";
}

/// `{prefix}__sis_marker__{id}` — the node a marker becomes.
///
/// The one string a marker announcement and a pose on the same tracker must
/// agree on. They are derived here and in the engine from the same two inputs;
/// composing it any other way is how one marker becomes two nodes, one of them
/// fed by nothing.
inline std::string marker_node_name(MarkerKind kind, std::uint32_t marker_id)
{
    const std::string_view prefix = marker_prefix(kind);
    std::string out;
    out.reserve(prefix.size() + kMarkerNodeInfix.size() + 10);
    out.append(prefix.data(), prefix.size());
    out.append(kMarkerNodeInfix.data(), kMarkerNodeInfix.size());
    out += std::to_string(marker_id);
    return out;
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_TRACKER_H
