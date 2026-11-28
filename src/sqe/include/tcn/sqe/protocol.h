// SPDX-License-Identifier: internal
//
// The protocol floor (#35 R5, §6): one number, written once.
//
// A header of its own so that contract.h (which exposes it) and
// capabilities.h (which compares against it) both include it without
// including each other.
#ifndef TCN_SQE_PROTOCOL_H
#define TCN_SQE_PROTOCOL_H

#include <cstdint>

/// The **protocol floor**: the engine compatibility break (B-number) whose
/// wire this library speaks -- the 0.11 break (provisionally B99). An engine
/// announcing a lower `SISEngineCapabilities.protocol_break` speaks an older
/// wire (`Capabilities::meets_floor()`); optional features above the floor are
/// capabilities (`Capabilities::has`), not higher numbers.
///
/// The only version number this library declares: the 0.10 B-levels
/// (`kContractBLevel`, `kSessionBLevel`, `kProtocolBLevel` and their macros)
/// are gone, and a consumer that asserted on them now fails to compile rather
/// than passing against a stale number.
///
/// The number is written once, here, and the constant is derived from it.
/// Hand-kept: the library is header-only and used from the source tree, so
/// nothing generates it yet. The value 99 is provisional: set at 0.11.0 to the engine's B-number.
#define TCN_SQE_PROTOCOL_BREAK 99

namespace tcn {
namespace sqe {

/// `TCN_SQE_PROTOCOL_BREAK`, typed like the wire field.
inline constexpr std::uint32_t kProtocolBreak = TCN_SQE_PROTOCOL_BREAK;

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_PROTOCOL_H
