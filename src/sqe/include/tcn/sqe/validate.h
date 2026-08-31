// SPDX-License-Identifier: internal
//
// The two naming rules, written once as an allow-list.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.3.
#ifndef TCN_SQE_VALIDATE_H
#define TCN_SQE_VALIDATE_H

#include <cstddef>
#include <string_view>

#include "tcn/sqe/error.h"
#include "tcn/sqe/result.h"

namespace tcn {
namespace sqe {

// ---------------------------------------------------------------------------
// The character classes
// ---------------------------------------------------------------------------
//
// `<cctype>` is not used, and its absence is load-bearing rather than
// stylistic. `std::isalnum` and friends are (a) locale-sensitive, so what they
// accept depends on a global this library does not own, and (b) undefined
// behaviour for a negative `int` argument — and plain `char` is *signed* on
// x86 and *unsigned* on aarch64, so a validator written with them behaves
// differently on Jetson and Android from how it behaves on a developer's
// desktop. Everything below is a range comparison over `unsigned char`, which
// is correct on every target and cheap enough to be `constexpr`.

/// The identifier alphabet: `[A-Za-z0-9_.:-]`.
constexpr bool is_identifier_char(unsigned char c) noexcept
{
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '_' || c == '.' || c == ':' || c == '-';
}

/// The fragment alphabet: the identifier alphabet plus exactly one character,
/// `/`.
///
/// Written as `is_identifier_char(c) || c == '/'` on purpose. The two rules
/// are one rule with one difference, so there is no second alphabet for a
/// later edit to fall out of sync with — and because both are allow-lists,
/// Zenoh's wildcards (`*`, `?`, `#`, `$`), whitespace, control characters and
/// every non-ASCII byte are excluded *structurally*. A blacklist has to be
/// remembered; an allow-list cannot be forgotten.
constexpr bool is_fragment_char(unsigned char c) noexcept
{
    return is_identifier_char(c) || c == '/';
}

// ---------------------------------------------------------------------------
// The two rules
// ---------------------------------------------------------------------------

/// **identifier**: non-empty; every character in `[A-Za-z0-9_.:-]`.
///
/// Governs node names and edge names — anything a topic is later built from.
constexpr bool valid_identifier(std::string_view s) noexcept
{
    if (s.empty()) { return false; }
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (!is_identifier_char(static_cast<unsigned char>(s[i]))) { return false; }
    }
    return true;
}

/// **fragment**: non-empty; every character in `[A-Za-z0-9_.:/-]`; no empty
/// chunk — no leading `/`, no trailing `/`, no `//`.
///
/// Governs a client id, which is a Zenoh key *prefix* and contains `/` by
/// design. Validating one with the identifier rule is what took the daemon
/// down (B39); the two rules are kept apart deliberately.
constexpr bool valid_fragment(std::string_view s) noexcept
{
    if (s.empty()) { return false; }
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (!is_fragment_char(static_cast<unsigned char>(s[i]))) { return false; }
    }
    if (s.front() == '/' || s.back() == '/') { return false; }
    for (std::size_t i = 1; i < s.size(); ++i)
    {
        if (s[i] == '/' && s[i - 1] == '/') { return false; }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Diagnosis
// ---------------------------------------------------------------------------

namespace detail {

/// Names the *reason* a byte was refused, for the benefit of a log line.
///
/// This is not a second rule and does not participate in the decision: the
/// decision is always and only "the byte is not in the allow-list". Adding a
/// case here can never admit a character, and forgetting one can never admit
/// a character either — the worst it can do is produce a vaguer message.
constexpr Error classify_rejected_byte(unsigned char c) noexcept
{
    if (c >= 0x80) { return Error::NonAscii; }
    if (c == '*' || c == '?' || c == '#' || c == '$') { return Error::ContainsWildcard; }
    if (c <= 0x20 || c == 0x7F) { return Error::ContainsWhitespace; }
    if (c == '/') { return Error::ContainsSlash; }
    return Error::IllegalCharacter;
}

}  // namespace detail

/// `valid_identifier` with a reason attached.
inline Result<void> check_identifier(std::string_view s) noexcept
{
    if (s.empty()) { return Result<void>::fail(Error::Empty); }
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (!is_identifier_char(c)) { return Result<void>::fail(detail::classify_rejected_byte(c)); }
    }
    return Result<void>::ok();
}

/// `valid_fragment` with a reason attached.
///
/// The alphabet is checked before the chunk structure, so a value that is
/// wrong in both ways is reported against the alphabet.
inline Result<void> check_fragment(std::string_view s) noexcept
{
    if (s.empty()) { return Result<void>::fail(Error::Empty); }
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (!is_fragment_char(c)) { return Result<void>::fail(detail::classify_rejected_byte(c)); }
    }
    if (s.front() == '/') { return Result<void>::fail(Error::LeadingSlash); }
    if (s.back() == '/') { return Result<void>::fail(Error::TrailingSlash); }
    for (std::size_t i = 1; i < s.size(); ++i)
    {
        if (s[i] == '/' && s[i - 1] == '/') { return Result<void>::fail(Error::EmptyChunk); }
    }
    return Result<void>::ok();
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_VALIDATE_H
