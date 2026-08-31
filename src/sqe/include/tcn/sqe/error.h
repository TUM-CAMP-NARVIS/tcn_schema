// SPDX-License-Identifier: internal
//
// tcn::sqe — the dependency-free SQE contract core.
//
// Every failure in this library is a returned `Error`. Nothing here throws,
// allocates on the failure path, or consults a locale.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.1.
#ifndef TCN_SQE_ERROR_H
#define TCN_SQE_ERROR_H

namespace tcn {
namespace sqe {

/// Why an operation refused.
///
/// Ordinals are append-only: a consumer may log the integer, and the
/// unimplemented entries below are reserved by the design (§4.1) so that
/// implementing the rest of the core later cannot renumber the ones that
/// already ship.
enum class Error : int {
    Ok = 0,

    // --- naming (§4.2, §4.3) ------------------------------------------------
    /// The value was empty. Neither rule admits an empty string.
    Empty,
    /// A byte outside the rule's allow-list. The four entries after this one
    /// are *diagnoses* of the same refusal, never a separate rule: the
    /// decision is always "not in the allow-list" (see validate.h).
    IllegalCharacter,
    /// Diagnosis: the offending byte was >= 0x80.
    NonAscii,
    /// Diagnosis: the offending byte was a Zenoh wildcard (`*`, `?`, `#`, `$`).
    ContainsWildcard,
    /// Diagnosis: the offending byte was a space, tab, newline or other
    /// C0/DEL control character.
    ContainsWhitespace,
    /// Diagnosis: the offending byte was `/`, which the identifier rule — but
    /// not the fragment rule — excludes.
    ContainsSlash,
    /// A fragment contained an empty chunk, i.e. `//` somewhere inside it.
    EmptyChunk,
    /// A fragment began with `/`, which makes its first chunk empty.
    LeadingSlash,
    /// A fragment ended with `/`, which makes its last chunk empty.
    TrailingSlash,

    // --- reserved by the design, not produced by this component -------------
    /// Reserved for FragmentBuilder (§4.9); not implemented here.
    OriginNotDeclared,

    // --- relation-stream replies (§4.4) -------------------------------------
    /// The reply carried no handle. Not necessarily a fault: a `NoPath`,
    /// `UnknownFrame`, `Rejected` or `Inactive` reply correctly carries none.
    HandleAbsent,
    /// The reply's status and handle disagree. `ASSIGNED` must occur exactly
    /// when the status is `Active` or `Pending`. Reported rather than papered
    /// over, because either half may be the wrong one.
    HandleInvariantViolated,

    /// A second `acquire` for a relation this process already leases, on
    /// different terms. Implemented by `RelationLeaseTable` (§4.6) in the
    /// `sqe_session` component; the ordinal was reserved here from the start.
    ConflictingLease,

    // --- wire encoding (§4.7) -----------------------------------------------
    /// The payload declared a type name other than the one asked for.
    WrongDeclaredType,

    /// Reserved for the pose publication shapes (§4.8); not implemented here.
    MoreThanOneItem,

    // --- appended after the design's list ------------------------------------
    /// The encoding annotation carried no `;<type>` suffix at all. CDR is not
    /// self-describing, so an unannotated payload cannot be decoded safely.
    MissingDeclaredType,
    /// The encoding annotation's media type was not one this library knows.
    UnknownEncoding,
    /// The reply named an assigned handle whose topic was the empty string.
    EmptyHandleTopic,

    // --- the session layer (sqe_session) --------------------------------------
    //
    // Appended, never inserted: a consumer may log the integer.

    /// No transport, or no start/stop function, was supplied. A programming
    /// error, reported rather than dereferenced.
    NoTransport,
    /// The adapter refused or failed for a reason of its own. It logs the
    /// detail; this library does not log at all.
    TransportFailed,
    /// A query's timeout elapsed with no reply. Distinct from
    /// `TransportFailed` because the request may still be in flight and may
    /// still be acted on by the engine.
    Timeout,
    /// The consumer's codec could not serialise the request.
    EncodeFailed,
    /// The consumer's codec could not deserialise a reply whose declared type
    /// was already checked and matched.
    DecodeFailed,
    /// `freshness_bias` was outside [0, 1] — or was NaN, which fails the same
    /// comparison.
    FreshnessBiasOutOfRange,
    /// A fixed-rate trigger's `hz` was not a positive, finite number.
    InvalidTriggerRate,
    /// An on-stream trigger named no topic.
    EmptyTriggerTopic,
    /// The liveliness token could not be declared, so the session was not
    /// opened. A client with no token is not visible to the engine (B41), and
    /// this library will not pretend otherwise by continuing without one.
    PresenceUnavailable,
};

/// A static, ASCII, locale-independent description. Never null, never
/// allocates; safe to hand to whichever logger the consumer has.
inline const char* describe(Error e) noexcept
{
    switch (e)
    {
        case Error::Ok:                      return "ok";
        case Error::Empty:                   return "empty value";
        case Error::IllegalCharacter:        return "character outside the allowed set";
        case Error::NonAscii:                return "non-ASCII byte";
        case Error::ContainsWildcard:        return "zenoh wildcard character";
        case Error::ContainsWhitespace:      return "whitespace or control character";
        case Error::ContainsSlash:           return "'/' is not allowed in an identifier";
        case Error::EmptyChunk:              return "empty chunk ('//')";
        case Error::LeadingSlash:            return "leading '/'";
        case Error::TrailingSlash:           return "trailing '/'";
        case Error::OriginNotDeclared:       return "origin node was not declared";
        case Error::HandleAbsent:            return "reply carries no handle";
        case Error::HandleInvariantViolated: return "reply status and handle disagree";
        case Error::ConflictingLease:        return "an existing lease has different terms";
        case Error::WrongDeclaredType:       return "payload declares a different type";
        case Error::MoreThanOneItem:         return "more than one item";
        case Error::MissingDeclaredType:     return "encoding declares no type name";
        case Error::UnknownEncoding:         return "unrecognised encoding media type";
        case Error::EmptyHandleTopic:        return "assigned handle has an empty topic";
        case Error::NoTransport:             return "no transport was supplied";
        case Error::TransportFailed:         return "the transport reported a failure";
        case Error::Timeout:                 return "the query timed out";
        case Error::EncodeFailed:            return "the codec could not encode the request";
        case Error::DecodeFailed:            return "the codec could not decode the reply";
        case Error::FreshnessBiasOutOfRange: return "freshness_bias is not in [0, 1]";
        case Error::InvalidTriggerRate:      return "trigger rate is not a positive finite value";
        case Error::EmptyTriggerTopic:       return "on-stream trigger names no topic";
        case Error::PresenceUnavailable:     return "the liveliness token could not be declared";
    }
    return "unknown error";
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_ERROR_H
