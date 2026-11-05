// SPDX-License-Identifier: internal
//
// A join is a query since B73, and the engine ignores a `put` on its key. A
// client that published it would never join and would be told so by nothing but
// an engine log line -- every stream it started afterwards answered NO_PATH or
// UNKNOWN_FRAME for frames it believes it declared.
#include <cstdint>
#include <vector>

#include "tcn/sqe/session.h"

namespace tcnart_msgs { namespace rpc { class SISJoinRequest { public: int x = 0; }; } }

using namespace tcn::sqe;

struct Codec
{
    template <class T> static Result<void> encode(const T&, std::vector<std::uint8_t>&)
    { return Result<void>::ok(); }
    template <class T> static Result<void> decode(BytesView, T&) { return Result<void>::ok(); }
};

void f(SisSession<Codec>& s, const ::tcnart_msgs::rpc::SISJoinRequest& req)
{
    (void)s.publish_request(req);   // must not compile: this request is a query since B73
}
