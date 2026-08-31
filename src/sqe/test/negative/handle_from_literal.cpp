// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. Same rule, reached through a string literal.
#include "tcn/sqe/handle.h"

int main()
{
    tcn::sqe::Handle h{"sqe/lab1/rel/0123456789abcdef"};
    return static_cast<int>(h.data_topic().size());
}
