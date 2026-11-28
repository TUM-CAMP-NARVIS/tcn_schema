// SPDX-License-Identifier: internal
//
// FrameRef and its N2a text form, run against the shared vector file
// (src/sqe/vectors/frame_ref.tsv) that the engine's Rust tests also run.
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

#include "check.h"
#include "stub_msgs.h"
#include "tcn/sqe/frame_ref.h"

using namespace tcn::sqe;

#ifndef TCN_SQE_VECTORS_DIR
#error "TCN_SQE_VECTORS_DIR must name src/sqe/vectors"
#endif

// No public constructor: a FrameRef exists only if it validated.
static_assert(!std::is_default_constructible<FrameRef>::value, "");
static_assert(!std::is_constructible<FrameRef, std::string>::value, "");
static_assert(!std::is_constructible<FrameRef, const char*>::value, "");
static_assert(!std::is_constructible<FrameRef, std::string_view>::value, "");
static_assert(std::is_copy_constructible<FrameRef>::value, "");

static_assert(std::is_same<std::underlying_type<NodeScope>::type, std::int32_t>::value, "");
static_assert(std::is_same<std::underlying_type<FrameSelector>::type, std::int32_t>::value, "");

namespace {

// ---------------------------------------------------------------------------
// The vector file
// ---------------------------------------------------------------------------

struct Row
{
    int line = 0;
    std::vector<std::string> fields;
};

/// Splits on exactly one TAB; no trimming, no quoting -- the file's own rules.
std::vector<std::string> split_tabs(const std::string& s)
{
    std::vector<std::string> out;
    std::string cur;
    for (const char c : s)
    {
        if (c == '\t') { out.push_back(cur); cur.clear(); }
        else { cur += c; }
    }
    out.push_back(cur);
    return out;
}

std::vector<Row> load_vectors()
{
    std::vector<Row> rows;
    std::FILE* f = std::fopen(TCN_SQE_VECTORS_DIR "/frame_ref.tsv", "rb");
    if (f == nullptr) { return rows; }
    std::string line;
    int number = 0;
    int c = 0;
    while (true)
    {
        c = std::fgetc(f);
        if (c == '\n' || c == EOF)
        {
            ++number;
            if (!line.empty() && line.back() == '\r') { line.pop_back(); }
            if (!line.empty() && line[0] != '#') { rows.push_back(Row{number, split_tabs(line)}); }
            line.clear();
            if (c == EOF) { break; }
        }
        else
        {
            line += static_cast<char>(c);
        }
    }
    std::fclose(f);
    return rows;
}

std::string at(const Row& r, const char* what)
{
    return std::string(what) + " (frame_ref.tsv:" + std::to_string(r.line) + ")";
}

/// A field comparison that names the vector file's line on failure.
void row_eq(const Row& r, const char* field, const std::string& actual, const std::string& expected)
{
    const std::string what = std::string(field) + ": actual \"" + actual + "\", expected \"" + expected + "\"";
    CHECK_MSG(actual == expected, at(r, what.c_str()).c_str());
}

const char* scope_text(NodeScope s)
{
    switch (s)
    {
        case NodeScope::Local:   return "local";
        case NodeScope::Global:  return "global";
        case NodeScope::Domain:  return "domain";
        case NodeScope::Unknown: break;
    }
    return "unknown";
}

const char* selector_text(FrameSelector s)
{
    switch (s)
    {
        case FrameSelector::Frame:     return "frame";
        case FrameSelector::Instances: return "instances";
        case FrameSelector::Unknown:   break;
    }
    return "unknown";
}

using tcn_types::msg::SRFrameRef;
using tcn_types::msg::SRFrameSelector;
using tcn_types::msg::SRNodeScope;
using tcn_types::msg::SR_NODE_SCOPE_UNKNOWN;
using tcn_types::msg::SR_NODE_SCOPE_LOCAL;
using tcn_types::msg::SR_NODE_SCOPE_GLOBAL;
using tcn_types::msg::SR_NODE_SCOPE_DOMAIN;
using tcn_types::msg::SR_FRAME_SELECTOR_UNKNOWN;
using tcn_types::msg::SR_FRAME_SELECTOR_FRAME;
using tcn_types::msg::SR_FRAME_SELECTOR_INSTANCES;

SRFrameRef wire(std::uint32_t scope, const char* owner, const char* name, std::uint32_t selector)
{
    SRFrameRef w;
    w.scope(static_cast<SRNodeScope>(scope));
    w.owner(owner);
    w.name(name);
    w.selector(static_cast<SRFrameSelector>(selector));
    return w;
}

}  // namespace

// The file is found and is not trivially short; a missing file would
// otherwise make every row-driven test below pass vacuously.
TCN_SQE_TEST(the_vector_file_is_present_and_well_formed)
{
    const std::vector<Row> rows = load_vectors();
    CHECK_MSG(rows.size() >= 60, "frame_ref.tsv is missing or has too few rows");
    int ok_rows = 0;
    int err_rows = 0;
    for (const Row& r : rows)
    {
        const std::string& kind = r.fields[0];
        if (kind == "ok")
        {
            ++ok_rows;
            CHECK_MSG(r.fields.size() == 6, at(r, "an ok row has six fields").c_str());
        }
        else if (kind == "err")
        {
            ++err_rows;
            CHECK_MSG(r.fields.size() == 3, at(r, "an err row has three fields").c_str());
        }
        else
        {
            CHECK_MSG(false, at(r, "a row is neither ok nor err").c_str());
        }
    }
    CHECK(ok_rows >= 7);
    CHECK(err_rows >= 20);
}

TCN_SQE_TEST(every_ok_vector_parses_to_its_fields_and_formats_back)
{
    for (const Row& r : load_vectors())
    {
        if (r.fields[0] != "ok" || r.fields.size() != 6) { continue; }
        const Result<FrameRef> f = FrameRef::parse(r.fields[1]);
        if (!f)
        {
            CHECK_MSG(false, at(r, (std::string("refused: ") + code(f.error())).c_str()).c_str());
            continue;
        }
        row_eq(r, "scope", scope_text(f.value().scope()), r.fields[2]);
        row_eq(r, "owner", f.value().owner(), r.fields[3]);
        row_eq(r, "name", f.value().name(), r.fields[4]);
        row_eq(r, "selector", selector_text(f.value().selector()), r.fields[5]);
        row_eq(r, "format", f.value().format(), r.fields[1]);
    }
}

TCN_SQE_TEST(every_err_vector_is_refused_with_its_code)
{
    for (const Row& r : load_vectors())
    {
        if (r.fields[0] != "err" || r.fields.size() != 3) { continue; }
        const Result<FrameRef> f = FrameRef::parse(r.fields[1]);
        if (f)
        {
            CHECK_MSG(false, at(r, ("accepted: \"" + r.fields[1] + "\"").c_str()).c_str());
            continue;
        }
        row_eq(r, "code", code(f.error()), r.fields[2]);
    }
}

// ---------------------------------------------------------------------------
// The named constructors
// ---------------------------------------------------------------------------

TCN_SQE_TEST(the_named_constructors_validate_each_half_by_its_own_rule)
{
    const Result<FrameRef> l = FrameRef::local("tcn/loc/pcpd/hl2-01", "head");
    CHECK_OK(l);
    if (l)
    {
        CHECK(l.value().scope() == NodeScope::Local);
        CHECK(l.value().selector() == FrameSelector::Frame);
        CHECK_STR_EQ(l.value().format(), "local:tcn/loc/pcpd/hl2-01/head");
    }
    CHECK_ERR(FrameRef::local("tcn/loc", "head/left"), Error::ContainsSlash);
    CHECK_ERR(FrameRef::local("/tcn/loc", "head"), Error::LeadingSlash);
    CHECK_ERR(FrameRef::local("tcn/loc/", "head"), Error::TrailingSlash);
    CHECK_ERR(FrameRef::local("", "head"), Error::Empty);   // "this fragment" has no text form
    CHECK_ERR(FrameRef::local("tcn/loc", ""), Error::Empty);

    const Result<FrameRef> g = FrameRef::global("world");
    CHECK_OK(g);
    if (g) { CHECK(g.value().owner().empty()); CHECK_STR_EQ(g.value().format(), "global:world"); }
    CHECK_ERR(FrameRef::global("a/b"), Error::ContainsSlash);
    CHECK_ERR(FrameRef::global(""), Error::Empty);

    const Result<FrameRef> d = FrameRef::domain("tcn/ksv1/rig01", "cam0");
    CHECK_OK(d);
    if (d) { CHECK(d.value().scope() == NodeScope::Domain); CHECK_STR_EQ(d.value().format(), "domain:tcn/ksv1/rig01/cam0"); }
    // The `::` rule of 0.10.7 went with N1: a domain is a fragment, nothing more.
    CHECK_OK(FrameRef::domain("tcn::ksv1", "cam"));
    CHECK_ERR(FrameRef::domain("", "cam"), Error::Empty);
    CHECK_ERR(FrameRef::domain("tcn/ksv1", "a/b"), Error::ContainsSlash);
}

TCN_SQE_TEST(instances_sets_the_selector_and_nothing_else)
{
    const FrameRef f = FrameRef::local("dev1", "hmd").value();
    const FrameRef all = f.instances();
    CHECK(all.selector() == FrameSelector::Instances);
    CHECK(f.selector() == FrameSelector::Frame);
    CHECK_STR_EQ(all.format(), "local:dev1/hmd[instances]");
    CHECK(all != f);
    CHECK(all.instances() == all);
    CHECK(FrameRef::parse("local:dev1/hmd[instances]").value() == all);
}

TCN_SQE_TEST(frame_refs_compare_on_every_field_and_order_totally)
{
    const FrameRef a = FrameRef::parse("local:a/x").value();
    CHECK(a == FrameRef::local("a", "x").value());
    CHECK(a != FrameRef::local("a", "y").value());
    CHECK(a != FrameRef::local("b", "x").value());
    CHECK(a != FrameRef::domain("a", "x").value());
    CHECK(a != a.instances());

    CHECK(FrameRef::local("a", "z").value() < FrameRef::local("b", "a").value());
    CHECK(FrameRef::local("a", "a").value() < FrameRef::local("a", "b").value());
    CHECK(a < a.instances());
    CHECK(!(a < a));
    // Different scopes are ordered too, so a std::map key never collides.
    const FrameRef g = FrameRef::global("x").value();
    CHECK((a < g) != (g < a));
}

// ---------------------------------------------------------------------------
// The wire
// ---------------------------------------------------------------------------

TCN_SQE_TEST(scope_and_selector_ordinals_match_the_idl_declaration_order)
{
    CHECK_INT_EQ(static_cast<int>(NodeScope::Unknown), 0);
    CHECK_INT_EQ(static_cast<int>(NodeScope::Local), 1);
    CHECK_INT_EQ(static_cast<int>(NodeScope::Global), 2);
    CHECK_INT_EQ(static_cast<int>(NodeScope::Domain), 3);
    CHECK_INT_EQ(static_cast<int>(FrameSelector::Unknown), 0);
    CHECK_INT_EQ(static_cast<int>(FrameSelector::Frame), 1);
    CHECK_INT_EQ(static_cast<int>(FrameSelector::Instances), 2);
}

TCN_SQE_TEST(to_wire_then_from_wire_is_identity)
{
    const char* const texts[] = {
        "local:tcn/loc/ch/room/camera", "global:world", "domain:tcn/ksv1/rig01/cam0",
        "local:dev1/hmd[instances]", "global:a:b[instances]", "domain:d/n.x-1_y",
    };
    for (const char* t : texts)
    {
        const FrameRef f = FrameRef::parse(t).value();
        SRFrameRef w;
        f.to_wire(w);
        const Result<FrameRef> back = FrameRef::from_wire(w);
        CHECK_OK(back);
        if (back) { CHECK(back.value() == f); CHECK_STR_EQ(back.value().format(), t); }
    }

    SRFrameRef w;
    FrameRef::parse("domain:tcn/ksv1/rig01/cam0[instances]").value().to_wire(w);
    CHECK_INT_EQ(w.scope(), SR_NODE_SCOPE_DOMAIN);
    CHECK_STR_EQ(w.owner(), "tcn/ksv1/rig01");
    CHECK_STR_EQ(w.name(), "cam0");
    CHECK_INT_EQ(w.selector(), SR_FRAME_SELECTOR_INSTANCES);

    FrameRef::global("world").value().to_wire(w);
    CHECK_INT_EQ(w.scope(), SR_NODE_SCOPE_GLOBAL);
    CHECK_STR_EQ(w.owner(), "");     // overwritten, not left from the previous value
    CHECK_INT_EQ(w.selector(), SR_FRAME_SELECTOR_FRAME);
}

// An empty LOCAL owner means "this fragment" and exists only inside a join; a
// FrameRef never holds one (Review Focus 5).
TCN_SQE_TEST(from_wire_refuses_what_has_no_text_form)
{
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_LOCAL, "", "head", SR_FRAME_SELECTOR_FRAME)), Error::MissingOwner);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_DOMAIN, "", "cam", SR_FRAME_SELECTOR_FRAME)), Error::MissingOwner);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_GLOBAL, "x", "world", SR_FRAME_SELECTOR_FRAME)), Error::UnexpectedOwner);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_UNKNOWN, "a", "b", SR_FRAME_SELECTOR_FRAME)), Error::UnknownScope);
    CHECK_ERR(FrameRef::from_wire(wire(7, "a", "b", SR_FRAME_SELECTOR_FRAME)), Error::UnknownScope);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_LOCAL, "a", "b", SR_FRAME_SELECTOR_UNKNOWN)), Error::UnknownSelector);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_LOCAL, "a", "b", 3)), Error::UnknownSelector);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_LOCAL, "a//b", "c", SR_FRAME_SELECTOR_FRAME)), Error::EmptyChunk);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_LOCAL, "a", "b/c", SR_FRAME_SELECTOR_FRAME)), Error::ContainsSlash);
    CHECK_ERR(FrameRef::from_wire(wire(SR_NODE_SCOPE_GLOBAL, "", "", SR_FRAME_SELECTOR_FRAME)), Error::Empty);
    CHECK_OK(FrameRef::from_wire(wire(SR_NODE_SCOPE_LOCAL, "a", "b", SR_FRAME_SELECTOR_INSTANCES)));
}

// ---------------------------------------------------------------------------
// The codes
// ---------------------------------------------------------------------------

TCN_SQE_TEST(error_codes_are_the_snake_case_of_the_enumerators)
{
    CHECK_STR_EQ(code(Error::Ok), "ok");
    CHECK_STR_EQ(code(Error::Empty), "empty");
    CHECK_STR_EQ(code(Error::IllegalCharacter), "illegal_character");
    CHECK_STR_EQ(code(Error::NonAscii), "non_ascii");
    CHECK_STR_EQ(code(Error::ContainsWildcard), "contains_wildcard");
    CHECK_STR_EQ(code(Error::ContainsWhitespace), "contains_whitespace");
    CHECK_STR_EQ(code(Error::ContainsSlash), "contains_slash");
    CHECK_STR_EQ(code(Error::EmptyChunk), "empty_chunk");
    CHECK_STR_EQ(code(Error::LeadingSlash), "leading_slash");
    CHECK_STR_EQ(code(Error::TrailingSlash), "trailing_slash");
    CHECK_STR_EQ(code(Error::MissingScheme), "missing_scheme");
    CHECK_STR_EQ(code(Error::UnknownScheme), "unknown_scheme");
    CHECK_STR_EQ(code(Error::MissingOwner), "missing_owner");
    CHECK_STR_EQ(code(Error::UnexpectedOwner), "unexpected_owner");
    CHECK_STR_EQ(code(Error::MalformedSelector), "malformed_selector");
    CHECK_STR_EQ(code(Error::UnknownScope), "unknown_scope");
    CHECK_STR_EQ(code(Error::UnknownSelector), "unknown_selector");
    CHECK_STR_EQ(code(Error::MalformedCapability), "malformed_capability");
    CHECK_STR_EQ(code(Error::ReplyRefused), "reply_refused");
    CHECK_STR_EQ(code(Error::MissingSchemaVersion), "missing_schema_version");
    CHECK_STR_EQ(code(Error::SchemaVersionMismatch), "schema_version_mismatch");
    CHECK_STR_EQ(code(Error::HandleInvariantViolated), "handle_invariant_violated");
    CHECK_STR_EQ(code(Error::FreshnessBiasOutOfRange), "freshness_bias_out_of_range");
}

// Every enumerator has its own code and none falls through to the default.
TCN_SQE_TEST(every_error_has_a_distinct_code)
{
    std::vector<std::string> seen;
    for (int i = 0; i <= static_cast<int>(Error::MissingSchemaVersion); ++i)
    {
        const std::string c = code(static_cast<Error>(i));
        CHECK_MSG(c != "unknown", "an Error has no code");
        for (const std::string& s : seen) { CHECK_MSG(s != c, "two Errors share a code"); }
        seen.push_back(c);
    }
}

// N2a: a text with no scheme, or an unknown one, is refused with a message
// that says which forms exist.
TCN_SQE_TEST(a_scheme_error_names_the_forms_that_exist)
{
    const Error errors[] = {Error::MissingScheme, Error::UnknownScheme};
    for (const Error e : errors)
    {
        const std::string d = describe(e);
        CHECK_MSG(d.find("local:<fragment>/<name>") != std::string::npos, d.c_str());
        CHECK_MSG(d.find("global:<name>") != std::string::npos, d.c_str());
        CHECK_MSG(d.find("domain:<domain>/<name>") != std::string::npos, d.c_str());
        CHECK_MSG(d.find("[instances]") != std::string::npos, d.c_str());
    }
}
