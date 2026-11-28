// SPDX-License-Identifier: internal
//
// tcn::sqe — the dependency-free SQE contract core.
//
// Every failure in this library is a returned `Error`. Nothing here throws,
// allocates on the failure path, or consults a locale.
//
// See docs/design/2026-08-31-sqe-contract-library-design.md §4.1.
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
    /// An OK stream reply carried no handle. Not a fault: the answer to a stop
    /// (relation `Inactive`) correctly carries none. A refusal is
    /// `ReplyRefused`, checked first.
    HandleAbsent,
    /// An OK stream reply's relation state and handle disagree: the handle must
    /// be present exactly when the state is `Active` or `Pending` (#35 H1).
    /// Reported rather than papered over, because either half may be wrong.
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
    /// The encoding annotation's media type was not exactly `application/cdr`
    /// or `application/json` (V1).
    UnknownEncoding,
    /// A start reply's handle had an empty string: the handle, or its data,
    /// status or plan topic.
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

    // --- wire encoding, again (B76) ---------------------------------------
    /// The annotation carried a `tcn_schema=` parameter whose major.minor is
    /// not this library's `kWireSchemaVersion` -- or is not a version at all.
    /// The type name matched; the layout behind it may not. A peer built
    /// against another minor can append a field this reader does not know, or
    /// lack one it expects, and CDR decodes either without complaint.
    SchemaVersionMismatch,

    // --- 0.11 (#35) ----------------------------------------------------------
    //
    // The first five are the N2a text form's own refusals (frame_ref.h); with
    // the naming entries above they are the vocabulary of the shared vector
    // file src/sqe/vectors/frame_ref.tsv, spelled by `code()`.

    /// A frame reference's text had no `:`, so no scheme.
    MissingScheme,
    /// The scheme was not exactly `local`, `global` or `domain`.
    UnknownScheme,
    /// A LOCAL or DOMAIN reference had no owner: no `/` in its text, or an
    /// empty owner on the wire (which means "this fragment" and exists only
    /// inside a join).
    MissingOwner,
    /// A GLOBAL reference had an owner: a `/` in its text, or a non-empty
    /// owner on the wire.
    UnexpectedOwner,
    /// A `[` that is not exactly one trailing `[instances]`.
    MalformedSelector,
    /// A wire `SRNodeScope` of `UNKNOWN` or an ordinal this library does not
    /// know.
    UnknownScope,
    /// A wire `SRFrameSelector` of `UNKNOWN` or an ordinal this library does
    /// not know.
    UnknownSelector,
    /// An engine capability name outside `[a-z0-9_]{1,64}`.
    MalformedCapability,
    /// The reply's header status was not OK; the header says why.
    ReplyRefused,
    /// The encoding annotation carried no `tcn_schema=` parameter. Required
    /// from 0.11 (V1): an unversioned payload is not accepted.
    MissingSchemaVersion,
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
        case Error::SchemaVersionMismatch:   return "encoding declares another tcn_schema version";
        case Error::MissingScheme:           return "frame reference has no scheme; use local:<fragment>/<name>, "
                                                    "global:<name> or domain:<domain>/<name>, optionally + [instances]";
        case Error::UnknownScheme:           return "frame reference scheme is not local, global or domain; use "
                                                    "local:<fragment>/<name>, global:<name> or domain:<domain>/<name>, "
                                                    "optionally + [instances]";
        case Error::MissingOwner:            return "local or domain frame reference has no owner";
        case Error::UnexpectedOwner:         return "global frame reference has an owner";
        case Error::MalformedSelector:       return "frame selector is not a single trailing [instances]";
        case Error::UnknownScope:            return "unknown node scope";
        case Error::UnknownSelector:         return "unknown frame selector";
        case Error::MalformedCapability:     return "capability name is not [a-z0-9_]{1,64}";
        case Error::ReplyRefused:            return "the reply's status is not OK";
        case Error::MissingSchemaVersion:    return "encoding declares no tcn_schema version";
    }
    return "unknown error";
}

/// The stable machine code of an error: the snake_case of its enumerator
/// (`Error::EmptyChunk` -> `"empty_chunk"`). Never null, never allocates.
///
/// Unlike `describe`, these strings are a contract: they are the `code`
/// column of the shared N2a vector file and the codes the engine reports, so
/// an implementation in another language compares against them verbatim.
/// They change only with the enumerator's name.
inline const char* code(Error e) noexcept
{
    switch (e)
    {
        case Error::Ok:                      return "ok";
        case Error::Empty:                   return "empty";
        case Error::IllegalCharacter:        return "illegal_character";
        case Error::NonAscii:                return "non_ascii";
        case Error::ContainsWildcard:        return "contains_wildcard";
        case Error::ContainsWhitespace:      return "contains_whitespace";
        case Error::ContainsSlash:           return "contains_slash";
        case Error::EmptyChunk:              return "empty_chunk";
        case Error::LeadingSlash:            return "leading_slash";
        case Error::TrailingSlash:           return "trailing_slash";
        case Error::OriginNotDeclared:       return "origin_not_declared";
        case Error::HandleAbsent:            return "handle_absent";
        case Error::HandleInvariantViolated: return "handle_invariant_violated";
        case Error::ConflictingLease:        return "conflicting_lease";
        case Error::WrongDeclaredType:       return "wrong_declared_type";
        case Error::MoreThanOneItem:         return "more_than_one_item";
        case Error::MissingDeclaredType:     return "missing_declared_type";
        case Error::UnknownEncoding:         return "unknown_encoding";
        case Error::EmptyHandleTopic:        return "empty_handle_topic";
        case Error::NoTransport:             return "no_transport";
        case Error::TransportFailed:         return "transport_failed";
        case Error::Timeout:                 return "timeout";
        case Error::EncodeFailed:            return "encode_failed";
        case Error::DecodeFailed:            return "decode_failed";
        case Error::FreshnessBiasOutOfRange: return "freshness_bias_out_of_range";
        case Error::InvalidTriggerRate:      return "invalid_trigger_rate";
        case Error::EmptyTriggerTopic:       return "empty_trigger_topic";
        case Error::PresenceUnavailable:     return "presence_unavailable";
        case Error::SchemaVersionMismatch:   return "schema_version_mismatch";
        case Error::MissingScheme:           return "missing_scheme";
        case Error::UnknownScheme:           return "unknown_scheme";
        case Error::MissingOwner:            return "missing_owner";
        case Error::UnexpectedOwner:         return "unexpected_owner";
        case Error::MalformedSelector:       return "malformed_selector";
        case Error::UnknownScope:            return "unknown_scope";
        case Error::UnknownSelector:         return "unknown_selector";
        case Error::MalformedCapability:     return "malformed_capability";
        case Error::ReplyRefused:            return "reply_refused";
        case Error::MissingSchemaVersion:    return "missing_schema_version";
    }
    return "unknown";
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_ERROR_H
