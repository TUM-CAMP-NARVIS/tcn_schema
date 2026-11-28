// SPDX-License-Identifier: internal
//
// The reply header every reply and notification opens with (#35 R1-R3).
#include <cstdint>
#include <string>

#include "check.h"
#include "stub_msgs.h"
#include "tcn/sqe/reply.h"

using namespace tcn::sqe;

namespace rpc = ::sqe_msgs::rpc;

// The review focus: every `sqe_msgs` enum shifted, and 0 is UNKNOWN. A zeroed
// or truncated reply must never read as success.
TCN_SQE_TEST(a_zeroed_reply_is_not_ok)
{
    const rpc::SISMutationReply zeroed;
    const ReplyHeader h = reply_header(zeroed);
    CHECK(h.status == ReplyStatus::Unknown);
    CHECK(h.reason == Reason::Unknown);
    CHECK(!h.ok());
    CHECK(h.detail.empty());
    CHECK(h.engine_id.empty());
    CHECK(h.engine_epoch == 0u);
}

TCN_SQE_TEST(ordinal_one_is_ok)
{
    rpc::SISMutationReply r;
    r.header_.status_ = static_cast<rpc::SISStatus>(1);
    const ReplyHeader h = reply_header(r);
    CHECK(h.status == ReplyStatus::Ok);
    CHECK(h.ok());
}

// A status a newer engine appended reads as UNKNOWN, which is not OK.
TCN_SQE_TEST(an_unknown_status_ordinal_reads_as_unknown_and_not_ok)
{
    rpc::SISMutationReply r;
    r.header_.status_ = static_cast<rpc::SISStatus>(99);
    const ReplyHeader h = reply_header(r);
    CHECK(h.status == ReplyStatus::Unknown);
    CHECK(!h.ok());
}

TCN_SQE_TEST(the_reason_and_the_engine_identity_are_copied)
{
    rpc::SISRelationStreamReply r;
    r.header_.status_ = rpc::SIS_STATUS_REJECTED;
    r.header_.reason_ = static_cast<rpc::SISReason>(7);
    r.header_.detail_ = "route crosses lab1/clock and lab2/clock";
    r.header_.engine_id_ = "lab1";
    r.header_.engine_epoch_ = 1762300800123ull;

    const ReplyHeader h = reply_header(r);
    CHECK(h.status == ReplyStatus::Rejected);
    CHECK(h.reason == Reason::ClockDomainMismatch);
    CHECK(!h.ok());
    CHECK_STR_EQ(h.detail, "route crosses lab1/clock and lab2/clock");
    CHECK_STR_EQ(h.engine_id, "lab1");
    CHECK(h.engine_epoch == 1762300800123ull);
}

// Only OK is OK: TRUNCATED was carried out but is not the whole answer, and a
// caller that wants to accept it says so.
TCN_SQE_TEST(only_ok_is_ok)
{
    for (std::int32_t i = 0; i <= 9; ++i)
    {
        rpc::SISJoinReply r;
        r.header_.status_ = static_cast<rpc::SISStatus>(i);
        CHECK(reply_header(r).ok() == (i == 1));
    }
}
