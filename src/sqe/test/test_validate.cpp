// SPDX-License-Identifier: internal
//
// The two naming rules. This is the file that has to agree with the engine
// byte for byte.
#include <string>
#include <string_view>

#include "check.h"
#include "tcn/sqe/validate.h"

using namespace tcn::sqe;

// The rules are usable in a constant expression. Not decoration: it is what
// lets a consumer validate a compile-time constant id at compile time.
static_assert(valid_identifier("node_1"), "");
static_assert(!valid_identifier("a/b"), "");
static_assert(valid_fragment("tcn/loc/pcpd/cam1"), "");
static_assert(!valid_fragment("a//b"), "");
static_assert(is_fragment_char('/') && !is_identifier_char('/'), "");

namespace {

/// The identifier alphabet, spelled out independently of the implementation.
/// The point of writing it a second way is that the sweep below compares two
/// spellings rather than comparing the implementation with itself.
const std::string kIdentifierAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "_.:-";

}  // namespace

// ---------------------------------------------------------------------------
// The character classes, swept over all 256 byte values
// ---------------------------------------------------------------------------

TCN_SQE_TEST(identifier_char_class_admits_exactly_the_alphabet)
{
    for (int i = 0; i < 256; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(i);
        const bool expected = kIdentifierAlphabet.find(static_cast<char>(c)) != std::string::npos;
        CHECK_MSG(is_identifier_char(c) == expected, "identifier alphabet disagrees at a byte value");
    }
}

TCN_SQE_TEST(fragment_char_class_is_the_identifier_class_plus_slash_and_nothing_else)
{
    for (int i = 0; i < 256; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(i);
        const bool expected = is_identifier_char(c) || c == '/';
        CHECK_MSG(is_fragment_char(c) == expected, "fragment alphabet is not identifier + '/'");
    }
}

TCN_SQE_TEST(the_allow_list_structurally_excludes_wildcards_whitespace_and_control)
{
    // These are never named by either rule. Only a sweep proves they are out,
    // which is the whole argument for an allow-list.
    const char* const excluded = "*?#$ \t\n\r\v\f/@+%()[]{}<>!\"'`~^&|;,=";
    for (const char* p = excluded; *p != '\0'; ++p)
    {
        const unsigned char c = static_cast<unsigned char>(*p);
        CHECK_MSG(!is_identifier_char(c), "identifier rule admitted an excluded character");
        if (c != '/') { CHECK_MSG(!is_fragment_char(c), "fragment rule admitted an excluded character"); }
    }
    for (int i = 0; i < 0x20; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(i);
        CHECK_MSG(!is_identifier_char(c) && !is_fragment_char(c), "a control byte was admitted");
    }
    CHECK(!is_identifier_char(0x7F) && !is_fragment_char(0x7F));
    for (int i = 0x80; i < 256; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(i);
        CHECK_MSG(!is_identifier_char(c) && !is_fragment_char(c), "a non-ASCII byte was admitted");
    }
}

// ---------------------------------------------------------------------------
// identifier
// ---------------------------------------------------------------------------

TCN_SQE_TEST(identifier_accepts_the_shapes_the_contract_names)
{
    const char* const good[] = {
        "a", "A", "0", "node_1", "marker.7", "ns:name", "a-b", "a::b",
        "camera_left", "0123456789", kIdentifierAlphabet.c_str(),
    };
    for (const char* g : good)
    {
        CHECK_MSG(valid_identifier(g), g);
        CHECK_OK(check_identifier(g));
    }
}

TCN_SQE_TEST(identifier_rejects_empty)
{
    CHECK(!valid_identifier(""));
    CHECK_ERR(check_identifier(""), Error::Empty);
}

TCN_SQE_TEST(identifier_rejects_slash_because_a_name_is_not_a_key_prefix)
{
    CHECK(!valid_identifier("a/b"));
    CHECK_ERR(check_identifier("a/b"), Error::ContainsSlash);
    CHECK(!valid_identifier("tcn/loc/pcpd/cam1"));
}

TCN_SQE_TEST(identifier_rejects_wildcards_whitespace_and_control)
{
    CHECK_ERR(check_identifier("a*b"), Error::ContainsWildcard);
    CHECK_ERR(check_identifier("a?b"), Error::ContainsWildcard);
    CHECK_ERR(check_identifier("a#b"), Error::ContainsWildcard);
    CHECK_ERR(check_identifier("a$b"), Error::ContainsWildcard);
    CHECK_ERR(check_identifier("a b"), Error::ContainsWhitespace);
    CHECK_ERR(check_identifier("a\tb"), Error::ContainsWhitespace);
    CHECK_ERR(check_identifier("a\nb"), Error::ContainsWhitespace);
    CHECK_ERR(check_identifier(std::string_view("a\x01" "b", 3)), Error::ContainsWhitespace);
    CHECK_ERR(check_identifier(std::string_view("a\x7F" "b", 3)), Error::ContainsWhitespace);
    CHECK_ERR(check_identifier("a@b"), Error::IllegalCharacter);
    CHECK_ERR(check_identifier("a+b"), Error::IllegalCharacter);
}

// The aarch64 case, and the reason `<cctype>` is banned. Every byte here is
// >= 0x80, which is *negative* when `char` is signed (x86) and positive when
// it is unsigned (Jetson, Android). `std::isalnum` on the former is undefined
// behaviour; the range check over `unsigned char` behaves identically on both.
TCN_SQE_TEST(identifier_rejects_non_ascii_on_a_signed_and_an_unsigned_char_target)
{
    CHECK_ERR(check_identifier("caf\xC3\xA9"), Error::NonAscii);            // "café", UTF-8
    CHECK_ERR(check_identifier("m\xC3\xBC" "ller"), Error::NonAscii);       // "müller", UTF-8
    CHECK_ERR(check_identifier("\xE3\x81\x82"), Error::NonAscii);           // "あ", UTF-8
    CHECK_ERR(check_identifier(std::string_view("\x80", 1)), Error::NonAscii);
    CHECK_ERR(check_identifier(std::string_view("\xFF", 1)), Error::NonAscii);

    // Stated as a property rather than as five cases: no byte with the high
    // bit set is ever admitted, whatever the platform's signedness of `char`.
    for (int i = 0x80; i < 256; ++i)
    {
        const char raw = static_cast<char>(i);
        const std::string s(1, raw);
        CHECK_MSG(!valid_identifier(s), "a high-bit byte was admitted as an identifier");
        CHECK_MSG(!valid_fragment(s), "a high-bit byte was admitted as a fragment");
    }
}

// ---------------------------------------------------------------------------
// fragment
// ---------------------------------------------------------------------------

TCN_SQE_TEST(fragment_accepts_a_client_id_shaped_key_prefix)
{
    const char* const good[] = {
        "a", "a/b", "tcn/loc/pcpd/cam1", "tcn/loc/pcpd", "a-b.c:d/e_f",
        "one/two/three/four/five",
    };
    for (const char* g : good)
    {
        CHECK_MSG(valid_fragment(g), g);
        CHECK_OK(check_fragment(g));
    }
}

TCN_SQE_TEST(fragment_rejects_empty)
{
    CHECK(!valid_fragment(""));
    CHECK_ERR(check_fragment(""), Error::Empty);
}

// Every empty-chunk case, individually. An empty chunk is not a legal key
// expression segment, and each of these three produces one a different way.
TCN_SQE_TEST(fragment_rejects_every_empty_chunk_case)
{
    CHECK(!valid_fragment("/a"));
    CHECK_ERR(check_fragment("/a"), Error::LeadingSlash);
    CHECK(!valid_fragment("/tcn/loc"));
    CHECK_ERR(check_fragment("/tcn/loc"), Error::LeadingSlash);

    CHECK(!valid_fragment("a/"));
    CHECK_ERR(check_fragment("a/"), Error::TrailingSlash);
    CHECK(!valid_fragment("tcn/loc/"));
    CHECK_ERR(check_fragment("tcn/loc/"), Error::TrailingSlash);

    CHECK(!valid_fragment("a//b"));
    CHECK_ERR(check_fragment("a//b"), Error::EmptyChunk);
    CHECK(!valid_fragment("a///b"));
    CHECK_ERR(check_fragment("a///b"), Error::EmptyChunk);
    CHECK(!valid_fragment("tcn//loc/pcpd"));
    CHECK_ERR(check_fragment("tcn//loc/pcpd"), Error::EmptyChunk);

    // Degenerate values that are only slashes: leading is reported first.
    CHECK(!valid_fragment("/"));
    CHECK_ERR(check_fragment("/"), Error::LeadingSlash);
    CHECK(!valid_fragment("//"));
    CHECK_ERR(check_fragment("//"), Error::LeadingSlash);
}

TCN_SQE_TEST(fragment_rejects_wildcards_whitespace_control_and_non_ascii)
{
    CHECK_ERR(check_fragment("a/*"), Error::ContainsWildcard);
    CHECK_ERR(check_fragment("**/a"), Error::ContainsWildcard);
    CHECK_ERR(check_fragment("a/$" "*b"), Error::ContainsWildcard);
    CHECK_ERR(check_fragment("a/?"), Error::ContainsWildcard);
    CHECK_ERR(check_fragment("a/#"), Error::ContainsWildcard);
    CHECK_ERR(check_fragment("a b/c"), Error::ContainsWhitespace);
    CHECK_ERR(check_fragment("a/b\tc"), Error::ContainsWhitespace);
    CHECK_ERR(check_fragment(std::string_view("a/\x01", 3)), Error::ContainsWhitespace);
    CHECK_ERR(check_fragment("tcn/l\xC3\xB8" "c"), Error::NonAscii);
    CHECK_ERR(check_fragment("a/b@c"), Error::IllegalCharacter);
}

// The alphabet is checked before the chunk structure, so a value that is wrong
// in both ways is reported against the alphabet. Pinned because a consumer may
// switch on the error.
TCN_SQE_TEST(fragment_reports_the_alphabet_before_the_chunk_structure)
{
    CHECK_ERR(check_fragment("/a*b"), Error::ContainsWildcard);
    CHECK_ERR(check_fragment("a//b c"), Error::ContainsWhitespace);
}

// The one difference between the two rules, asserted as a difference.
TCN_SQE_TEST(the_fragment_rule_differs_from_the_identifier_rule_by_exactly_one_character)
{
    const char* const with_slash[] = {"a/b", "tcn/loc/pcpd/cam1", "x/y"};
    for (const char* s : with_slash)
    {
        CHECK_MSG(valid_fragment(s), "fragment must accept '/'");
        CHECK_MSG(!valid_identifier(s), "identifier must reject '/'");
    }

    // For every value containing no '/', the two rules agree exactly.
    const char* const no_slash[] = {"a", "a-b", "a.b", "a:b", "a_b", "", "a b", "a*b", "caf\xC3\xA9"};
    for (const char* s : no_slash)
    {
        CHECK_MSG(valid_identifier(s) == valid_fragment(s),
                  "with no '/' present the two rules must agree");
    }
}
