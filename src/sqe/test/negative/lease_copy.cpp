// SPDX-License-Identifier: internal
//
// A lease is a claim, and a claim cannot be duplicated: a copy would add a
// holder the table never counted, and its destruction would decrement a count
// it never incremented.
#include "tcn/sqe/lease.h"

using namespace tcn::sqe;

void f(const RelationLease& src)
{
    RelationLease copy(src);   // must not compile
    (void)copy;
}
