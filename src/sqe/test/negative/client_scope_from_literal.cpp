// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. A ClientScope is made by parse(), which applies the
// fragment rule; a literal is not a scope.
#include "tcn/sqe/scope.h"

int main()
{
    tcn::sqe::ClientScope s = "tcn";
    return static_cast<int>(s.str().size());
}
