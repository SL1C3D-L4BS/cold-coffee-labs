// engine/render/shader/permutation_cache.cpp — ADR-0074 Wave 17A.

#include "engine/render/shader/permutation_cache.hpp"

#include <cstring>
#include <utility>

namespace gw::render::shader {

namespace {

constexpr std::uint64_t kFnvOffset64 = 0xcbf29ce484222325ull;
constexpr std::uint64_t kFnvPrime64  = 0x00000100000001b3ull;

inline std::uint64_t fnv1a_mix(std::uint64_t h, std::string_view s) noexcept {
    for (unsigned char c : s) {
        h ^= static_cast<std::uint64_t>(c);
        h *= kFnvPrime64;
    }
    return h;
}

// Fibonacci constant for 64-bit diffusion (ADR-0076 §3 mirror).
constexpr std::uint64_t kFib64 = 0x9E3779B97F4A7C15ull;

inline std::uint64_t diffuse(std::uint64_t h) noexcept {
    h ^= h >> 27;
    h *= kFib64;
    h ^= h >> 31;
    return h;
}

inline std::uint64_t bytes_of(const CachedPermutation& v) noexcept {
    return static_cast<std::uint64_t>(v.spirv_words.size() * sizeof(std::uint32_t))
         + static_cast<std::uint64_t>(v.reflection.bindings.size() * sizeof(DescriptorBinding))
         + 64ull;
}

} // namespace

std::uint64_t PermutationCache::compute_hash(std::string_view source,
                                               std::string_view defines,
                                               std::string_view compiler_version) noexcept {
    std::uint64_t h = kFnvOffset64;
    h = fnv1a_mix(h, source);
    h = fnv1a_mix(h, "\x01");
    h = fnv1a_mix(h, defines);
    h = fnv1a_mix(h, "\x02");
    h = fnv1a_mix(h, compiler_version);
    return diffuse(h);
}

PermutationCache::PermutationCache(std::uint64_t cap_bytes) : cap_bytes_(cap_bytes) {}

void PermutationCache::set_capacity_bytes(std::uint64_t bytes) noexcept {
    cap_bytes_ = bytes;
    evict_until_fits_(0);
}

std::string PermutationCache::encode_(const CacheKey& k) {
    std::string out;
    out.reserve(k.template_name.size() + 24);
    out.append(k.template_name);
    out.push_back('|');
    // Fixed-width hex for permutation + hash.
    static constexpr char kHex[] = "0123456789abcdef";
    for (int s = 28; s >= 0; s -= 4) out.push_back(kHex[(k.permutation.bits >> s) & 0xF]);
    out.push_back('|');
    for (int s = 60; s >= 0; s -= 4) out.push_back(kHex[(k.content_hash >> s) & 0xF]);
    return out;
}

void PermutationCache::evict_until_fits_(std::uint64_t incoming_bytes) {
    while (bytes_used_ + incoming_bytes > cap_bytes_ && !lru_.empty()) {
        auto& tail = lru_.back();
        bytes_used_ -= bytes_of(tail.second);
        map_.erase(tail.first);
        lru_.pop_back();
        ++evictions_;
    }
}

bool PermutationCache::insert(const CacheKey& key, CachedPermutation v) {
    const auto k        = encode_(key);
    const auto incoming = bytes_of(v);
    if (incoming > cap_bytes_) return false;

    if (auto it = map_.find(k); it != map_.end()) {
        bytes_used_ -= bytes_of(it->second->second);
        it->second->second = std::move(v);
        bytes_used_ += bytes_of(it->second->second);
        lru_.splice(lru_.begin(), lru_, it->second);
        evict_until_fits_(0);
        return true;
    }
    evict_until_fits_(incoming);
    lru_.emplace_front(k, std::move(v));
    map_[k] = lru_.begin();
    bytes_used_ += incoming;
    return true;
}

const CachedPermutation* PermutationCache::get(const CacheKey& key) {
    const auto k = encode_(key);
    auto it = map_.find(k);
    if (it == map_.end()) { ++misses_; return nullptr; }
    lru_.splice(lru_.begin(), lru_, it->second);
    ++hits_;
    return &it->second->second;
}

void PermutationCache::clear() noexcept {
    lru_.clear();
    map_.clear();
    bytes_used_ = 0;
}

CacheStats PermutationCache::stats() const noexcept {
    CacheStats s{};
    s.hits       = hits_;
    s.misses     = misses_;
    s.evictions  = evictions_;
    s.bytes_used = bytes_used_;
    s.bytes_cap  = cap_bytes_;
    return s;
}

std::vector<std::string> PermutationCache::keys_in_lru_order() const {
    std::vector<std::string> out;
    out.reserve(lru_.size());
    for (const auto& [k, _] : lru_) out.push_back(k);
    return out;
}

} // namespace gw::render::shader
