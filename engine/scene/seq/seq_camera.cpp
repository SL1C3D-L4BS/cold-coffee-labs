// engine/scene/seq/seq_camera.cpp

#include "engine/scene/seq/seq_camera.hpp"

#include "engine/scene/seq/seq_cut.hpp"

#include "editor/scene/components.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <vector>

namespace gw::seq {

namespace {

struct Candidate {
    gw::ecs::Entity                 entity{};
    std::int32_t                    priority = 0;
    glm::dvec3                      eye{0.0};
    glm::quat                       rot{1.f, 0.f, 0.f, 0.f};
    const CinematicCameraComponent* cam = nullptr;
};

[[nodiscard]] glm::vec3 forward_ws(const glm::quat& q) noexcept {
    return glm::normalize(q * glm::vec3(0.f, 0.f, -1.f));
}

}  // namespace

void CinematicCameraSystem::tick(gw::ecs::World& world) noexcept {
    // Cut-driven two-camera blend (Phase 18-B) overrides priority pick.
    const CameraBlendStateComponent* blend = nullptr;
    world.for_each<CameraBlendStateComponent>([&](const gw::ecs::Entity, const CameraBlendStateComponent& b) {
        blend = &b;
    });
    if (blend != nullptr && blend->blend_active && world.is_alive(blend->cam_a) && world.is_alive(blend->cam_b)) {
        const auto* ta = world.get_component<gw::editor::scene::TransformComponent>(blend->cam_a);
        const auto* tb = world.get_component<gw::editor::scene::TransformComponent>(blend->cam_b);
        const auto* ca = world.get_component<CinematicCameraComponent>(blend->cam_a);
        const auto* cb = world.get_component<CinematicCameraComponent>(blend->cam_b);
        if (ta != nullptr && tb != nullptr && ca != nullptr && cb != nullptr && ca->active && cb->active) {
            const float a  = blend->weight < 0.f ? 0.f : (blend->weight > 1.f ? 1.f : blend->weight);
            const glm::vec3 eye = glm::mix(glm::vec3(ta->position), glm::vec3(tb->position), a);
            const glm::quat rot =
                glm::normalize(glm::slerp(ta->rotation, tb->rotation, a));
            const float fov = glm::mix(ca->fov_deg, cb->fov_deg, a);
            const float nz  = glm::mix(ca->near_plane, cb->near_plane, a);
            const float fz  = glm::mix(ca->far_plane, cb->far_plane, a);

            const glm::vec3 f  = forward_ws(rot);
            const glm::vec3 up = glm::normalize(rot * glm::vec3(0.f, 1.f, 0.f));
            out_.view      = glm::lookAt(eye, eye + f, up);
            out_.fov_y_deg = fov;
            out_.near_z    = nz;
            out_.far_z     = fz;
            out_.valid     = true;
            return;
        }
    }

    std::vector<Candidate> cands;
    world.for_each<gw::editor::scene::TransformComponent, CinematicCameraComponent>(
        [&](gw::ecs::Entity e, gw::editor::scene::TransformComponent& tr, CinematicCameraComponent& cam) {
            if (!cam.active) return;
            cands.push_back(Candidate{e, cam.priority, tr.position, tr.rotation, &cam});
        });

    if (cands.empty()) {
        out_.valid = false;
        return;
    }

    std::sort(cands.begin(), cands.end(),
              [](const Candidate& a, const Candidate& b) { return a.priority > b.priority; });

    const Candidate& win = cands.front();
    glm::vec3        eye = glm::vec3(win.eye);
    glm::quat        rot = win.rot;
    float            fov = win.cam->fov_deg;
    float            nz  = win.cam->near_plane;
    float            fz  = win.cam->far_plane;

    if (auto* blendc = world.get_component<CinematicCameraBlendComponent>(win.entity)) {
        if (world.is_alive(blendc->partner)) {
            const auto* ptr = world.get_component<gw::editor::scene::TransformComponent>(blendc->partner);
            const auto* ocp = world.get_component<CinematicCameraComponent>(blendc->partner);
            if (ptr != nullptr && ocp != nullptr && ocp->active && blendc->duration_frames > 0u &&
                blendc->elapsed_frames < blendc->duration_frames) {
                const float a =
                    static_cast<float>(blendc->elapsed_frames) / static_cast<float>(blendc->duration_frames);
                const glm::vec3 eye_b = glm::vec3(ptr->position);
                const glm::quat rot_b = ptr->rotation;
                eye = glm::mix(eye_b, eye, a);
                rot = glm::normalize(glm::slerp(rot_b, rot, a));
                fov = glm::mix(ocp->fov_deg, fov, a);
                nz  = glm::mix(ocp->near_plane, nz, a);
                fz  = glm::mix(ocp->far_plane, fz, a);
                blendc->elapsed_frames = blendc->elapsed_frames + 1u;
            }
        }
    }

    const glm::vec3 f  = forward_ws(rot);
    const glm::vec3 up = glm::normalize(rot * glm::vec3(0.f, 1.f, 0.f));
    out_.view      = glm::lookAt(eye, eye + f, up);
    out_.fov_y_deg = fov;
    out_.near_z    = nz;
    out_.far_z     = fz;
    out_.valid     = true;
}

}  // namespace gw::seq
