// SPDX-License-Identifier: internal
//
// The tests' only harness: CHECK(cond) records a failure and continues; tcn_test_exit() returns the process status.
#pragma once
#include <cstdio>

inline int g_failures = 0;

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                                 \
        }                                                                                 \
    } while (0)

inline int tcn_test_exit(const char* name)
{
    if (g_failures) {
        std::fprintf(stderr, "%s: %d check(s) failed\n", name, g_failures);
        return 1;
    }
    std::printf("%s: ok\n", name);
    return 0;
}
