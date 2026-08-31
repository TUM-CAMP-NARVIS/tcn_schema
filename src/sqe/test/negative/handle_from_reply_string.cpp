// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. `from_reply` takes a reply, and a string is not one.
#include <string>

#include "tcn/sqe/handle.h"

int main()
{
    const auto h = tcn::sqe::Handle::from_reply(std::string("sqe/lab1/rel/h42"));
    return h.is_ok() ? 0 : 1;
}
