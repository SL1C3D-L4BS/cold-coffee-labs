#pragma once
// engine/platform_services/rate_limiter.hpp — ADR-0065 §5.
//
// Per-event sliding-window counter.  Keys are string_views hashed with
// fnv1a32; collisions are extremely unlikely for the < 256 event names we
// expect but the test suite probes it explicitly.

#include <cstdint>
#include <string_view>
#include <vector>

namespace gw::platform_services {

class RateLimiter {
public:
    RateLimiter() = default;

    explicit RateLimiter(std::int32_t per_minute) noexcept
        : per_minute_(per_minute) {}

    void set_per_minute(std::int32_t v) noexcept { per_minute_ = v; }
    [[nodiscard]] std::int32_t per_minute() const noexcept { return per_minute_; }

    // Registers the call at `now_ms`.  Returns true if the call is allowed.
    // Once the sliding window count reaches `per_minute_`, further calls
    // fall through until the oldest entry ages out.
    bool check(std::string_view name, std::uint64_t now_ms) noexcept;

    // Peek — does *not* register a call.  Used by `IPlatformServices::would_accept`.
    [[nodiscard]] bool would_accept(std::string_view name,
                                    std::uint64_t    now_ms) const noexcept;

    // Drop everything older than `now_ms - 60000`.  Called each `step()`.
    void evict(std::uint64_t now_ms) noexcept;

    // Reset all counters (tests).
    void clear() noexcept;

    [[nodiscard]] std::size_t tracked_names() const noexcept { return buckets_.size(); }

private:
    struct Bucket {
        std::uint32_t            hash{0};
        std::vector<std::uint64_t> stamps_ms{};
    };

    [[nodiscard]] std::uint32_t hash_(std::string_view s) const noexcept;

    Bucket*       find_or_create_(std::string_view name) noexcept;
    const Bucket* find_(std::string_view name) const noexcept;

    std::vector<Bucket> buckets_{};
    std::int32_t        per_minute_{120};
};

} // namespace gw::platform_services
