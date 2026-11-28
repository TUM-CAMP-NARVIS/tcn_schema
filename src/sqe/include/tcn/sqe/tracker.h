// SPDX-License-Identifier: internal
//
// The tracker key family: descriptor, data stream, and marker announcement.
//
// Three keys about one device, and the engine resolves each from the others:
// a marker announcement on `{client}/cfg/marker/{device}/{marker_id}` sends it
// looking for the descriptor at `{client}/cfg/desc/{device}`, and the
// descriptor names the data topic. Composed apart, they drift apart, and the
// failure is silent at every step -- an unmatched key is not an error to
// Zenoh, and the engine cannot report a descriptor it never saw.
#ifndef TCN_SQE_TRACKER_H
#define TCN_SQE_TRACKER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

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

/// `{client}/cfg/marker/{device}/{marker_id}` — the liveliness key that tells
/// the engine `device` has seen the marker `marker_id`.
///
/// The marker is its numeric id in decimal (0.11, #35 H5), the same
/// `uint32` a materialized node records in `SRMaterialization.marker_id`.
/// Before 0.11 it was a composed node name (`_mtarucom___sis_marker__7`),
/// which the engine had to agree on character for character; an id has no
/// spelling to disagree about.
inline Result<std::string> marker_announce_key(const ClientId& client, std::string_view device,
                                               std::uint32_t marker_id)
{
    Result<std::string> head = detail::device_key(client, keys::kMarkerAnnounceFamily, device);
    if (!head) { return head; }

    std::string out = std::move(head.value());
    out += '/';
    out += std::to_string(marker_id);
    return Result<std::string>::ok(std::move(out));
}

// ---------------------------------------------------------------------------
// Marker kinds
// ---------------------------------------------------------------------------

/// What a tracker reports, mirroring `tcn_types::msg::MarkerType`.
///
/// A mirror rather than the IDL enum, because this library includes no
/// generated header and builds where fastddsgen does not exist. The ordinals
/// are the IDL's own and are pinned against it by
/// tools/idl/tests/test_contract_enums.py.
enum class MarkerKind : std::int32_t {
    None = 0,
    ArucoMarker = 1,
    ArucoBoard = 2,
    ArucoFractal = 3,
    IrTarget = 4,
    HmdPose = 5,
    AprilTag = 6,
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_TRACKER_H
