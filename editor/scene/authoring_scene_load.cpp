// editor/scene/authoring_scene_load.cpp

#include "editor/scene/authoring_scene_load.hpp"

#include "editor/scene/components.hpp"
#include "engine/ecs/hierarchy.hpp"
#include "engine/scene/migration.hpp"

#include <cstring>
#include <mutex>
#include <span>

namespace gw::editor::scene {

void ensure_authoring_scene_types_for_load(gw::ecs::World& w) {
    (void)w.component_registry().id_of<NameComponent>();
    (void)w.component_registry().id_of<TransformComponent>();
    (void)w.component_registry().id_of<VisibilityComponent>();
    (void)w.component_registry().id_of<WorldMatrixComponent>();
    (void)w.component_registry().id_of<BlockoutPrimitiveComponent>();
    (void)w.component_registry().id_of<EditorCookedMeshComponent>();
    (void)w.component_registry().id_of<gw::ecs::HierarchyComponent>();
}

void register_authoring_scene_migrations_once() {
    static std::once_flag done;
    std::call_once(done, [] {
        using TransformComponent = gw::editor::scene::TransformComponent;
        constexpr std::uint32_t kV1Size = 12u + 16u + 12u;
        static_assert(sizeof(TransformComponent) == 56u,
                      "TransformComponent v2 must land at 56 bytes "
                      "(dvec3+quat+vec3 + 4B trailing padding under 8B alignment)");

        gw::scene::MigrationRegistry::global().register_fn<TransformComponent>(
            kV1Size,
            [](std::span<const std::uint8_t> src,
               std::span<std::uint8_t>       dst) {
                if (src.size() != kV1Size ||
                    dst.size() != sizeof(TransformComponent)) {
                    return false;
                }
                float pos_f[3];
                std::memcpy(pos_f, src.data(), sizeof(pos_f));
                const std::uint8_t* src_after_pos = src.data() + sizeof(pos_f);

                TransformComponent t{};
                t.position = glm::dvec3(
                    static_cast<double>(pos_f[0]),
                    static_cast<double>(pos_f[1]),
                    static_cast<double>(pos_f[2]));
                std::memcpy(&t.rotation, src_after_pos, sizeof(t.rotation));
                std::memcpy(&t.scale, src_after_pos + sizeof(t.rotation),
                            sizeof(t.scale));
                std::memcpy(dst.data(), &t, sizeof(t));
                return true;
            },
            "TransformComponent: widen position vec3->dvec3");

        constexpr std::uint32_t kBlockoutV1 = 4u;
        static_assert(sizeof(BlockoutPrimitiveComponent) == 196u,
                      "keep BlockoutPrimitive migration in sync");
        gw::scene::MigrationRegistry::global().register_fn<BlockoutPrimitiveComponent>(
            kBlockoutV1,
            [](std::span<const std::uint8_t> src,
               std::span<std::uint8_t>       dst) {
                if (src.size() != kBlockoutV1 ||
                    dst.size() != sizeof(BlockoutPrimitiveComponent)) {
                    return false;
                }
                BlockoutPrimitiveComponent b{};
                std::memcpy(&b.shape, src.data(), kBlockoutV1);
                std::memcpy(dst.data(), &b, sizeof(b));
                return true;
            },
            "BlockoutPrimitiveComponent: add gwmat_rel path buffer");
    });
}

} // namespace gw::editor::scene
