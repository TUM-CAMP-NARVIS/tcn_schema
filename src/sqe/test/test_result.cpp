// SPDX-License-Identifier: internal
//
// Result<T>. Hand-rolled storage, so its lifetime behaviour is tested rather
// than assumed.
#include <string>
#include <utility>

#include "check.h"
#include "tcn/sqe/contract.h"   // also proves the umbrella header compiles alone
#include "tcn/sqe/result.h"

using namespace tcn::sqe;

static_assert(kContractBLevel >= 35, "");
static_assert(TCN_SQE_CONTRACT_B_LEVEL == kContractBLevel, "");

namespace {

int g_live = 0;

struct Counted
{
    int v;
    explicit Counted(int value) : v(value) { ++g_live; }
    Counted(const Counted& o) : v(o.v) { ++g_live; }
    Counted(Counted&& o) noexcept : v(o.v) { ++g_live; }
    Counted& operator=(const Counted& o) { v = o.v; return *this; }
    Counted& operator=(Counted&& o) noexcept { v = o.v; return *this; }
    ~Counted() { --g_live; }
};

}  // namespace

TCN_SQE_TEST(a_result_holds_either_a_value_or_an_error)
{
    const Result<int> ok = Result<int>::ok(7);
    CHECK(ok.is_ok());
    CHECK(static_cast<bool>(ok));
    CHECK(ok.error() == Error::Ok);
    CHECK_INT_EQ(ok.value(), 7);

    const Result<int> bad = Result<int>::fail(Error::Empty);
    CHECK(!bad.is_ok());
    CHECK(!static_cast<bool>(bad));
    CHECK(bad.error() == Error::Empty);
    CHECK_INT_EQ(bad.value_or(-1), -1);
}

TCN_SQE_TEST(a_result_destroys_its_value_exactly_once)
{
    g_live = 0;
    {
        Result<Counted> a = Result<Counted>::ok(Counted(1));
        CHECK_INT_EQ(g_live, 1);
        Result<Counted> b = a;                       // copy
        CHECK_INT_EQ(g_live, 2);
        Result<Counted> c = std::move(b);            // move
        CHECK_INT_EQ(c.value().v, 1);
        a = Result<Counted>::fail(Error::Empty);     // assign over a value
        CHECK(!a.is_ok());
    }
    CHECK_INT_EQ(g_live, 0);

    {
        Result<Counted> e = Result<Counted>::fail(Error::Empty);
        CHECK_INT_EQ(g_live, 0);
        e = Result<Counted>::ok(Counted(3));
        CHECK_INT_EQ(g_live, 1);
        CHECK_INT_EQ(e.value().v, 3);
    }
    CHECK_INT_EQ(g_live, 0);
}

TCN_SQE_TEST(a_result_carries_a_non_default_constructible_value)
{
    // Handle is not default-constructible, which is why Result cannot be a
    // struct with a `T value_` member.
    const Result<std::string> s = Result<std::string>::ok("topic");
    CHECK_STR_EQ(s.value(), "topic");
    CHECK_STR_EQ(std::string(describe(Error::HandleAbsent)), "reply carries no handle");
}

TCN_SQE_TEST(result_void_reports_success_without_a_value)
{
    CHECK(Result<void>::ok().is_ok());
    CHECK(!Result<void>::fail(Error::Empty).is_ok());
    CHECK(Result<void>::fail(Error::Empty).error() == Error::Empty);
}

TCN_SQE_TEST(every_error_has_a_distinct_non_empty_description)
{
    const Error all[] = {
        Error::Ok, Error::Empty, Error::IllegalCharacter, Error::NonAscii,
        Error::ContainsWildcard, Error::ContainsWhitespace, Error::ContainsSlash,
        Error::EmptyChunk, Error::LeadingSlash, Error::TrailingSlash,
        Error::OriginNotDeclared, Error::HandleAbsent, Error::HandleInvariantViolated,
        Error::ConflictingLease, Error::WrongDeclaredType, Error::MoreThanOneItem,
        Error::MissingDeclaredType, Error::UnknownEncoding, Error::EmptyHandleTopic,
    };
    const std::size_t n = sizeof(all) / sizeof(all[0]);
    for (std::size_t i = 0; i < n; ++i)
    {
        const std::string a(describe(all[i]));
        CHECK(!a.empty());
        for (std::size_t j = i + 1; j < n; ++j)
        {
            CHECK_MSG(a != std::string(describe(all[j])), "two errors share a description");
        }
    }
}
