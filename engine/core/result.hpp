#pragma once
// engine/core/result.hpp — Greywater error-handling primitive.
//
// gw::core::Result<T, E> is a value-or-error type modelled on std::expected
// (C++23).  Where callers have access to <expected> directly they may also
// use std::expected; this type exists as the stable, project-internal ABI
// that can be customised (e.g. adding stack traces, telemetry) without
// touching every call site.
//
// Monadic operations (map, and_then, or_else, transform_error) follow the
// C++23 std::expected API surface so code can be mechanically migrated later.

#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace gw {
namespace core {

// ---------------------------------------------------------------------------
// Sentinel type used to construct a Result in the error state.
// Usage: return gw::core::unexpected(SomeError{...});
// ---------------------------------------------------------------------------
template<typename E>
struct Unexpected {
    E value;
    explicit Unexpected(E v) : value(std::move(v)) {}
};

template<typename E>
[[nodiscard]] Unexpected<std::decay_t<E>> unexpected(E&& e) {
    return Unexpected<std::decay_t<E>>{std::forward<E>(e)};
}

// ---------------------------------------------------------------------------
// Result<T, E>
// ---------------------------------------------------------------------------
template<typename T, typename E>
class Result final {
    static_assert(!std::is_reference_v<T>, "T must not be a reference");
    static_assert(!std::is_reference_v<E>, "E must not be a reference");

public:
    using value_type = T;
    using error_type = E;

    // Construct from a value (success)
    Result(const T& v) : storage_(std::in_place_index<0>, v) {}  // NOLINT
    Result(T&& v)      : storage_(std::in_place_index<0>, std::move(v)) {}

    // Construct from an error
    Result(const E& e) : storage_(std::in_place_index<1>, e) {}  // NOLINT
    Result(E&& e)      : storage_(std::in_place_index<1>, std::move(e)) {}

    // Construct from Unexpected<E> (like std::unexpected)
    Result(Unexpected<E> u)  // NOLINT
        : storage_(std::in_place_index<1>, std::move(u.value)) {}

    // --- Observers ----------------------------------------------------------

    [[nodiscard]] bool has_value() const noexcept {
        return storage_.index() == 0;
    }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] const T& value() const& {
        if (!has_value()) throw std::bad_variant_access{};
        return std::get<0>(storage_);
    }
    [[nodiscard]] T& value() & {
        if (!has_value()) throw std::bad_variant_access{};
        return std::get<0>(storage_);
    }
    [[nodiscard]] T&& value() && {
        if (!has_value()) throw std::bad_variant_access{};
        return std::get<0>(std::move(storage_));
    }

    [[nodiscard]] const E& error() const& {
        if (has_value()) throw std::bad_variant_access{};
        return std::get<1>(storage_);
    }
    [[nodiscard]] E& error() & {
        if (has_value()) throw std::bad_variant_access{};
        return std::get<1>(storage_);
    }
    [[nodiscard]] E&& error() && {
        if (has_value()) throw std::bad_variant_access{};
        return std::get<1>(std::move(storage_));
    }

    // value_or — return the value, or a fallback if in error state.
    template<typename U>
    [[nodiscard]] T value_or(U&& fallback) const& {
        if (has_value()) return std::get<0>(storage_);
        return static_cast<T>(std::forward<U>(fallback));
    }
    template<typename U>
    [[nodiscard]] T value_or(U&& fallback) && {
        if (has_value()) return std::get<0>(std::move(storage_));
        return static_cast<T>(std::forward<U>(fallback));
    }

    // --- Monadic operations (C++23 std::expected API) -----------------------

    // map — transform the value, propagate errors unchanged.
    // F: T -> U   → Result<U, E>
    template<typename F>
    [[nodiscard]] auto map(F&& f) const& {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value()) return Result<U, E>{std::invoke(std::forward<F>(f), std::get<0>(storage_))};
        return Result<U, E>{std::get<1>(storage_)};
    }
    template<typename F>
    [[nodiscard]] auto map(F&& f) && {
        using U = std::invoke_result_t<F, T>;
        if (has_value()) return Result<U, E>{std::invoke(std::forward<F>(f), std::get<0>(std::move(storage_)))};
        return Result<U, E>{std::get<1>(std::move(storage_))};
    }

    // and_then — chain another Result-returning computation.
    // F: T -> Result<U, E>
    template<typename F>
    [[nodiscard]] auto and_then(F&& f) const& {
        using Ret = std::invoke_result_t<F, const T&>;
        if (has_value()) return std::invoke(std::forward<F>(f), std::get<0>(storage_));
        return Ret{std::get<1>(storage_)};
    }
    template<typename F>
    [[nodiscard]] auto and_then(F&& f) && {
        using Ret = std::invoke_result_t<F, T>;
        if (has_value()) return std::invoke(std::forward<F>(f), std::get<0>(std::move(storage_)));
        return Ret{std::get<1>(std::move(storage_))};
    }

    // or_else — recover from an error; F must return Result<T, G>.
    // F: E -> Result<T, G>
    template<typename F>
    [[nodiscard]] auto or_else(F&& f) const& {
        using Ret = std::invoke_result_t<F, const E&>;
        if (!has_value()) return std::invoke(std::forward<F>(f), std::get<1>(storage_));
        return Ret{std::get<0>(storage_)};
    }
    template<typename F>
    [[nodiscard]] auto or_else(F&& f) && {
        using Ret = std::invoke_result_t<F, E>;
        if (!has_value()) return std::invoke(std::forward<F>(f), std::get<1>(std::move(storage_)));
        return Ret{std::get<0>(std::move(storage_))};
    }

    // transform_error — map the error type; values pass through unchanged.
    // F: E -> G   → Result<T, G>
    template<typename F>
    [[nodiscard]] auto transform_error(F&& f) const& {
        using G = std::invoke_result_t<F, const E&>;
        if (!has_value()) return Result<T, G>{std::invoke(std::forward<F>(f), std::get<1>(storage_))};
        return Result<T, G>{std::get<0>(storage_)};
    }
    template<typename F>
    [[nodiscard]] auto transform_error(F&& f) && {
        using G = std::invoke_result_t<F, E>;
        if (!has_value()) return Result<T, G>{std::invoke(std::forward<F>(f), std::get<1>(std::move(storage_)))};
        return Result<T, G>{std::get<0>(std::move(storage_))};
    }

    // --- Comparison ---------------------------------------------------------
    bool operator==(const Result& o) const noexcept { return storage_ == o.storage_; }
    bool operator!=(const Result& o) const noexcept { return !(*this == o); }

private:
    std::variant<T, E> storage_;
};

} // namespace core
} // namespace gw
