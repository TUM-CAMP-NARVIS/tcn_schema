// SPDX-License-Identifier: internal
//
// tcn::sqe — the SQE contract core. Header-only, C++17, dependency-free.
//
// One include for the whole component. See
// docs/2026-08-31-sqe-contract-library-design.md.
//
// What this header does NOT do, deliberately:
//   * it does not compute a relation handle (the FNV-1a algorithm is
//     specified, and implementing it is exactly the mistake it invites);
//   * it does not compose a *relation stream's* output topic (the engine
//     chooses it and the reply carries it, so a computed one subscribes to a
//     key nothing publishes on);
//   * it does not implement `sqe.derived:` name generation (Rust's
//     `DefaultHasher` is not reproducible in C++ at all);
//   * it performs no I/O, holds no thread, opens no session, and logs nothing.
#ifndef TCN_SQE_CONTRACT_H
#define TCN_SQE_CONTRACT_H

#include "tcn/sqe/client_id.h"
#include "tcn/sqe/error.h"
#include "tcn/sqe/handle.h"
#include "tcn/sqe/node_name.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/status.h"
#include "tcn/sqe/topics.h"
#include "tcn/sqe/tracker.h"
#include "tcn/sqe/validate.h"
#include "tcn/sqe/wire_type.h"

// Every target is little-endian and every fixture is little-endian DDS-CDR.
// This is here so that a future big-endian port fails loudly rather than
// producing quiet garbage. No-op on toolchains that do not define the macro
// (MSVC), which have no big-endian target anyway.
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
static_assert(__BYTE_ORDER__ != __ORDER_BIG_ENDIAN__,
              "tcn::sqe assumes a little-endian target; the CDR fixtures are little-endian");
#endif

namespace tcn {
namespace sqe {

/// The **floor**: the highest entry in the engine's
/// `docs/reference/compatibility-breaks.md` such that every contract-core
/// obligation at or below it is implemented here.
///
/// A consumer writes `static_assert(tcn::sqe::kContractBLevel >= 35);` next to
/// code that depends on the `Pending`/`Active` status split and gets a compile
/// error instead of a retry loop against an acceptance.
///
/// It is 35, not 42, and the number is a floor rather than a boast:
///   * B36 — the relation lease table's local refcount — is not here. It
///     needs a start and a stop to go out, so it belongs to the `sqe_session`
///     component, where `RelationLeaseTable` implements it; a consumer that
///     links only `sqe_contract` does not have it, and this number must not
///     say otherwise. `tcn/sqe/session.h` declares its own floor,
///     `kSessionBLevel`, which is above this one.
///   * B39 — the identifier/fragment split — *is* implemented (see
///     validate.h), above the floor. A floor is not a boast.
///   * B41 is the liveliness token, a transport obligation: this component
///     names the key (`ClientId::alive_key`) but cannot declare the token.
///     The same is true of the marker announcement (`marker_announce_key`),
///     which is a token on a different key and no entry in the B-log at all:
///     the engine added a subscription, and a client that declares nothing is
///     exactly as correct as it was before.
///   * B40 and B42 concern the engine's own example clients and CLI.
inline constexpr int kContractBLevel = 35;

}  // namespace sqe
}  // namespace tcn

#define TCN_SQE_CONTRACT_B_LEVEL 35

#endif  // TCN_SQE_CONTRACT_H
