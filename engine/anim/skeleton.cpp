// engine/anim/skeleton.cpp — Phase 13 Wave 13A (ADR-0039).

#include "engine/anim/skeleton.hpp"

#include "engine/physics/determinism_hash.hpp"  // fnv1a64 primitives

#include <cstring>

namespace gw::anim {

std::uint64_t skeleton_content_hash(const SkeletonDesc& desc) noexcept {
    std::uint64_t h = gw::physics::kFnvOffset64;

    auto fold_bytes = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };

    const std::uint64_t name_len = desc.name.size();
    fold_bytes(&name_len, sizeof(name_len));
    fold_bytes(desc.name.data(), desc.name.size());

    const std::uint64_t joint_count = desc.joint_names.size();
    fold_bytes(&joint_count, sizeof(joint_count));

    for (std::size_t i = 0; i < desc.joint_names.size(); ++i) {
        const std::uint64_t nm = desc.joint_names[i].size();
        fold_bytes(&nm, sizeof(nm));
        fold_bytes(desc.joint_names[i].data(), desc.joint_names[i].size());
        if (i < desc.parents.size()) {
            const std::uint16_t p = desc.parents[i];
            fold_bytes(&p, sizeof(p));
        } else {
            const std::uint16_t p = kInvalidJoint;
            fold_bytes(&p, sizeof(p));
        }
    }

    return h;
}

} // namespace gw::anim
