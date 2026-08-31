// SPDX-License-Identifier: internal
//
// Status ordinals and semantics.
#include <cstdint>

#include "check.h"
#include "tcn/sqe/status.h"

using namespace tcn::sqe;

// Pinned as compile-time facts, so a renumbering is a build failure here
// rather than a stream that never produces anything.
static_assert(static_cast<std::int32_t>(Status::Active) == 0, "");
static_assert(static_cast<std::int32_t>(Status::Inactive) == 1, "");
static_assert(static_cast<std::int32_t>(Status::NoPath) == 2, "");
static_assert(static_cast<std::int32_t>(Status::UnknownFrame) == 3, "");
static_assert(static_cast<std::int32_t>(Status::ClockDomainMismatch) == 4, "");
static_assert(static_cast<std::int32_t>(Status::Rejected) == 5, "");
static_assert(static_cast<std::int32_t>(Status::Pending) == 6, "");

static_assert(is_acceptance(Status::Pending), "Pending is acceptance, not failure");
static_assert(is_acceptance(Status::Active), "");
static_assert(!is_acceptance(Status::Inactive), "");

TCN_SQE_TEST(pending_is_acceptance_and_not_a_reason_to_retry)
{
    // The retry loop against Pending is the precise failure the status split
    // was introduced to end: the stream is registered and resumes on its own.
    CHECK(is_acceptance(Status::Pending));
    CHECK(!is_final_for_this_graph(Status::Pending));
    CHECK(carries_handle(Status::Pending));
}

TCN_SQE_TEST(acceptance_is_exactly_active_and_pending)
{
    CHECK(is_acceptance(Status::Active));
    CHECK(is_acceptance(Status::Pending));
    CHECK(!is_acceptance(Status::Inactive));
    CHECK(!is_acceptance(Status::NoPath));
    CHECK(!is_acceptance(Status::UnknownFrame));
    CHECK(!is_acceptance(Status::ClockDomainMismatch));
    CHECK(!is_acceptance(Status::Rejected));
}

TCN_SQE_TEST(carrying_a_handle_is_exactly_acceptance)
{
    for (std::int32_t i = 0; i <= kMaxStatusOrdinal; ++i)
    {
        const Status s = static_cast<Status>(i);
        CHECK_MSG(carries_handle(s) == is_acceptance(s),
                  "the reply invariant must track acceptance exactly");
    }
}

TCN_SQE_TEST(finality_is_exactly_no_path_unknown_frame_and_rejected)
{
    CHECK(is_final_for_this_graph(Status::NoPath));
    CHECK(is_final_for_this_graph(Status::UnknownFrame));
    CHECK(is_final_for_this_graph(Status::Rejected));
    CHECK(!is_final_for_this_graph(Status::Active));
    CHECK(!is_final_for_this_graph(Status::Inactive));
    CHECK(!is_final_for_this_graph(Status::Pending));
    CHECK(!is_final_for_this_graph(Status::ClockDomainMismatch));
}

TCN_SQE_TEST(an_unknown_ordinal_is_refused_rather_than_cast)
{
    for (std::int32_t i = 0; i <= kMaxStatusOrdinal; ++i)
    {
        CHECK_OK(status_from_ordinal(i));
        CHECK_INT_EQ(static_cast<std::int32_t>(status_from_ordinal(i).value()), i);
    }
    CHECK(!status_from_ordinal(7).is_ok());
    CHECK(!status_from_ordinal(-1).is_ok());
    CHECK(!status_from_ordinal(1 << 20).is_ok());
}

TCN_SQE_TEST(status_names_are_the_wire_spellings)
{
    CHECK_STR_EQ(status_name(Status::Active),              "SIS_RS_ACTIVE");
    CHECK_STR_EQ(status_name(Status::Inactive),            "SIS_RS_INACTIVE");
    CHECK_STR_EQ(status_name(Status::NoPath),              "SIS_RS_NO_PATH");
    CHECK_STR_EQ(status_name(Status::UnknownFrame),        "SIS_RS_UNKNOWN_FRAME");
    CHECK_STR_EQ(status_name(Status::ClockDomainMismatch), "SIS_RS_CLOCK_DOMAIN_MISMATCH");
    CHECK_STR_EQ(status_name(Status::Rejected),            "SIS_RS_REJECTED");
    CHECK_STR_EQ(status_name(Status::Pending),             "SIS_RS_PENDING");
}
