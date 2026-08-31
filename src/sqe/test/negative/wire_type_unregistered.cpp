// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. An unregistered type has no specialisation, so a name the
// engine refuses fails at the call site instead of at the daemon.
#include <string>

#include "tcn/sqe/wire_type.h"

namespace tcnart_msgs {
namespace rpc {
class SISJoinMessage;   // 0.3.0 deleted this; the engine refuses it
}
}

int main()
{
    const std::string e = tcn::sqe::cdr_encoding< ::tcnart_msgs::rpc::SISJoinMessage>();
    return static_cast<int>(e.size());
}
