// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. A join is answered with SISJoinReply since 0.11 (#35 R5):
// it carries the engine's capabilities, which a SISMutationReply has no room
// for. Both open with the same SISReplyHeader, so the bytes of one decode as
// the other without complaint; the registration is what tells them apart, and
// asking for the old reply type is a compile error rather than a reply whose
// capabilities silently read as empty.
//
// The types are complete here, so the only thing that can stop this compiling
// is the registration's reply type -- not an incomplete type.
#include <cstdint>
#include <vector>

#include "tcn/sqe/session.h"

namespace sqe_msgs { namespace rpc {
class SISJoinRequest { public: int x = 0; };
class SISMutationReply { public: int x = 0; };
} }

using namespace tcn::sqe;

struct Codec
{
    template <class T> static Result<void> encode(const T&, std::vector<std::uint8_t>&)
    { return Result<void>::ok(); }
    template <class T> static Result<void> decode(BytesView, T&) { return Result<void>::ok(); }
};

void f(SisSession<Codec>& s, const ::sqe_msgs::rpc::SISJoinRequest& req)
{
    ::sqe_msgs::rpc::SISMutationReply reply;
    (void)s.query_request(req, reply);   // must not compile: a join is not answered by a mutation reply
}
