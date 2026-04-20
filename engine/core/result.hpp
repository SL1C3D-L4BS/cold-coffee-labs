#pragma once

#include <variant>

namespace gw::core {

template <typename T, typename E>
class Result final {
public:
    Result(const T& value) : storage_(value) {}
    Result(T&& value) : storage_(std::move(value)) {}
    Result(const E& error) : storage_(error) {}
    Result(E&& error) : storage_(std::move(error)) {}

    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] const T& value() const& {
        return std::get<T>(storage_);
    }
    [[nodiscard]] T& value() & {
        return std::get<T>(storage_);
    }
    [[nodiscard]] T&& value() && {
        return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] const E& error() const& {
        return std::get<E>(storage_);
    }
    [[nodiscard]] E& error() & {
        return std::get<E>(storage_);
    }

private:
    std::variant<T, E> storage_;
};

}  // namespace gw::core
