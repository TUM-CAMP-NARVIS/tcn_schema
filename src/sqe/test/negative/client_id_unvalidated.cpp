// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. A ClientId that was never validated is the B39 failure.
#include <string>

#include "tcn/sqe/client_id.h"

int main()
{
    tcn::sqe::ClientId c{std::string("tcn/loc/pcpd/cam1")};
    return static_cast<int>(c.join_key().size());
}
