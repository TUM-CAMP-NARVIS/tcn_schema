// SPDX-License-Identifier: internal
//
// A start function that returns only a handle must not compile.
//
// `{handle}/status` is the only channel on which a client learns that a stream
// it holds went `NoPath` or that the engine stopped answering, and it is the
// only teardown event guaranteed to arrive at all. While `StartFn` returned a
// bare `Handle`, declaring that subscriber was a convention, and an integrator
// who skipped it got no error, ever. Returning a `StreamClaim` is what turns
// the convention into an obligation the compiler states.
//
// A `Declaration()` in the claim is still permitted: the type asks, it does not
// compel. Not answering the question at all is what this rejects.
#include "tcn/sqe/lease.h"

using namespace tcn::sqe;

void f(RelationLeaseTable::StartFn& out, const Handle& h)
{
    out = [h](const RelationStreamRequest&) -> Result<Handle> {   // must not compile
        return Result<Handle>::ok(h);
    };
}
