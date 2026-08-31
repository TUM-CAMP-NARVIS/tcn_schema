// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. A relation-stream handle is issued by the engine and read
// out of a reply; a topic a client chose stops nothing.
#include <string>

#include "tcn/sqe/handle.h"

int main()
{
    tcn::sqe::Handle h{std::string("sqe/lab1/rel/0123456789abcdef")};
    return static_cast<int>(h.data_topic().size());
}
