// SPDX-License-Identifier: internal
//
// RelationLeaseTable — the local refcount that makes a shared handle safe.
//
// This is B36, and the engine's own documentation calls it "the single most
// important type in the whole API" on the consumer side and "the most likely
// design error" on this one. The rule it exists for:
//
//   > Requesters are tracked as a **set of client ids**, not a count. Two
//   > starts from the same client id attach that id **once**, and one stop from
//   > it detaches **entirely**.
//
// So two components in one process that ask for the same relation are given the
// same handle by the engine, and the first component's stop stops the second
// component's stream — with no error anywhere, on either side. A wrapper that
// expects its own refcounting to nest is wrong unless it counts locally. This
// counts locally.
//
// It does no I/O. The start and stop are functions the owner supplies, which is
// what lets every rule below be tested with no daemon, no session, and no
// network. The table does hold one thing the transport made — the
// `{handle}/status` subscriber's `Declaration`, produced by the owner's start
// function — but it only stores it, and never calls a `Transport` itself.
#ifndef TCN_SQE_LEASE_H
#define TCN_SQE_LEASE_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "tcn/sqe/error.h"
#include "tcn/sqe/handle.h"
#include "tcn/sqe/relation.h"
#include "tcn/sqe/result.h"
#include "tcn/sqe/status.h"
#include "tcn/sqe/transport.h"

namespace tcn {
namespace sqe {

class RelationLeaseTable;

/// What a start produced: the handle the engine issued, and the
/// `{handle}/status` subscriber declared for it.
///
/// **Why the subscriber is in the return type.** `{handle}/status` is the only
/// channel on which a client learns that a stream it holds went `NoPath`, or
/// that the engine stopped answering, and those two are not distinguishable any
/// other way. It is also the only teardown event guaranteed to arrive: the
/// pipeline stop, the descriptor withdrawal and the derived-edge retraction are
/// each gated on the stream currently holding that thing, so a consumer waiting
/// on a withdrawal instead simply hangs. Documenting "declare it in your start
/// function" asked nothing of anybody: an integrator who forgot got no error,
/// ever. Returning it makes producing one a type obligation.
///
/// **Why the table keeps it rather than the caller.** The subscriber's lifetime
/// is the *lease's* — declared when the stream begins, dropped when nothing
/// publishes on that key any more — and only the table knows when that is.
/// Declared per-component instead, N components sharing one stream leave N
/// subscribers on it, and the released ones go on listening to a dead key.
/// Handing the `Declaration` over is what turns that coupling from documented
/// into enforced: the integrator no longer holds it and so cannot drop it in
/// the wrong place.
///
/// **An empty `status` is allowed, and is a choice.** An integrator who
/// genuinely wants no status subscriber returns a default-constructed
/// `Declaration`. The type asks; it does not compel. What it buys is that the
/// empty appears in the integrator's own code, where a reviewer can see it,
/// instead of being an omission nobody ever notices.
///
/// Move-only, because `Declaration` is.
struct StreamClaim
{
    Handle handle;
    Declaration status;
};

/// One component's claim on a relation stream. Move-only; releasing is what
/// the destructor does.
///
/// **There is no public `stop()`, and that is the point.** A `stop()` here
/// would be the shape every component reaches for, and calling it detaches the
/// whole process — every other component included — because the engine tracks
/// requesters as a set of client ids. The escape hatch exists, on the table,
/// spelled `force_detach_process_from`, so that nobody arrives at it by
/// accident.
class RelationLease
{
public:
    RelationLease(const RelationLease&) = delete;
    RelationLease& operator=(const RelationLease&) = delete;

    RelationLease(RelationLease&& o) noexcept;
    RelationLease& operator=(RelationLease&& o) noexcept;
    ~RelationLease();

    /// False for a moved-from lease, and for one whose table has been
    /// destroyed. A released or moved-from lease decrements nothing.
    bool valid() const noexcept { return table_ != nullptr; }

    /// The engine-issued handle. Precondition: `valid()`.
    const Handle& handle() const noexcept { return handle_; }

    /// `{data_topic}/status` — where this stream's status notifications arrive.
    std::string status_topic() const { return handle_.status_topic(); }

private:
    friend class RelationLeaseTable;
    RelationLease(RelationLeaseTable* t, Handle h) noexcept;

    void detach() noexcept { table_ = nullptr; }

    RelationLeaseTable* table_;
    Handle handle_;
};

/// The process-wide record of which relation streams this process holds, and
/// how many local claims each one has.
///
/// # One table must cover every requester that shares a client id
///
/// **This is an integrator's obligation. The library does not enforce it, on
/// purpose.** Where a process-wide object lives — a singleton, an entry in a
/// service locator, a member of the root application object, a field on a
/// long-lived pipeline — is a question about *your* architecture, and a library
/// that answered it for you would fight whichever answer you had already
/// chosen. So it is documented here rather than imposed.
///
/// The requirement itself is not negotiable. The count has to span every
/// component that requests relations under the same client id, because the
/// thing being counted — that client id's attachment to a stream, engine-side —
/// spans them too. The engine deduplicates by `(observer, target)`: two
/// components asking for the same relation are handed the *same* handle and
/// attach the *same* requester id. It cannot tell them apart, and it is not
/// trying to.
///
/// **What a second table costs.** Each table believes it is the sole holder. The
/// first one to release issues a stop, the engine sees its only requester
/// withdraw, and the stream is retired — out from under the other component,
/// which still believes it holds a live claim. Its subscriber simply stops
/// receiving. There is no error on either side: the stop was well-formed, the
/// engine did exactly what it was asked, and the surviving component was never
/// told. This is the failure `RelationLeaseTable` exists to remove, reappearing
/// one level up.
///
/// **What to do.** Own one table wherever your architecture owns process-wide
/// services, and hand every component a reference to it. Do **not** construct
/// one per component, per pipeline, per connection or per request — that is the
/// two-table case, and it looks correct in review because each site is
/// individually reasonable.
///
/// **How to recognise it in the wild.** Two components request the same
/// relation; one of them stops; the *other* stops receiving poses while the
/// engine reports the stream retired normally. If that happens, count your
/// tables before suspecting the engine — and see `local_count()`, which is
/// exported for exactly this diagnosis.
///
/// One table per *client id* is the precise rule; one per process is the same
/// thing whenever a process joins under a single client id, which is the
/// ordinary case. A process that deliberately joins as two distinct clients has
/// two independent attachment sets engine-side, and wants one table each.
///
/// Not internally synchronized, in keeping with the rest of the library: the
/// owner serializes on its own request thread.
class RelationLeaseTable
{
public:
    /// Sends the start, and returns both of the things a stream begins with:
    /// the handle the engine issued, and the `{handle}/status` subscriber
    /// declared for it.
    ///
    /// It returns a `StreamClaim` and not the generated reply, so that this
    /// header includes no generated type and its tests need no fastddsgen
    /// toolchain. The one line that turns a reply into a `Handle` —
    /// `Handle::from_reply` — already lives in the contract core and enforces
    /// the reply invariant on the way through, so the consumer's start function
    /// is a call to its own query wrapper, that, and a `subscribe`.
    ///
    /// Called exactly when a handle's local count goes 0 -> 1, which is exactly
    /// when the subscriber should be declared. See `StreamClaim` for why the
    /// subscriber is in the return type at all, and why the table keeps it.
    ///
    /// A start that is called but whose claim is not kept — two spellings of
    /// one relation converging on a handle this table already holds — drops the
    /// `Declaration` it was handed, immediately. That is the point: the record
    /// already has a subscriber for that key, and a second one would be the
    /// duplicate this design exists to prevent.
    using StartFn = std::function<Result<StreamClaim>(const RelationStreamRequest&)>;

    /// Sends the stop. Called exactly when a handle's local count reaches 0.
    ///
    /// It does **not** undeclare the status subscriber, and must not try: the
    /// table holds that `Declaration` and drops it itself, *after* this
    /// function returns. See `retire`.
    ///
    /// The engine answers `Inactive`, and that means only *"you are no longer
    /// attached"*. It does **not** mean the stream stopped: another process may
    /// still hold the same handle. This table therefore never reports
    /// "stopped", and the status returned here is passed through unread.
    using StopFn = std::function<Result<Status>(const Handle&)>;

    RelationLeaseTable(StartFn start, StopFn stop)
        : start_(std::move(start)), stop_(std::move(stop)) {}

    RelationLeaseTable(const RelationLeaseTable&) = delete;
    RelationLeaseTable& operator=(const RelationLeaseTable&) = delete;

    /// Detaches every live lease so that none of them dereferences this table
    /// afterwards, then stops every stream this process still holds.
    ///
    /// A lease outliving its table is a lifetime bug the owner should not
    /// write; it is handled anyway, because the alternative is a dangling
    /// pointer in a destructor.
    ///
    /// Every stop goes out before any record is erased, so every status
    /// subscriber is still held while the stops run — the same ordering
    /// `retire` keeps, and for the same reason.
    ~RelationLeaseTable()
    {
        for (RelationLease* l : live_) { l->detach(); }
        live_.clear();
        if (stop_)
        {
            for (const auto& kv : records_) { (void)stop_(kv.second.handle); }
        }
        records_.clear();   // and with them the status subscribers
        memo_.clear();
    }

    /// Claim `req`'s relation, starting the stream if this process does not
    /// already hold it.
    ///
    /// The rules, each with the failure it prevents:
    ///
    ///   * a start goes out only on 0 -> 1 — a redundant start is answered
    ///     `Rejected` if its terms differ, and a `Rejected` a caller cannot
    ///     attribute to either request is worse than no request;
    ///   * the record is keyed by the **handle from the reply**, never by the
    ///     request, so two callers spelling the same relation differently
    ///     converge on one record instead of diverging into two;
    ///   * a second acquire of the same relation on different terms fails here,
    ///     locally, with `Error::ConflictingLease` — before the wire, where the
    ///     same mistake becomes an uninterpretable `Rejected`.
    Result<RelationLease> acquire(const RelationStreamRequest& req)
    {
        if (!start_) { return Result<RelationLease>::fail(Error::NoTransport); }

        const RelationStreamRequest::Relation rel = req.relation();

        // Fast path: this process already asked for this relation. No wire.
        const auto memo = memo_.find(rel);
        if (memo != memo_.end())
        {
            const auto rec = records_.find(memo->second);
            if (rec != records_.end())
            {
                if (!rec->second.terms.same_terms_as(req))
                {
                    return Result<RelationLease>::fail(Error::ConflictingLease);
                }
                ++rec->second.count;
                return Result<RelationLease>::ok(RelationLease(this, rec->second.handle));
            }
            // A stale alias: the record retired without its memo entry being
            // swept. Cannot happen through this class's own paths, and is
            // repaired rather than trusted.
            memo_.erase(memo);
        }

        Result<StreamClaim> started = start_(req);
        if (!started) { return Result<RelationLease>::fail(started.error()); }
        StreamClaim claim = std::move(started.value());
        const Handle& h = claim.handle;

        const auto existing = records_.find(h.data_topic());
        if (existing != records_.end())
        {
            // Two spellings of one relation. The engine resolved them to the
            // same handle, which is the authority; the memo learns the alias.
            //
            // `claim.status` is *not* kept: this record already holds a
            // subscriber for that key, and a second one on one stream is the
            // duplicate the design exists to prevent. It is dropped — and so
            // undeclared — when `claim` goes out of scope below.
            memo_.emplace(rel, h.data_topic());
            if (!existing->second.terms.same_terms_as(req))
            {
                // The start already went out, and it changed nothing: this
                // client id was already in the stream's requester set, because
                // the existing lease put it there. So there is nothing to undo
                // and no stop to send — sending one would detach the lease that
                // is legitimately held.
                return Result<RelationLease>::fail(Error::ConflictingLease);
            }
            ++existing->second.count;
            return Result<RelationLease>::ok(RelationLease(this, existing->second.handle));
        }

        const std::string topic = h.data_topic();
        records_.emplace(topic, Record{claim.handle, req, 1, std::move(claim.status)});
        memo_.emplace(rel, topic);
        return Result<RelationLease>::ok(RelationLease(this, claim.handle));
    }

    /// How many local claims stand on `h`. Zero for a handle this process does
    /// not hold. For diagnostics and for tests.
    std::size_t local_count(const Handle& h) const noexcept
    {
        const auto it = records_.find(h.data_topic());
        return it == records_.end() ? 0u : it->second.count;
    }

    /// How many distinct streams this process holds.
    std::size_t size() const noexcept { return records_.size(); }

    /// How many relation spellings this table has learned a handle for.
    ///
    /// Normally equal to `size()`, and larger when two spellings of one
    /// relation resolved to one handle. It must fall back to zero as records
    /// retire: a process that starts and stops many streams over a long run
    /// would otherwise accumulate one dead alias per stream, forever.
    std::size_t distinct_relations() const noexcept { return memo_.size(); }

    /// The terms the existing lease on `h` was taken on — what to name in the
    /// log line after an `Error::ConflictingLease`.
    const RelationStreamRequest* terms_of(const Handle& h) const noexcept
    {
        const auto it = records_.find(h.data_topic());
        return it == records_.end() ? nullptr : &it->second.terms;
    }

    /// Stop `h` for this whole process, whatever any component still believes.
    ///
    /// **This is the escape hatch, and its name is a warning.** Because the
    /// engine tracks requesters as a set of client ids, one stop detaches the
    /// process entirely — every component that holds a lease on `h` loses its
    /// stream, and none of them is told. Nothing in normal operation calls
    /// this; leases already released their own claims by being destroyed.
    ///
    /// Returns false when this process did not hold `h`. Leases on `h` remain
    /// valid objects and their release afterwards is a no-op.
    bool force_detach_process_from(const Handle& h)
    {
        const auto it = records_.find(h.data_topic());
        if (it == records_.end()) { return false; }
        retire(it);
        return true;
    }

private:
    friend class RelationLease;

    struct Record
    {
        Handle handle;
        RelationStreamRequest terms;
        std::size_t count;

        /// The `{handle}/status` subscriber, owned for exactly as long as this
        /// record exists. The table never touches the transport itself — this
        /// `Declaration` was produced by the integrator's `StartFn` and is only
        /// stored here — which is what keeps the lease table's own logic free
        /// of any transport dependency.
        ///
        /// Move-only, so `Record` is, and so is this table's `records_` map.
        Declaration status;
    };

    void enroll(RelationLease* l) { live_.push_back(l); }

    void rebind(RelationLease* from, RelationLease* to) noexcept
    {
        for (RelationLease*& slot : live_)
        {
            if (slot == from) { slot = to; return; }
        }
    }

    void withdraw(RelationLease* l) noexcept
    {
        for (std::size_t i = 0; i < live_.size(); ++i)
        {
            if (live_[i] == l) { live_.erase(live_.begin() + static_cast<std::ptrdiff_t>(i)); return; }
        }
    }

    /// What `~RelationLease` does. Private: the only way to give up a claim is
    /// to destroy the object that holds it.
    ///
    /// Releasing a handle this table does not hold is a no-op, deliberately.
    /// It is reachable — `force_detach_process_from` retires a record while
    /// leases on it are still alive — and a second stop for a stream this
    /// process is no longer attached to would, at best, be answered `Inactive`
    /// and, at worst, detach a claim some other component took in between.
    void release_one(const Handle& h)
    {
        const auto it = records_.find(h.data_topic());
        if (it == records_.end()) { return; }
        if (it->second.count > 1) { --it->second.count; return; }
        retire(it);
    }

    /// Drops the record, sends the stop, and *then* drops the status
    /// subscriber.
    ///
    /// **That order is the point, and it is easy to get backwards.** The
    /// subscriber is moved out of the record and held in a local across the
    /// stop, so it is still live while `StopFn` runs and for the reply that
    /// ends it. A final publish on `{handle}/status` — the engine reporting the
    /// stream retired, or reporting `NoPath` in the same breath — therefore
    /// still reaches it. Undeclare first and that last message goes nowhere,
    /// which is precisely the event a consumer waiting on a teardown is waiting
    /// for: the status publish is the only teardown event guaranteed to arrive,
    /// because the pipeline stop, the descriptor withdrawal and the derived-edge
    /// retraction are each gated on the stream still holding that thing.
    ///
    /// The record is erased *before* the stop, and that order matters too: a
    /// `StopFn` that re-enters this table must see a count of zero, not a
    /// record that is about to vanish.
    void retire(std::map<std::string, Record>::iterator it)
    {
        const Handle h = it->second.handle;
        Declaration status = std::move(it->second.status);   // outlives the record
        records_.erase(it);
        for (auto m = memo_.begin(); m != memo_.end();)
        {
            if (m->second == h.data_topic()) { m = memo_.erase(m); } else { ++m; }
        }
        if (stop_) { (void)stop_(h); }
        // `status` is undeclared here, by leaving scope: after the stop, never
        // before it.
    }

    StartFn start_;
    StopFn stop_;
    std::map<std::string, Record> records_;
    std::map<RelationStreamRequest::Relation, std::string> memo_;
    std::vector<RelationLease*> live_;
};

// --- RelationLease, out of line because it needs the table's definition -----

inline RelationLease::RelationLease(RelationLeaseTable* t, Handle h) noexcept
    : table_(t), handle_(std::move(h))
{
    if (table_) { table_->enroll(this); }
}

inline RelationLease::RelationLease(RelationLease&& o) noexcept
    : table_(o.table_), handle_(std::move(o.handle_))
{
    if (table_) { table_->rebind(&o, this); }
    o.table_ = nullptr;
}

inline RelationLease& RelationLease::operator=(RelationLease&& o) noexcept
{
    if (this != &o)
    {
        if (table_) { table_->withdraw(this); table_->release_one(handle_); }
        table_ = o.table_;
        handle_ = std::move(o.handle_);
        if (table_) { table_->rebind(&o, this); }
        o.table_ = nullptr;
    }
    return *this;
}

inline RelationLease::~RelationLease()
{
    if (table_)
    {
        table_->withdraw(this);
        table_->release_one(handle_);
    }
}

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_LEASE_H
