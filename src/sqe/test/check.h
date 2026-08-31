// SPDX-License-Identifier: internal
//
// A ~100-line test harness, header-only and dependency-free.
//
// Deliberately not gtest. `tcn_schema` has no C++ test target and no CI that
// compiles C++ at all today, so the first test target should not also be the
// repository's first third-party test dependency, its first `find_package` in
// a test path, and its first thing to resolve on a build machine that today
// only runs a code generator. The core under test is 100% header-only string
// and type work; asserting on it needs a counter and a message.
//
// No <iostream>: the library forbids it, and the tests keep the same rule so
// that the test target proves the constraint rather than exempting itself.
#ifndef TCN_SQE_TEST_CHECK_H
#define TCN_SQE_TEST_CHECK_H

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace tcn {
namespace sqe {
namespace test {

struct Case
{
    const char* name;
    void (*fn)();
};

inline std::vector<Case>& registry()
{
    static std::vector<Case> cases;
    return cases;
}

inline int& failures()
{
    static int n = 0;
    return n;
}

inline int& assertions()
{
    static int n = 0;
    return n;
}

struct Registrar
{
    Registrar(const char* name, void (*fn)()) { registry().push_back(Case{name, fn}); }
};

inline void fail(const char* file, int line, const char* what)
{
    ++failures();
    std::printf("    FAIL %s:%d: %s\n", file, line, what);
}

inline void fail2(const char* file, int line, const char* what,
                  const std::string& actual, const std::string& expected)
{
    ++failures();
    std::printf("    FAIL %s:%d: %s\n         actual:   \"%s\"\n         expected: \"%s\"\n",
                file, line, what, actual.c_str(), expected.c_str());
}

inline void fail_int(const char* file, int line, const char* what, long long actual, long long expected)
{
    ++failures();
    std::printf("    FAIL %s:%d: %s (actual %lld, expected %lld)\n", file, line, what, actual, expected);
}

}  // namespace test
}  // namespace sqe
}  // namespace tcn

#define TCN_SQE_TEST(name)                                                          \
    static void name();                                                             \
    static ::tcn::sqe::test::Registrar name##_registrar_(#name, &name);             \
    static void name()

#define CHECK(expr)                                                                 \
    do {                                                                            \
        ++::tcn::sqe::test::assertions();                                           \
        if (!(expr)) { ::tcn::sqe::test::fail(__FILE__, __LINE__, #expr); }         \
    } while (false)

#define CHECK_MSG(expr, msg)                                                        \
    do {                                                                            \
        ++::tcn::sqe::test::assertions();                                           \
        if (!(expr)) { ::tcn::sqe::test::fail(__FILE__, __LINE__, msg); }           \
    } while (false)

#define CHECK_STR_EQ(actual, expected)                                              \
    do {                                                                            \
        ++::tcn::sqe::test::assertions();                                           \
        const std::string a_((actual));                                             \
        const std::string e_((expected));                                           \
        if (a_ != e_) { ::tcn::sqe::test::fail2(__FILE__, __LINE__,                 \
                                                #actual " == " #expected, a_, e_); }\
    } while (false)

#define CHECK_INT_EQ(actual, expected)                                              \
    do {                                                                            \
        ++::tcn::sqe::test::assertions();                                           \
        const long long a_ = static_cast<long long>(actual);                        \
        const long long e_ = static_cast<long long>(expected);                      \
        if (a_ != e_) { ::tcn::sqe::test::fail_int(__FILE__, __LINE__,              \
                                                   #actual " == " #expected, a_, e_); } \
    } while (false)

/// Asserts that a `Result` failed, and failed with exactly this `Error`.
#define CHECK_ERR(result, expected_error)                                           \
    do {                                                                            \
        ++::tcn::sqe::test::assertions();                                           \
        const auto& r_ = (result);                                                  \
        if (r_.is_ok())                                                             \
        {                                                                           \
            ::tcn::sqe::test::fail(__FILE__, __LINE__, #result " unexpectedly ok"); \
        }                                                                           \
        else if (r_.error() != (expected_error))                                    \
        {                                                                           \
            ::tcn::sqe::test::fail2(__FILE__, __LINE__, #result " error",           \
                                    ::tcn::sqe::describe(r_.error()),               \
                                    ::tcn::sqe::describe(expected_error));          \
        }                                                                           \
    } while (false)

/// Asserts that a `Result` succeeded, naming the error if it did not.
#define CHECK_OK(result)                                                            \
    do {                                                                            \
        ++::tcn::sqe::test::assertions();                                           \
        const auto& r_ = (result);                                                  \
        if (!r_.is_ok())                                                            \
        {                                                                           \
            ::tcn::sqe::test::fail(__FILE__, __LINE__,                              \
                                   ::tcn::sqe::describe(r_.error()));               \
        }                                                                           \
    } while (false)

#endif  // TCN_SQE_TEST_CHECK_H
