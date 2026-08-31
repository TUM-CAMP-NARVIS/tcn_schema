// SPDX-License-Identifier: internal
//
// A relation-stream start is answered, and publishing it one-way would throw
// away the handle -- leaving the client attached to a stream it cannot name,
// cannot subscribe to and cannot stop.
#include <cstdint>
#include <vector>

#include "tcn/sqe/session.h"

namespace tcnart_msgs { namespace rpc { class SISRelationStreamStartRequest { public: int x = 0; }; } }

using namespace tcn::sqe;

struct Codec
{
    template <class T> static Result<void> encode(const T&, std::vector<std::uint8_t>&)
    { return Result<void>::ok(); }
    template <class T> static Result<void> decode(BytesView, T&) { return Result<void>::ok(); }
};

void f(SisSession<Codec>& s, const ::tcnart_msgs::rpc::SISRelationStreamStartRequest& req)
{
    (void)s.publish_request(req);   // must not compile: this request is a query
}
