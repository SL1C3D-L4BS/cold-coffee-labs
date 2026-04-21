// engine/anim/clip.cpp — Phase 13 Wave 13B (ADR-0040).

#include "engine/anim/clip.hpp"

#include "engine/physics/determinism_hash.hpp"

#include <cstring>

namespace gw::anim {

std::uint64_t clip_content_hash(const ClipDesc& desc) noexcept {
    std::uint64_t h = gw::physics::kFnvOffset64;

    auto fold_bytes = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };

    const std::uint64_t name_len = desc.name.size();
    fold_bytes(&name_len, sizeof(name_len));
    fold_bytes(desc.name.data(), desc.name.size());

    fold_bytes(&desc.key_count, sizeof(desc.key_count));
    fold_bytes(&desc.duration_s, sizeof(desc.duration_s));
    const std::uint8_t loop_byte = desc.looping ? 1u : 0u;
    fold_bytes(&loop_byte, sizeof(loop_byte));
    fold_bytes(&desc.target_skeleton_hash, sizeof(desc.target_skeleton_hash));

    const std::uint64_t ntracks = desc.tracks.size();
    fold_bytes(&ntracks, sizeof(ntracks));
    for (const auto& t : desc.tracks) {
        fold_bytes(&t.joint_index, sizeof(t.joint_index));
        if (!t.translations.empty())
            fold_bytes(t.translations.data(), t.translations.size() * sizeof(glm::vec3));
        if (!t.rotations.empty())
            fold_bytes(t.rotations.data(), t.rotations.size() * sizeof(glm::quat));
        if (!t.scales.empty())
            fold_bytes(t.scales.data(), t.scales.size() * sizeof(glm::vec3));
    }

    if (!desc.root_motion_translation.empty())
        fold_bytes(desc.root_motion_translation.data(),
                   desc.root_motion_translation.size() * sizeof(glm::vec3));
    if (!desc.root_motion_rotation.empty())
        fold_bytes(desc.root_motion_rotation.data(),
                   desc.root_motion_rotation.size() * sizeof(glm::quat));

    return h;
}

} // namespace gw::anim
