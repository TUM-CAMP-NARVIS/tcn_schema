// SPDX-License-Identifier: internal
//
// An in-memory `Transport`, and the worked example of what an adapter has to
// do.
//
// This is the whole reason `Transport` is an abstract class rather than a
// template parameter: substituting this for a zenoh session makes the protocol
// testable with no daemon, no network and no zenoh-cpp at all. It records what
// was sent, replays scripted replies, and can be told to fail any operation.
//
// It is also a size check on the interface. If an adapter is much longer than
// this, the interface has grown something it should not have.
//
// It backs two things: the session tests, and the lease table's status
// subscriber. `RelationLeaseTable::StartFn` returns a `StreamClaim` — a handle
// *and* the `{handle}/status` subscriber's `Declaration` — so the worked
// example of a start function now runs against this fake: see the `Engine`
// fixture at the top of test_lease.cpp, which declares that subscriber through
// a `Transport&` and hands the `Declaration` to the table. `live_count` and
// `deliver` below are what make the resulting lifetime observable.
#ifndef TCN_SQE_TEST_FAKE_TRANSPORT_H
#define TCN_SQE_TEST_FAKE_TRANSPORT_H

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "tcn/sqe/transport.h"

namespace tcn {
namespace sqe {
namespace test {

struct SentMessage
{
    std::string key;
    std::string encoding;
    std::vector<std::uint8_t> payload;
    std::uint32_t timeout_ms = 0;
};

struct DeclaredThing
{
    enum Kind { Token, Subscriber } kind;
    std::string key;
    bool live;
    SampleFn on_sample;
};

class FakeTransport final : public Transport
{
public:
    // -- what the library did ------------------------------------------------
    std::vector<SentMessage> published;
    std::vector<SentMessage> queried;
    std::map<std::uint64_t, DeclaredThing> declared;

    // -- what the fake does back ---------------------------------------------
    std::deque<Result<void>> scripted_query_outcomes;   ///< front is used first
    std::deque<QueryReply> scripted_replies;
    Error publish_error = Error::Ok;                    ///< Ok = succeed
    bool token_fails = false;
    bool subscriber_fails = false;

    Result<void> publish(std::string_view key, std::string_view encoding, BytesView payload) override
    {
        published.push_back(SentMessage{std::string(key), std::string(encoding), copy(payload), 0});
        if (publish_error != Error::Ok) { return Result<void>::fail(publish_error); }
        return Result<void>::ok();
    }

    Result<void> query(std::string_view key, std::string_view encoding, BytesView payload,
                       std::uint32_t timeout_ms, QueryReply& out) override
    {
        queried.push_back(SentMessage{std::string(key), std::string(encoding), copy(payload), timeout_ms});

        if (!scripted_query_outcomes.empty())
        {
            const Result<void> outcome = scripted_query_outcomes.front();
            scripted_query_outcomes.pop_front();
            if (!outcome) { return outcome; }
        }
        if (scripted_replies.empty()) { return Result<void>::fail(Error::Timeout); }
        out = scripted_replies.front();
        scripted_replies.pop_front();
        return Result<void>::ok();
    }

    Result<Registration> declare_liveliness_token(std::string_view key) override
    {
        if (token_fails) { return Result<Registration>::fail(Error::TransportFailed); }
        return Result<Registration>::ok(add(DeclaredThing::Token, key, SampleFn()));
    }

    Result<Registration> declare_subscriber(std::string_view key, SampleFn on_sample) override
    {
        if (subscriber_fails) { return Result<Registration>::fail(Error::TransportFailed); }
        return Result<Registration>::ok(add(DeclaredThing::Subscriber, key, std::move(on_sample)));
    }

    void undeclare(Registration r) noexcept override
    {
        const auto it = declared.find(r.id);
        if (it != declared.end()) { it->second.live = false; }
    }

    // -- inspection ----------------------------------------------------------

    std::size_t live_count(DeclaredThing::Kind k) const
    {
        std::size_t n = 0;
        for (const auto& kv : declared) { if (kv.second.kind == k && kv.second.live) { ++n; } }
        return n;
    }

    const DeclaredThing* find_declared(const std::string& key) const
    {
        for (const auto& kv : declared) { if (kv.second.key == key) { return &kv.second; } }
        return nullptr;
    }

    /// Deliver a sample to whichever live subscriber holds `key`.
    bool deliver(const std::string& key, const std::string& encoding, const std::string& payload)
    {
        for (auto& kv : declared)
        {
            if (kv.second.kind != DeclaredThing::Subscriber || !kv.second.live) { continue; }
            if (kv.second.key != key || !kv.second.on_sample) { continue; }
            const Sample s{key, encoding,
                           BytesView(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size())};
            kv.second.on_sample(s);
            return true;
        }
        return false;
    }

private:
    static std::vector<std::uint8_t> copy(BytesView b)
    {
        return std::vector<std::uint8_t>(b.data, b.data + b.size);
    }

    Registration add(DeclaredThing::Kind k, std::string_view key, SampleFn fn)
    {
        const std::uint64_t id = ++next_;
        declared.emplace(id, DeclaredThing{k, std::string(key), true, std::move(fn)});
        Registration r;
        r.id = id;
        return r;
    }

    std::uint64_t next_ = 0;
};

}  // namespace test
}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_TEST_FAKE_TRANSPORT_H
