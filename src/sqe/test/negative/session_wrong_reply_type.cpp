// SPDX-License-Identifier: internal
//
// A join is answered with SISMutationReply. Asking for a SISRelationStreamReply
// used to compile and then fail at run time as WrongDeclaredType -- reported as
// a protocol error for a mistake that was entirely the caller's. The reply type
// is part of the registration now, so it does not compile.
#include <cstdint>
#include <vector>

#include "tcn/sqe/session.h"

namespace tcnart_msgs { namespace rpc {
class SISJoinRequest { public: int x = 0; };
class SISRelationStreamReply { public: int x = 0; };
} }

using namespace tcn::sqe;

struct Codec
{
    template <class T> static Result<void> encode(const T&, std::vector<std::uint8_t>&)
    { return Result<void>::ok(); }
    template <class T> static Result<void> decode(BytesView, T&) { return Result<void>::ok(); }
};

void f(SisSession<Codec>& s, const ::tcnart_msgs::rpc::SISJoinRequest& req)
{
    ::tcnart_msgs::rpc::SISRelationStreamReply reply;
    (void)s.query_request(req, reply);   // must not compile: the wrong reply type
}
