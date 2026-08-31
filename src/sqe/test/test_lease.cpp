// SPDX-License-Identifier: internal
//
// B36 -- the local refcount for shared handles.
//
// The engine tracks a stream's requesters as a **set of client ids**, so two
// starts from one process attach it once and one stop detaches it entirely.
// Every case below is a way a process with two components in it silently loses
// a stream, and the table is the only thing standing between them.
//
// Nothing here does I/O. The start and stop are recording functions, which is
// what makes the arithmetic testable with no daemon and no network.
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "check.h"
#include "stub_msgs.h"
#include "tcn/sqe/lease.h"

using namespace tcn::sqe;
using tcn::sqe::test::handle_for;

namespace {

FrameRef frame(const char* client, const char* node)
{
    return FrameRef::parse(client, node).value();
}

StreamTrigger hz(double v) { return StreamTrigger::fixed_rate(v).value(); }

RelationStreamRequest req(const char* observer_node, const char* target_node,
                          double rate = 30.0, double bias = 0.0)
{
    return RelationStreamRequest::make(frame("tcn/loc/a", observer_node),
                                       frame("tcn/loc/b", target_node),
                                       hz(rate), bias)
        .value();
}

/// Records every start and stop, and answers each relation with a stable
/// engine-issued handle -- which is what the engine does: the same relation
/// resolves to the same handle however it was asked for.
class Engine
{
public:
    std::vector<std::string> starts;   ///< "observer->target@rate/bias"
    std::vector<std::string> stops;    ///< the handle topic
    Error start_error = Error::Ok;

    /// Every relation the fixture knows, mapped to the handle the engine issues
    /// for it. Anything not listed gets a handle derived from the target node.
    std::string handle_override;

    RelationLeaseTable::StartFn start_fn()
    {
        return [this](const RelationStreamRequest& r) -> Result<Handle> {
            starts.push_back(describe(r));
            if (start_error != Error::Ok) { return Result<Handle>::fail(start_error); }
            const std::string topic = handle_override.empty()
                                          ? ("sqe/eng/rel/" + r.observer().node() + "-" + r.target().node())
                                          : handle_override;
            return Result<Handle>::ok(handle_for(topic.c_str()));
        };
    }

    RelationLeaseTable::StopFn stop_fn()
    {
        return [this](const Handle& h) -> Result<Status> {
            stops.push_back(h.data_topic());
            // A stop always answers Inactive, and that means only "you are no
            // longer attached" -- never "the stream stopped".
            return Result<Status>::ok(Status::Inactive);
        };
    }

private:
    static std::string describe(const RelationStreamRequest& r)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "@%g/%g", r.trigger().hz(), r.freshness_bias());
        return r.observer().node() + "->" + r.target().node() + buf;
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// The sharing semantics, directly
// ---------------------------------------------------------------------------

// Two components in one process, same relation: one start, one handle, one
// lease record with a count of two. A second start here would be answered
// `Rejected` if its terms differed, and the caller could not tell which of the
// two requests the engine was refusing.
TCN_SQE_TEST(two_callers_asking_for_the_same_relation_get_one_handle_and_one_start)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    Result<RelationLease> b = table.acquire(req("head", "marker"));
    CHECK_OK(a);
    CHECK_OK(b);
    if (!a || !b) { return; }

    CHECK_INT_EQ(e.starts.size(), 1);
    CHECK(a.value().handle() == b.value().handle());
    CHECK_INT_EQ(table.local_count(a.value().handle()), 2);
    CHECK_INT_EQ(table.size(), 1);
    CHECK_INT_EQ(e.stops.size(), 0);
}

// The first release must not retire the stream. This is the failure the table
// exists for: without it the first component's destructor detaches the process
// and the second component's stream goes dead with no error on either side.
TCN_SQE_TEST(the_first_release_does_not_stop_the_stream)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    Result<RelationLease> b = table.acquire(req("head", "marker"));
    CHECK_OK(a); CHECK_OK(b);
    if (!a || !b) { return; }
    const Handle h = a.value().handle();

    a = Result<RelationLease>::fail(Error::Empty);   // destroys the first lease

    CHECK_INT_EQ(e.stops.size(), 0);
    CHECK_INT_EQ(table.local_count(h), 1);
    CHECK(b.value().handle() == h);
}

// ... and the last one does.
TCN_SQE_TEST(the_last_release_stops_the_stream_exactly_once)
{
    Engine e;
    std::string topic;
    {
        RelationLeaseTable table(e.start_fn(), e.stop_fn());
        Result<RelationLease> a = table.acquire(req("head", "marker"));
        Result<RelationLease> b = table.acquire(req("head", "marker"));
        Result<RelationLease> c = table.acquire(req("head", "marker"));
        CHECK_OK(a); CHECK_OK(b); CHECK_OK(c);
        if (!a || !b || !c) { return; }
        topic = a.value().handle().data_topic();
        CHECK_INT_EQ(table.local_count(a.value().handle()), 3);

        a = Result<RelationLease>::fail(Error::Empty);
        CHECK_INT_EQ(e.stops.size(), 0);
        b = Result<RelationLease>::fail(Error::Empty);
        CHECK_INT_EQ(e.stops.size(), 0);
        c = Result<RelationLease>::fail(Error::Empty);
        CHECK_INT_EQ(e.stops.size(), 1);
        CHECK_INT_EQ(table.size(), 0);
    }
    // The table's own destruction must not send a second stop for a stream that
    // has already been retired.
    CHECK_INT_EQ(e.stops.size(), 1);
    if (e.stops.size() == 1) { CHECK_STR_EQ(e.stops.front(), topic); }
}

// Releasing a handle the table does not hold does nothing. It is reachable:
// `force_detach_process_from` retires a record while leases on it are alive, and
// their destructors run afterwards. A second stop would, at best, be answered
// `Inactive` and, at worst, detach a claim another component took in between.
TCN_SQE_TEST(a_release_from_a_holder_the_table_no_longer_knows_is_a_no_op)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    Result<RelationLease> b = table.acquire(req("head", "marker"));
    CHECK_OK(a); CHECK_OK(b);
    if (!a || !b) { return; }
    const Handle h = a.value().handle();

    CHECK(table.force_detach_process_from(h));
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(table.local_count(h), 0);

    // Both leases are still alive and both will run their destructors.
    a = Result<RelationLease>::fail(Error::Empty);
    b = Result<RelationLease>::fail(Error::Empty);
    CHECK_MSG(e.stops.size() == 1, "a release after the record retired sent another stop");

    // And detaching from a handle this process never held is a no-op too.
    CHECK(!table.force_detach_process_from(handle_for("sqe/eng/rel/never-held")));
    CHECK_INT_EQ(e.stops.size(), 1);
}

// A moved-from lease holds no claim. Otherwise every `return lease;` out of a
// factory would decrement the count it was supposed to hand on.
TCN_SQE_TEST(a_moved_from_lease_releases_nothing)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    CHECK_OK(a);
    if (!a) { return; }
    const Handle h = a.value().handle();

    {
        RelationLease moved(std::move(a.value()));
        CHECK(moved.valid());
        CHECK(!a.value().valid());
        CHECK_INT_EQ(table.local_count(h), 1);
        // `a`'s lease object is destroyed at the end of this test; the moved-to
        // one dies here and takes the single claim with it.
    }
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(table.local_count(h), 0);

    // The moved-from object's own destruction must not send a second stop.
    a = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 1);
}

TCN_SQE_TEST(move_assignment_releases_the_claim_it_overwrites)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> first = table.acquire(req("head", "marker"));
    Result<RelationLease> second = table.acquire(req("head", "other"));
    CHECK_OK(first); CHECK_OK(second);
    if (!first || !second) { return; }
    const Handle h1 = first.value().handle();
    const Handle h2 = second.value().handle();
    CHECK(h1 != h2);
    CHECK_INT_EQ(table.size(), 2);

    first.value() = std::move(second.value());

    CHECK_INT_EQ(e.stops.size(), 1);
    if (e.stops.size() == 1) { CHECK_STR_EQ(e.stops.front(), h1.data_topic()); }
    CHECK_INT_EQ(table.local_count(h1), 0);
    CHECK_INT_EQ(table.local_count(h2), 1);
    CHECK(first.value().handle() == h2);
}

// ---------------------------------------------------------------------------
// Keyed by the handle the engine issued, never by the request
// ---------------------------------------------------------------------------

// Two callers spelling one relation differently converge, because the engine's
// reply -- not the request -- is what the record is keyed by. Keyed by the
// request they would diverge into two records, and the first release would stop
// the stream the other one was still reading.
TCN_SQE_TEST(two_spellings_of_one_relation_converge_on_the_handle_the_engine_issued)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());
    e.handle_override = "sqe/eng/rel/one-and-the-same";

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    Result<RelationLease> b = table.acquire(req("hand", "marker"));   // a different relation...
    CHECK_OK(a); CHECK_OK(b);
    if (!a || !b) { return; }

    // ... that the engine resolved to the same stream.
    CHECK_INT_EQ(e.starts.size(), 2);
    CHECK(a.value().handle() == b.value().handle());
    CHECK_INT_EQ(table.size(), 1);
    CHECK_INT_EQ(table.local_count(a.value().handle()), 2);

    a = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 0);
    b = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 1);
}

// Every spelling that pointed at a retired record is forgotten with it.
//
// Two things ride on this. A later acquire must start a fresh stream rather
// than hand back a handle nothing is attached to any more; and a process that
// starts and stops many streams over a long run must not accumulate one dead
// alias per stream, forever.
TCN_SQE_TEST(retiring_a_record_forgets_every_spelling_that_pointed_at_it)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());
    e.handle_override = "sqe/eng/rel/one-and-the-same";

    {
        Result<RelationLease> a = table.acquire(req("head", "marker"));
        Result<RelationLease> b = table.acquire(req("hand", "marker"));
        CHECK_OK(a); CHECK_OK(b);
        CHECK_INT_EQ(table.size(), 1);
        CHECK_INT_EQ(table.distinct_relations(), 2);
    }
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(table.size(), 0);
    CHECK_MSG(table.distinct_relations() == 0,
              "a retired record left its relation spellings behind");

    Result<RelationLease> again = table.acquire(req("hand", "marker"));
    CHECK_OK(again);
    CHECK_MSG(e.starts.size() == 3, "an acquire after the record retired reused a stale memo entry");
    CHECK_INT_EQ(table.distinct_relations(), 1);
}

// ---------------------------------------------------------------------------
// Conflicting terms are caught here, not on the wire
// ---------------------------------------------------------------------------

// The engine answers a redundant start with different terms `Rejected`, and a
// caller cannot tell from that which of the two requests it is refusing. Caught
// locally, the second caller is told exactly what is wrong and the first
// caller's stream is untouched.
TCN_SQE_TEST(a_second_acquire_on_different_terms_is_refused_locally_and_sends_nothing)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker", 30.0, 0.0));
    CHECK_OK(a);
    if (!a) { return; }

    const Result<RelationLease> faster = table.acquire(req("head", "marker", 90.0, 0.0));
    CHECK_ERR(faster, Error::ConflictingLease);

    const Result<RelationLease> fresher = table.acquire(req("head", "marker", 30.0, 1.0));
    CHECK_ERR(fresher, Error::ConflictingLease);

    CHECK_MSG(e.starts.size() == 1, "a conflicting acquire went to the wire");
    CHECK_INT_EQ(table.local_count(a.value().handle()), 1);

    // The existing lease's terms are readable, so the refusal can name them.
    const RelationStreamRequest* held = table.terms_of(a.value().handle());
    CHECK(held != nullptr);
    if (held) { CHECK(held->trigger().hz() == 30.0); }
}

// A conflict must not damage the lease that is legitimately held.
TCN_SQE_TEST(a_refused_acquire_leaves_the_existing_lease_intact)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker", 30.0));
    CHECK_OK(a);
    if (!a) { return; }
    const Handle h = a.value().handle();

    (void)table.acquire(req("head", "marker", 90.0));
    CHECK_INT_EQ(e.stops.size(), 0);
    CHECK_INT_EQ(table.local_count(h), 1);

    a = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 1);
}

// The same relation on the same terms is *not* a conflict -- that is the whole
// sharing case, and refusing it would be worse than not counting at all.
TCN_SQE_TEST(the_same_terms_are_never_a_conflict)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());
    const Result<RelationLease> a = table.acquire(req("head", "marker", 30.0, 0.25));
    const Result<RelationLease> b = table.acquire(req("head", "marker", 30.0, 0.25));
    CHECK_OK(a);
    CHECK_OK(b);
}

// ---------------------------------------------------------------------------
// Failure paths
// ---------------------------------------------------------------------------

// A start that failed created no claim, so nothing was leased and no stop is
// owed. Recording one would have the next acquire hand out a handle the engine
// never issued.
TCN_SQE_TEST(a_failed_start_leases_nothing)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());
    e.start_error = Error::Timeout;

    const Result<RelationLease> a = table.acquire(req("head", "marker"));
    CHECK_ERR(a, Error::Timeout);
    CHECK_INT_EQ(table.size(), 0);
    CHECK_INT_EQ(e.stops.size(), 0);

    e.start_error = Error::Ok;
    const Result<RelationLease> b = table.acquire(req("head", "marker"));
    CHECK_OK(b);
    CHECK_INT_EQ(table.size(), 1);
}

TCN_SQE_TEST(a_table_with_no_start_function_refuses_rather_than_calling_through_it)
{
    RelationLeaseTable table{RelationLeaseTable::StartFn(), RelationLeaseTable::StopFn()};
    CHECK_ERR(table.acquire(req("head", "marker")), Error::NoTransport);
}

// A lease that outlives its table is a lifetime bug, and it is handled rather
// than left to dereference freed memory in a destructor.
TCN_SQE_TEST(a_lease_that_outlives_its_table_is_detached_not_dangling)
{
    Engine e;
    Result<RelationLease> a = Result<RelationLease>::fail(Error::Empty);
    {
        RelationLeaseTable table(e.start_fn(), e.stop_fn());
        a = table.acquire(req("head", "marker"));
        CHECK_OK(a);
        if (!a) { return; }
        CHECK(a.value().valid());
    }
    CHECK_MSG(!a.value().valid(), "a lease still pointed at a destroyed table");
    CHECK_INT_EQ(e.stops.size(), 1);   // the table stopped what it still held
    a = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 1);
}

TCN_SQE_TEST(a_lease_names_its_own_status_topic)
{
    Engine e;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());
    const Result<RelationLease> a = table.acquire(req("head", "marker"));
    CHECK_OK(a);
    if (!a) { return; }
    CHECK_STR_EQ(a.value().handle().data_topic(), "sqe/eng/rel/head-marker");
    CHECK_STR_EQ(a.value().status_topic(), "sqe/eng/rel/head-marker/status");
}
