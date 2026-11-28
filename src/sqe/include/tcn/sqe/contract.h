// SPDX-License-Identifier: internal
//
// tcn::sqe — the SQE contract core. Header-only, C++17, dependency-free.
//
// One include for the whole component. See
// docs/design/2026-08-31-sqe-contract-library-design.md.
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

// The protocol floor, `kProtocolBreak` / `TCN_SQE_PROTOCOL_BREAK` (#35 R5).
#include "tcn/sqe/protocol.h"

#include "tcn/sqe/capabilities.h"
#include "tcn/sqe/client_id.h"
#include "tcn/sqe/error.h"
#include "tcn/sqe/frame_ref.h"
#include "tcn/sqe/handle.h"
#include "tcn/sqe/node_name.h"
#include "tcn/sqe/reply.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/scope.h"
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

#endif  // TCN_SQE_CONTRACT_H
