// SPDX-License-Identifier: internal
//
// A request type nobody registered has no key and no wire name. It must fail at
// the call site rather than be published to whichever key an overload picked.
#include <cstdint>
#include <vector>

#include "tcn/sqe/session.h"

using namespace tcn::sqe;

struct Codec
{
    template <class T> static Result<void> encode(const T&, std::vector<std::uint8_t>&)
    { return Result<void>::ok(); }
    template <class T> static Result<void> decode(BytesView, T&) { return Result<void>::ok(); }
};

struct NotARegisteredRequest { int x = 0; };

void f(SisSession<Codec>& s, const NotARegisteredRequest& req)
{
    (void)s.publish_request(req);   // must not compile
}
