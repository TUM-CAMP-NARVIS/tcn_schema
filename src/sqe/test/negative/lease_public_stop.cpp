// SPDX-License-Identifier: internal
//
// There is no `stop()` on a lease, and its absence is the design.
//
// The engine tracks requesters as a set of client ids, so a stop detaches the
// whole process -- every other component's stream included, with no error
// anywhere. The only way to give up a claim is to destroy the object holding
// it; the escape hatch lives on the table and is spelled
// `force_detach_process_from` so nobody arrives at it by accident.
#include "tcn/sqe/lease.h"

using namespace tcn::sqe;

void f(RelationLease& lease)
{
    lease.stop();   // must not compile
}
