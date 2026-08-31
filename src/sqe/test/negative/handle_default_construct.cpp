// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. There is no empty handle waiting to be filled in.
#include "tcn/sqe/handle.h"

int main()
{
    tcn::sqe::Handle h;
    return static_cast<int>(h.data_topic().size());
}
