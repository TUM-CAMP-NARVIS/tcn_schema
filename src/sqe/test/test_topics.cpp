// SPDX-License-Identifier: internal
//
// The engine-issued topic shapes.
#include <string>

#include "check.h"
#include "tcn/sqe/topics.h"

using namespace tcn::sqe;

TCN_SQE_TEST(an_engine_issued_relation_topic_has_the_documented_shape)
{
    CHECK_STR_EQ(engine_relation_topic("lab1", "h42"), "sqe/lab1/rel/h42");
    CHECK_STR_EQ(engine_relation_topic("engine-0", "0123456789abcdef"),
                 "sqe/engine-0/rel/0123456789abcdef");
}

TCN_SQE_TEST(a_status_topic_is_the_output_topic_plus_slash_status)
{
    CHECK_STR_EQ(status_topic_for("sqe/lab1/rel/h42"), "sqe/lab1/rel/h42/status");
    CHECK_STR_EQ(status_topic_for("hmd/rel/marker_7"), "hmd/rel/marker_7/status");
    CHECK_STR_EQ(engine_relation_status_topic("lab1", "h42"), "sqe/lab1/rel/h42/status");
}

// The status topic is derived from the data topic in exactly one place, so
// the two can never disagree about the shape.
TCN_SQE_TEST(the_status_topic_is_derived_from_the_relation_topic_not_rebuilt)
{
    const std::string data = engine_relation_topic("lab1", "h42");
    CHECK_STR_EQ(engine_relation_status_topic("lab1", "h42"), status_topic_for(data));
}

TCN_SQE_TEST(the_namespace_constants_are_the_engine_s)
{
    CHECK_STR_EQ(std::string(kEngineNamespace), "sqe");
    CHECK_STR_EQ(std::string(kRelationFamily), "rel");
    CHECK_STR_EQ(std::string(kStatusSuffix), "/status");
}
