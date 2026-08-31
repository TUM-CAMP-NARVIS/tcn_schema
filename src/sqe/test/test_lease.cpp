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
// what makes the arithmetic testable with no daemon and no network — the
// `FakeTransport` below is in-memory too, and exists only so that the
// `{handle}/status` subscriber a `StartFn` now returns is a real `Declaration`
// over a real `Registration`, whose declare and undeclare can be observed.
#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "check.h"
#include "fake_transport.h"
#include "stub_msgs.h"
#include "tcn/sqe/lease.h"
#include "tcn/sqe/session.h"   // for declare_subscription: the only way to make one

using namespace tcn::sqe;
using tcn::sqe::test::DeclaredThing;
using tcn::sqe::test::FakeTransport;
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

    /// When set, every start declares the `{handle}/status` subscriber through
    /// this transport and hands the `Declaration` to the table -- the shape a
    /// real integrator writes. Left null, every start returns a
    /// default-constructed `Declaration`, which is the documented way to say
    /// "no status subscriber here".
    Transport* transport = nullptr;

    /// Every sample the status subscriber received, as `"key|payload"`.
    std::vector<std::string> status_samples;

    /// Run from inside `StopFn`, before it returns -- the one instant at which
    /// the ordering rule ("the subscriber outlives the stop") is observable.
    std::function<void(const Handle&)> during_stop;

    RelationLeaseTable::StartFn start_fn()
    {
        return [this](const RelationStreamRequest& r) -> Result<StreamClaim> {
            starts.push_back(describe(r));
            if (start_error != Error::Ok) { return Result<StreamClaim>::fail(start_error); }
            const std::string topic = handle_override.empty()
                                          ? ("sqe/eng/rel/" + r.observer().node() + "-" + r.target().node())
                                          : handle_override;
            Handle h = handle_for(topic.c_str());

            if (transport == nullptr)
            {
                // The deliberate opt-out: an empty `Declaration`, spelled out
                // where a reviewer can see it.
                return Result<StreamClaim>::ok(StreamClaim{std::move(h), Declaration()});
            }

            Result<Declaration> sub = declare_subscription(
                *transport, h.status_topic(),
                [this](const Sample& s) {
                    status_samples.push_back(std::string(s.key) + "|" +
                                             std::string(reinterpret_cast<const char*>(s.payload.data),
                                                         s.payload.size));
                });
            if (!sub) { return Result<StreamClaim>::fail(sub.error()); }
            return Result<StreamClaim>::ok(StreamClaim{std::move(h), std::move(sub.value())});
        };
    }

    RelationLeaseTable::StopFn stop_fn()
    {
        return [this](const Handle& h) -> Result<Status> {
            stops.push_back(h.data_topic());
            if (during_stop) { during_stop(h); }
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


// ---------------------------------------------------------------------------
// The status subscriber's lifetime is the lease's
// ---------------------------------------------------------------------------
//
// `{handle}/status` is the only channel on which a client learns that a stream
// it holds went `NoPath` or that the engine stopped answering, and it is the
// only teardown event guaranteed to arrive at all. Before `StartFn` returned
// one, an integrator who forgot to declare it got no error, ever. These tests
// are what the type obligation buys.

// Declared by the start, and released when -- and only when -- the record
// retires. Undeclared anywhere else, a subscriber is left alive on a key
// nothing publishes on any more.
TCN_SQE_TEST(the_status_subscriber_is_declared_by_the_start_and_released_when_the_lease_retires)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    {
        Result<RelationLease> a = table.acquire(req("head", "marker"));
        CHECK_OK(a);
        if (!a) { return; }

        CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 1);
        const DeclaredThing* d = t.find_declared("sqe/eng/rel/head-marker/status");
        CHECK(d != nullptr);
        if (d) { CHECK(d->kind == DeclaredThing::Subscriber); }

        CHECK(t.deliver("sqe/eng/rel/head-marker/status", "application/cdr;x", "nopath"));
        CHECK_INT_EQ(e.status_samples.size(), 1);
    }

    CHECK_MSG(t.live_count(DeclaredThing::Subscriber) == 0,
              "the retired lease left its status subscriber declared");
    CHECK_MSG(!t.deliver("sqe/eng/rel/head-marker/status", "application/cdr;x", "late"),
              "a sample reached a subscriber that should have been undeclared");
    CHECK_INT_EQ(e.status_samples.size(), 1);
    CHECK_INT_EQ(t.declared.size(), 1);   // undeclared, never re-declared
}

// A release that is not the last one must leave the subscriber held. Otherwise
// the first component to let go blinds every other component in the process to
// the stream's status -- the same failure the lease count exists to prevent,
// one channel over.
TCN_SQE_TEST(a_non_final_release_leaves_the_status_subscriber_held)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    Result<RelationLease> b = table.acquire(req("head", "marker"));
    CHECK_OK(a); CHECK_OK(b);
    if (!a || !b) { return; }

    // One start, so one subscriber -- never one per component.
    CHECK_INT_EQ(e.starts.size(), 1);
    CHECK_INT_EQ(t.declared.size(), 1);

    a = Result<RelationLease>::fail(Error::Empty);

    CHECK_INT_EQ(e.stops.size(), 0);
    CHECK_MSG(t.live_count(DeclaredThing::Subscriber) == 1,
              "a non-final release undeclared the status subscriber");
    CHECK(t.deliver("sqe/eng/rel/head-marker/status", "application/cdr;x", "still-listening"));
    CHECK_INT_EQ(e.status_samples.size(), 1);

    b = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 0);
}

// The ordering, directly: the subscriber is still live *while* `StopFn` runs,
// so the engine's final status publish -- "retired", or a `NoPath` in the same
// breath -- still reaches it. Undeclared first, that last message goes nowhere,
// and it is the only teardown event a consumer is guaranteed to get.
TCN_SQE_TEST(the_status_subscriber_is_still_held_while_the_stop_runs)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;

    int live_during_stop = -1;
    bool delivered_during_stop = false;
    e.during_stop = [&](const Handle& h) {
        live_during_stop = static_cast<int>(t.live_count(DeclaredThing::Subscriber));
        delivered_during_stop = t.deliver(h.status_topic(), "application/cdr;x", "retired");
    };

    {
        RelationLeaseTable table(e.start_fn(), e.stop_fn());
        Result<RelationLease> a = table.acquire(req("head", "marker"));
        CHECK_OK(a);
        if (!a) { return; }
    }

    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_MSG(live_during_stop == 1,
              "the status subscriber was undeclared before the stop returned");
    CHECK_MSG(delivered_during_stop,
              "the engine's final status publish did not reach the subscriber");
    CHECK_INT_EQ(e.status_samples.size(), 1);
    if (e.status_samples.size() == 1)
    {
        CHECK_STR_EQ(e.status_samples.front(), "sqe/eng/rel/head-marker/status|retired");
    }
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 0);
}

// The same ordering when the table itself is destroyed while it still holds
// streams. A lease outliving its table is a lifetime bug, but the teardown it
// forces must still be the right way round.
TCN_SQE_TEST(a_table_destroyed_holding_a_stream_stops_it_before_dropping_its_subscriber)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;

    int live_during_stop = -1;
    e.during_stop = [&](const Handle&) {
        live_during_stop = static_cast<int>(t.live_count(DeclaredThing::Subscriber));
    };

    Result<RelationLease> a = Result<RelationLease>::fail(Error::Empty);
    {
        RelationLeaseTable table(e.start_fn(), e.stop_fn());
        a = table.acquire(req("head", "marker"));
        CHECK_OK(a);
    }
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(live_during_stop, 1);
    CHECK_MSG(t.live_count(DeclaredThing::Subscriber) == 0,
              "the destroyed table left a status subscriber declared");
}

// `force_detach_process_from` retires the record, so it takes the subscriber
// with it -- there is nothing left to hear from on that key.
TCN_SQE_TEST(force_detach_releases_the_status_subscriber_too)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    CHECK_OK(a);
    if (!a) { return; }
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 1);

    CHECK(table.force_detach_process_from(a.value().handle()));
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 0);

    // And the lease's own release afterwards is still a no-op -- it must not
    // undeclare a second time or send a second stop.
    a = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(t.declared.size(), 1);
}

// A start whose claim the table does not keep must not leave its subscriber
// behind. Two spellings of one relation both reach the engine, which resolves
// them to one handle; the second claim's `Declaration` is dropped immediately,
// because the record already holds a subscriber for that key and two on one
// stream is exactly the duplicate this design removes.
TCN_SQE_TEST(a_redundant_start_drops_the_subscriber_it_declared)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;
    e.handle_override = "sqe/eng/rel/one-and-the-same";
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker"));
    Result<RelationLease> b = table.acquire(req("hand", "marker"));
    CHECK_OK(a); CHECK_OK(b);
    if (!a || !b) { return; }

    CHECK_INT_EQ(e.starts.size(), 2);
    CHECK_INT_EQ(t.declared.size(), 2);            // both starts declared one
    CHECK_MSG(t.live_count(DeclaredThing::Subscriber) == 1,
              "the redundant start left a second subscriber on one stream");

    // One subscriber, so one delivery -- not two.
    CHECK(t.deliver("sqe/eng/rel/one-and-the-same/status", "application/cdr;x", "s"));
    CHECK_INT_EQ(e.status_samples.size(), 1);

    a = Result<RelationLease>::fail(Error::Empty);
    b = Result<RelationLease>::fail(Error::Empty);
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(t.live_count(DeclaredThing::Subscriber), 0);
}

// The same, on the path where the redundant start is *refused*: a conflicting
// second acquire must not damage the held lease, and must not leak the
// subscriber its own start declared.
TCN_SQE_TEST(a_conflicting_redundant_start_drops_its_subscriber_and_leaves_the_held_one)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;
    e.handle_override = "sqe/eng/rel/one-and-the-same";
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    Result<RelationLease> a = table.acquire(req("head", "marker", 30.0));
    CHECK_OK(a);
    if (!a) { return; }

    CHECK_ERR(table.acquire(req("hand", "marker", 90.0)), Error::ConflictingLease);

    CHECK_INT_EQ(e.starts.size(), 2);
    CHECK_INT_EQ(t.declared.size(), 2);
    CHECK_MSG(t.live_count(DeclaredThing::Subscriber) == 1,
              "a refused acquire left the subscriber its start declared");
    CHECK_INT_EQ(e.stops.size(), 0);

    // The surviving subscriber is the held lease's, not the refused one's.
    CHECK(t.deliver("sqe/eng/rel/one-and-the-same/status", "application/cdr;x", "s"));
    CHECK_INT_EQ(e.status_samples.size(), 1);
}

// A start that failed leased nothing, so there is nothing to undeclare -- and
// the failure path must not be the one that leaks.
TCN_SQE_TEST(a_start_whose_subscriber_the_transport_refused_leases_nothing)
{
    FakeTransport t;
    Engine e;
    e.transport = &t;
    t.subscriber_fails = true;
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    CHECK_ERR(table.acquire(req("head", "marker")), Error::TransportFailed);
    CHECK_INT_EQ(table.size(), 0);
    CHECK_INT_EQ(t.declared.size(), 0);
    CHECK_INT_EQ(e.stops.size(), 0);
}

// The documented opt-out. An integrator who genuinely wants no status
// subscriber returns a default-constructed `Declaration`; the table stores it,
// retires it and asks nothing further. The type asks -- it does not compel.
TCN_SQE_TEST(a_start_that_returns_no_subscriber_is_allowed_and_retires_normally)
{
    FakeTransport t;
    Engine e;                    // e.transport stays null: the empty Declaration
    RelationLeaseTable table(e.start_fn(), e.stop_fn());

    {
        Result<RelationLease> a = table.acquire(req("head", "marker"));
        CHECK_OK(a);
        if (!a) { return; }
        CHECK_INT_EQ(table.local_count(a.value().handle()), 1);
    }
    CHECK_INT_EQ(e.stops.size(), 1);
    CHECK_INT_EQ(table.size(), 0);
    CHECK_INT_EQ(t.declared.size(), 0);
}
