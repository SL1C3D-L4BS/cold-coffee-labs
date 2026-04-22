#pragma once
// engine/render/shader/permutation_cache.hpp — ADR-0074 Wave 17A.
//
// Content-hash LRU for compiled SPIR-V permutations. Keyed by
// hash(source || defines || compiler_version). Value is a SPIR-V word
// stream + companion ReflectionReport. Hard cap governed by
// r.shader.cache_max_mb. Zero external deps — xxhash is used solely
// to diffuse the content hash (identity, not security).

#include "engine/render/shader/permutation.hpp"
#include "engine/render/shader/reflect.hpp"

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gw::render::shader {

struct CachedPermutation {
    std::vector<std::uint32_t> spirv_words;
    ReflectionReport           reflection;
    std::uint64_t              content_hash{0};
};

struct CacheKey {
    std::string     template_name;
    PermutationKey  permutation;
    std::uint64_t   content_hash{0};
};

struct CacheStats {
    std::uint64_t hits{0};
    std::uint64_t misses{0};
    std::uint64_t evictions{0};
    std::uint64_t bytes_used{0};
    std::uint64_t bytes_cap{0};
    [[nodiscard]] double hit_rate() const noexcept {
        const auto total = hits + misses;
        return total == 0 ? 0.0 : static_cast<double>(hits) / static_cast<double>(total);
    }
};

class PermutationCache {
public:
    explicit PermutationCache(std::uint64_t cap_bytes = 256ull * 1024ull * 1024ull);
    ~PermutationCache() = default;
    PermutationCache(const PermutationCache&) = delete;
    PermutationCache& operator=(const PermutationCache&) = delete;

    void set_capacity_bytes(std::uint64_t bytes) noexcept;
    [[nodiscard]] std::uint64_t capacity_bytes() const noexcept { return cap_bytes_; }

    // Pure utility: FNV-1a-64 + Fibonacci diffusion of the given inputs.
    // The cache does not care how the caller computes hashes, but it
    // canonicalises via this helper so reflect/material/.gwmat all share it.
    [[nodiscard]] static std::uint64_t compute_hash(std::string_view source,
                                                     std::string_view defines,
                                                     std::string_view compiler_version) noexcept;

    // Mutators.
    [[nodiscard]] bool  insert(const CacheKey& key, CachedPermutation v);
    [[nodiscard]] const CachedPermutation* get(const CacheKey& key);   // bumps LRU
    void clear() noexcept;

    // Introspection.
    [[nodiscard]] CacheStats stats() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return map_.size(); }

    // Returns the list of keys in LRU order (most-recently-used first).
    // Stable + deterministic — used by tests.
    [[nodiscard]] std::vector<std::string> keys_in_lru_order() const;

private:
    using ListIter = std::list<std::pair<std::string, CachedPermutation>>::iterator;
    std::list<std::pair<std::string, CachedPermutation>> lru_;
    std::unordered_map<std::string, ListIter>            map_;
    std::uint64_t                                        bytes_used_{0};
    std::uint64_t                                        cap_bytes_{256ull * 1024ull * 1024ull};
    mutable std::uint64_t                                hits_{0};
    mutable std::uint64_t                                misses_{0};
    std::uint64_t                                        evictions_{0};

    [[nodiscard]] static std::string encode_(const CacheKey& key);
    void evict_until_fits_(std::uint64_t incoming_bytes);
};

} // namespace gw::render::shader
