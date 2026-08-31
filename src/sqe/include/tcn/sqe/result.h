// SPDX-License-Identifier: internal
//
// Result<T> — a value or an Error, with no exception and no heap.
//
// See docs/2026-08-31-sqe-contract-library-design.md §4.1.
#ifndef TCN_SQE_RESULT_H
#define TCN_SQE_RESULT_H

#include <cassert>
#include <new>
#include <type_traits>
#include <utility>

#include "tcn/sqe/error.h"

namespace tcn {
namespace sqe {

/// Holds either a `T` or an `Error`, never both and never neither.
///
/// Deliberately not `std::variant`: `std::get` throws and `std::visit` can
/// throw `bad_variant_access`, and §3.3 forbids the core adding any exception
/// requirement of its own. The storage is a manually managed union so that
/// `T` need not be default-constructible — which matters, because `Handle`
/// (§4.4) must not be.
template <class T>
class Result
{
public:
    using value_type = T;

    static Result ok(T v) noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        return Result(std::move(v));
    }

    static Result fail(Error e) noexcept
    {
        assert(e != Error::Ok && "a failed Result must carry a real error");
        return Result(e);
    }

    Result(const Result& o) : err_(o.err_)
    {
        if (o.err_ == Error::Ok) { new (storage()) T(*o.ptr()); }
    }

    Result(Result&& o) noexcept(std::is_nothrow_move_constructible<T>::value) : err_(o.err_)
    {
        if (o.err_ == Error::Ok) { new (storage()) T(std::move(*o.ptr())); }
    }

    Result& operator=(const Result& o)
    {
        if (this != &o) { destroy(); err_ = o.err_; if (o.err_ == Error::Ok) { new (storage()) T(*o.ptr()); } }
        return *this;
    }

    Result& operator=(Result&& o) noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        if (this != &o) { destroy(); err_ = o.err_; if (o.err_ == Error::Ok) { new (storage()) T(std::move(*o.ptr())); } }
        return *this;
    }

    ~Result() { destroy(); }

    bool is_ok() const noexcept { return err_ == Error::Ok; }
    explicit operator bool() const noexcept { return is_ok(); }

    /// `Error::Ok` when this holds a value.
    Error error() const noexcept { return err_; }

    /// Precondition: `is_ok()`. Checked with `assert`, never with a throw.
    const T& value() const noexcept { assert(is_ok()); return *ptr(); }
    T& value() noexcept { assert(is_ok()); return *ptr(); }

    /// The value, or `fallback` — the total accessor, for callers that would
    /// rather not branch.
    T value_or(T fallback) const { return is_ok() ? *ptr() : std::move(fallback); }

private:
    explicit Result(T v) : err_(Error::Ok) { new (storage()) T(std::move(v)); }
    explicit Result(Error e) noexcept : err_(e) {}

    void destroy() noexcept { if (err_ == Error::Ok) { ptr()->~T(); } }

    void* storage() noexcept { return static_cast<void*>(&buf_); }
    T* ptr() noexcept { return reinterpret_cast<T*>(&buf_); }
    const T* ptr() const noexcept { return reinterpret_cast<const T*>(&buf_); }

    Error err_;
    typename std::aligned_storage<sizeof(T), alignof(T)>::type buf_;
};

/// The `void` specialisation: success carries nothing.
template <>
class Result<void>
{
public:
    using value_type = void;

    static Result ok() noexcept { return Result(Error::Ok); }
    static Result fail(Error e) noexcept
    {
        assert(e != Error::Ok && "a failed Result must carry a real error");
        return Result(e);
    }

    bool is_ok() const noexcept { return err_ == Error::Ok; }
    explicit operator bool() const noexcept { return is_ok(); }
    Error error() const noexcept { return err_; }

private:
    explicit Result(Error e) noexcept : err_(e) {}
    Error err_;
};

}  // namespace sqe
}  // namespace tcn

#endif  // TCN_SQE_RESULT_H
