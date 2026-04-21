// engine/anim/morph.cpp — Phase 13 Wave 13D (ADR-0042 §5).

#include "engine/anim/morph.hpp"

#include "engine/physics/determinism_hash.hpp"

#include <cstring>

namespace gw::anim {

std::uint64_t morph_set_content_hash(const MorphSetDesc& desc) noexcept {
    std::uint64_t h = gw::physics::kFnvOffset64;

    auto fold_bytes = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };

    const std::uint64_t name_len = desc.name.size();
    fold_bytes(&name_len, sizeof(name_len));
    fold_bytes(desc.name.data(), desc.name.size());

    const std::uint64_t ntargets = desc.targets.size();
    fold_bytes(&ntargets, sizeof(ntargets));
    for (const auto& t : desc.targets) {
        const std::uint64_t nl = t.name.size();
        fold_bytes(&nl, sizeof(nl));
        fold_bytes(t.name.data(), t.name.size());
        fold_bytes(&t.default_weight, sizeof(t.default_weight));
    }

    return h;
}

} // namespace gw::anim
