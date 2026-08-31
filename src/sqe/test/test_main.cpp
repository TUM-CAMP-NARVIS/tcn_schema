// SPDX-License-Identifier: internal
#include <cstdio>

#include "check.h"

int main()
{
    using namespace tcn::sqe::test;

    int total = 0;
    int failed_cases = 0;

    for (const Case& c : registry())
    {
        const int before = failures();
        c.fn();
        ++total;
        const bool ok = failures() == before;
        if (!ok) { ++failed_cases; }
        std::printf("%s %s\n", ok ? "[ ok ]" : "[FAIL]", c.name);
    }

    std::printf("\n%d cases, %d assertions, %d failed cases, %d failed assertions\n",
                total, assertions(), failed_cases, failures());
    return failures() == 0 ? 0 : 1;
}
