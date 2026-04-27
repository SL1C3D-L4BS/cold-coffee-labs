#include "editor/scene/blockout_brush_export.hpp"

#include "editor/scene/components.hpp"
#include "engine/math/vec.hpp"

#include <cmath>
#include <limits>

#include <glm/gtc/quaternion.hpp>

namespace gw::editor::scene {

void append_blockout_brush_aabbs(gw::ecs::World& world,
                                 std::vector<gw::services::level_architect::BrushAabb>& out) {
    world.for_each<TransformComponent, BlockoutPrimitiveComponent>(
        [&](gw::ecs::Entity /*e*/, const TransformComponent& t, const BlockoutPrimitiveComponent& b) {
            if (b.shape != 0) {
                return;
            }
            const float hx =
                0.5f * std::max(0.01f, static_cast<float>(std::abs(t.scale.x)));
            const float hy =
                0.5f * std::max(0.01f, static_cast<float>(std::abs(t.scale.y)));
            const float hz =
                0.5f * std::max(0.01f, static_cast<float>(std::abs(t.scale.z)));

            const glm::mat3 R = glm::mat3_cast(t.rotation);
            glm::vec3       mn(std::numeric_limits<float>::max());
            glm::vec3       mx(-std::numeric_limits<float>::max());
            for (int i = 0; i < 8; ++i) {
                const float sx = (i & 1) ? hx : -hx;
                const float sy = (i & 2) ? hy : -hy;
                const float sz = (i & 4) ? hz : -hz;
                const glm::vec3 local{sx, sy, sz};
                const glm::vec3 rotated = R * local;
                const glm::vec3 wc{
                    rotated.x + static_cast<float>(t.position.x),
                    rotated.y + static_cast<float>(t.position.y),
                    rotated.z + static_cast<float>(t.position.z),
                };
                mn = glm::min(mn, wc);
                mx = glm::max(mx, wc);
            }
            gw::services::level_architect::BrushAabb box{};
            box.min = gw::math::Vec3f{mn.x, mn.y, mn.z};
            box.max = gw::math::Vec3f{mx.x, mx.y, mx.z};
            out.push_back(box);
        });
}

}  // namespace gw::editor::scene
