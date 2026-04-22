// engine/platform_services/rate_limiter.cpp

#include "engine/platform_services/rate_limiter.hpp"

#include <algorithm>

namespace gw::platform_services {

std::uint32_t RateLimiter::hash_(std::string_view s) const noexcept {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= static_cast<std::uint32_t>(c);
        h *= 16777619u;
    }
    return h;
}

RateLimiter::Bucket* RateLimiter::find_or_create_(std::string_view name) noexcept {
    const auto h = hash_(name);
    for (auto& b : buckets_) {
        if (b.hash == h) return &b;
    }
    buckets_.push_back(Bucket{h, {}});
    return &buckets_.back();
}

const RateLimiter::Bucket* RateLimiter::find_(std::string_view name) const noexcept {
    const auto h = hash_(name);
    for (const auto& b : buckets_) {
        if (b.hash == h) return &b;
    }
    return nullptr;
}

bool RateLimiter::check(std::string_view name, std::uint64_t now_ms) noexcept {
    auto* b = find_or_create_(name);
    // evict inline
    const std::uint64_t cutoff = (now_ms >= 60'000) ? (now_ms - 60'000) : 0;
    auto it = std::lower_bound(b->stamps_ms.begin(), b->stamps_ms.end(), cutoff);
    b->stamps_ms.erase(b->stamps_ms.begin(), it);
    if (per_minute_ > 0
        && b->stamps_ms.size() >= static_cast<std::size_t>(per_minute_)) {
        return false;
    }
    b->stamps_ms.push_back(now_ms);
    return true;
}

bool RateLimiter::would_accept(std::string_view name,
                               std::uint64_t    now_ms) const noexcept {
    const auto* b = find_(name);
    if (!b) return per_minute_ != 0;
    const std::uint64_t cutoff = (now_ms >= 60'000) ? (now_ms - 60'000) : 0;
    std::size_t live = 0;
    for (auto s : b->stamps_ms) {
        if (s >= cutoff) ++live;
    }
    if (per_minute_ <= 0) return true;
    return live < static_cast<std::size_t>(per_minute_);
}

void RateLimiter::evict(std::uint64_t now_ms) noexcept {
    const std::uint64_t cutoff = (now_ms >= 60'000) ? (now_ms - 60'000) : 0;
    for (auto& b : buckets_) {
        auto it = std::lower_bound(b.stamps_ms.begin(), b.stamps_ms.end(), cutoff);
        b.stamps_ms.erase(b.stamps_ms.begin(), it);
    }
}

void RateLimiter::clear() noexcept { buckets_.clear(); }

} // namespace gw::platform_services
