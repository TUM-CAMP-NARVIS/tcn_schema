// SPDX-License-Identifier: internal
//
// MUST NOT COMPILE. A FrameRef is made by parse() or a named constructor, which
// validate; a string is not a frame reference.
#include "tcn/sqe/frame_ref.h"

int main()
{
    tcn::sqe::FrameRef f{"local:a/b"};
    return static_cast<int>(f.name().size());
}
