// SPDX-License-Identifier: internal
//
// The reply status mirrors (#35 R1-R3): ordinals, the unknown-ordinal rule,
// wire names, and the handle predicate.
#include <cstdint>

#include "check.h"
#include "tcn/sqe/status.h"

using namespace tcn::sqe;

// Pinned as compile-time facts, so a renumbering is a build failure here
// rather than a status read as another. `0` is UNKNOWN in every one (E6): a
// zeroed header is never success.
static_assert(static_cast<std::int32_t>(ReplyStatus::Unknown) == 0, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::Ok) == 1, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::Rejected) == 2, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::Unavailable) == 3, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::UnknownClient) == 4, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::UnknownFrame) == 5, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::NotFound) == 6, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::Truncated) == 7, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::Conflict) == 8, "");
static_assert(static_cast<std::int32_t>(ReplyStatus::NotInScope) == 9, "");

static_assert(static_cast<std::int32_t>(Reason::Unknown) == 0, "");
static_assert(static_cast<std::int32_t>(Reason::NoPath) == 6, "");
static_assert(static_cast<std::int32_t>(Reason::ClockDomainMismatch) == 7, "");
static_assert(static_cast<std::int32_t>(Reason::StoreUnavailable) == 9, "");

static_assert(static_cast<std::int32_t>(RelationState::Unknown) == 0, "");
static_assert(static_cast<std::int32_t>(RelationState::Active) == 1, "");
static_assert(static_cast<std::int32_t>(RelationState::Pending) == 2, "");
static_assert(static_cast<std::int32_t>(RelationState::Inactive) == 3, "");

static_assert(static_cast<std::int32_t>(ParkReason::Unknown) == 0, "");
static_assert(static_cast<std::int32_t>(ParkReason::Withdrawn) == 5, "");
static_assert(static_cast<std::int32_t>(ParkReason::ClockDomainMismatch) == 6, "");

static_assert(carries_handle(RelationState::Active), "");
static_assert(carries_handle(RelationState::Pending), "PENDING is accepted: there is a stream for you");
static_assert(!carries_handle(RelationState::Inactive), "");
static_assert(!carries_handle(RelationState::Unknown), "");

static_assert(from_ordinal<ReplyStatus>(1) == ReplyStatus::Ok, "usable in a constant expression");

TCN_SQE_TEST(every_known_ordinal_round_trips)
{
    for (std::int32_t i = 0; i <= 9; ++i)
    {
        CHECK_INT_EQ(static_cast<std::int32_t>(from_ordinal<ReplyStatus>(i)), i);
        CHECK_INT_EQ(static_cast<std::int32_t>(from_ordinal<Reason>(i)), i);
    }
    for (std::int32_t i = 0; i <= 3; ++i)
    {
        CHECK_INT_EQ(static_cast<std::int32_t>(from_ordinal<RelationState>(i)), i);
    }
    for (std::int32_t i = 0; i <= 6; ++i)
    {
        CHECK_INT_EQ(static_cast<std::int32_t>(from_ordinal<ParkReason>(i)), i);
    }
}

// A newer engine may append. An ordinal this library does not know reads as
// UNKNOWN -- never cast into the enum, and never an error a client must
// handle separately: UNKNOWN is already "not OK" (E6).
TCN_SQE_TEST(an_unknown_ordinal_reads_as_unknown)
{
    CHECK(from_ordinal<ReplyStatus>(10) == ReplyStatus::Unknown);
    CHECK(from_ordinal<ReplyStatus>(99) == ReplyStatus::Unknown);
    CHECK(from_ordinal<ReplyStatus>(-1) == ReplyStatus::Unknown);
    CHECK(from_ordinal<Reason>(10) == Reason::Unknown);
    CHECK(from_ordinal<RelationState>(4) == RelationState::Unknown);
    CHECK(from_ordinal<RelationState>(-7) == RelationState::Unknown);
    CHECK(from_ordinal<ParkReason>(7) == ParkReason::Unknown);
    CHECK(from_ordinal<ParkReason>(1 << 20) == ParkReason::Unknown);
}

TCN_SQE_TEST(wire_names_are_the_idl_spellings)
{
    CHECK_STR_EQ(wire_name(ReplyStatus::Unknown),       "SIS_STATUS_UNKNOWN");
    CHECK_STR_EQ(wire_name(ReplyStatus::Ok),            "SIS_STATUS_OK");
    CHECK_STR_EQ(wire_name(ReplyStatus::Rejected),      "SIS_STATUS_REJECTED");
    CHECK_STR_EQ(wire_name(ReplyStatus::Unavailable),   "SIS_STATUS_UNAVAILABLE");
    CHECK_STR_EQ(wire_name(ReplyStatus::UnknownClient), "SIS_STATUS_UNKNOWN_CLIENT");
    CHECK_STR_EQ(wire_name(ReplyStatus::UnknownFrame),  "SIS_STATUS_UNKNOWN_FRAME");
    CHECK_STR_EQ(wire_name(ReplyStatus::NotFound),      "SIS_STATUS_NOT_FOUND");
    CHECK_STR_EQ(wire_name(ReplyStatus::Truncated),     "SIS_STATUS_TRUNCATED");
    CHECK_STR_EQ(wire_name(ReplyStatus::Conflict),      "SIS_STATUS_CONFLICT");
    CHECK_STR_EQ(wire_name(ReplyStatus::NotInScope),    "SIS_STATUS_NOT_IN_SCOPE");

    CHECK_STR_EQ(wire_name(Reason::Unknown),             "SIS_REASON_UNKNOWN");
    CHECK_STR_EQ(wire_name(Reason::Malformed),           "SIS_REASON_MALFORMED");
    CHECK_STR_EQ(wire_name(Reason::SchemaMismatch),      "SIS_REASON_SCHEMA_MISMATCH");
    CHECK_STR_EQ(wire_name(Reason::Unsupported),         "SIS_REASON_UNSUPPORTED");
    CHECK_STR_EQ(wire_name(Reason::TermsConflict),       "SIS_REASON_TERMS_CONFLICT");
    CHECK_STR_EQ(wire_name(Reason::NotPermitted),        "SIS_REASON_NOT_PERMITTED");
    CHECK_STR_EQ(wire_name(Reason::NoPath),              "SIS_REASON_NO_PATH");
    CHECK_STR_EQ(wire_name(Reason::ClockDomainMismatch), "SIS_REASON_CLOCK_DOMAIN_MISMATCH");
    CHECK_STR_EQ(wire_name(Reason::LimitExceeded),       "SIS_REASON_LIMIT_EXCEEDED");
    CHECK_STR_EQ(wire_name(Reason::StoreUnavailable),    "SIS_REASON_STORE_UNAVAILABLE");

    CHECK_STR_EQ(wire_name(RelationState::Unknown),  "SIS_RELATION_STATE_UNKNOWN");
    CHECK_STR_EQ(wire_name(RelationState::Active),   "SIS_RELATION_STATE_ACTIVE");
    CHECK_STR_EQ(wire_name(RelationState::Pending),  "SIS_RELATION_STATE_PENDING");
    CHECK_STR_EQ(wire_name(RelationState::Inactive), "SIS_RELATION_STATE_INACTIVE");

    CHECK_STR_EQ(wire_name(ParkReason::Unknown),             "SIS_PARK_REASON_UNKNOWN");
    CHECK_STR_EQ(wire_name(ParkReason::Uncalibrated),        "SIS_PARK_REASON_UNCALIBRATED");
    CHECK_STR_EQ(wire_name(ParkReason::AwaitingDescriptor),  "SIS_PARK_REASON_AWAITING_DESCRIPTOR");
    CHECK_STR_EQ(wire_name(ParkReason::InputStarved),        "SIS_PARK_REASON_INPUT_STARVED");
    CHECK_STR_EQ(wire_name(ParkReason::Resolving),           "SIS_PARK_REASON_RESOLVING");
    CHECK_STR_EQ(wire_name(ParkReason::Withdrawn),           "SIS_PARK_REASON_WITHDRAWN");
    CHECK_STR_EQ(wire_name(ParkReason::ClockDomainMismatch), "SIS_PARK_REASON_CLOCK_DOMAIN_MISMATCH");
}

// A value outside the enumerators (only reachable by a cast) still names
// itself as UNKNOWN rather than returning null.
TCN_SQE_TEST(a_wire_name_is_never_null)
{
    CHECK_STR_EQ(wire_name(static_cast<ReplyStatus>(42)), "SIS_STATUS_UNKNOWN");
    CHECK_STR_EQ(wire_name(static_cast<Reason>(42)), "SIS_REASON_UNKNOWN");
    CHECK_STR_EQ(wire_name(static_cast<RelationState>(42)), "SIS_RELATION_STATE_UNKNOWN");
    CHECK_STR_EQ(wire_name(static_cast<ParkReason>(42)), "SIS_PARK_REASON_UNKNOWN");
}
