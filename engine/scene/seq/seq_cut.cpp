// engine/scene/seq/seq_cut.cpp

#include "engine/scene/seq/seq_cut.hpp"

#include "engine/scene/seq/seq_camera.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace gw::seq {
namespace {

void set_cinematic_all_off(gw::ecs::World& world) noexcept {
    world.for_each<CinematicCameraComponent>([&](const gw::ecs::Entity, CinematicCameraComponent& cc) {
        cc.active = false;
    });
}

void set_active(gw::ecs::World& world, const gw::ecs::Entity e, const bool on) noexcept {
    if (!world.is_alive(e)) return;
    if (auto* c = world.get_component<CinematicCameraComponent>(e)) c->active = on;
}

[[nodiscard]] float apply_curve(const CutBlendCurve c, const float w) noexcept {
    const float x = w < 0.f ? 0.f : (w > 1.f ? 1.f : w);
    switch (c) {
        case CutBlendCurve::Cut: return x;
        case CutBlendCurve::Linear: return x;
        case CutBlendCurve::EaseOut: {
            const float t = 1.f - x;
            return 1.f - t * t;
        }
    }
    return x;
}

void clear_blend(CameraBlendStateComponent& s) noexcept {
    s.cam_a         = {};
    s.cam_b         = {};
    s.weight        = 0.f;
    s.blend_active  = false;
}

void ensure_blend_singleton(gw::ecs::World& world) noexcept {
    std::uint32_t n = 0;
    world.for_each<CameraBlendStateComponent>([&](const gw::ecs::Entity, const CameraBlendStateComponent&) {
        ++n;
    });
    if (n == 0u) {
        const gw::ecs::Entity e = world.create_entity();
        (void)world.add_component<CameraBlendStateComponent>(e, CameraBlendStateComponent{});
    }
}

}  // namespace

void CutSystem::tick(gw::ecs::World& world, double play_head_frame) noexcept {
    ensure_blend_singleton(world);
    CameraBlendStateComponent* blend = nullptr;
    world.for_each<CameraBlendStateComponent>([&](const gw::ecs::Entity, CameraBlendStateComponent& s) {
        if (blend == nullptr) blend = &s;
    });
    if (blend == nullptr) return;

    const CutListComponent* list = nullptr;
    world.for_each<CutListComponent>([&](const gw::ecs::Entity, const CutListComponent& cl) {
        if (cl.cut_count > 0u) list = &cl;
    });

    if (list == nullptr || list->cut_count == 0u) {
        clear_blend(*blend);
        return;
    }

    struct Ord {
        std::uint32_t frame{};
        std::uint32_t ix{};
    };
    std::vector<Ord> order;
    order.reserve(list->cut_count);
    for (std::uint32_t i = 0; i < list->cut_count; ++i) order.push_back(Ord{list->cuts[i].frame, i});
    std::sort(order.begin(), order.end(), [](const Ord& a, const Ord& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.ix < b.ix;
    });

    const double t = play_head_frame;
    if (t < static_cast<double>(order.front().frame)) {
        clear_blend(*blend);
        set_cinematic_all_off(world);
        set_active(world, list->cuts[order.front().ix].from_camera, true);
        return;
    }

    // Last cut with frame <= t
    std::uint32_t i = 0u;
    for (std::uint32_t k = 0; k < order.size(); ++k) {
        if (static_cast<double>(order[k].frame) <= t + 1e-9) i = k;
    }
    const CutEntry& c = list->cuts[order[i].ix];
    if (c.blend_frames > 0u) {
        const double F  = static_cast<double>(c.frame);
        const double B  = static_cast<double>(c.blend_frames);
        const double eB = F + B;
        if (t < eB - 1e-9) {
            if (t < F) {
                clear_blend(*blend);
                set_cinematic_all_off(world);
                set_active(world, c.from_camera, true);
                return;
            }
            const float raw = static_cast<float>((t - F) / B);
            const float w   = apply_curve(c.blend_curve, raw);
            set_cinematic_all_off(world);
            set_active(world, c.from_camera, true);
            set_active(world, c.to_camera, true);
            blend->cam_a         = c.from_camera;
            blend->cam_b         = c.to_camera;
            blend->weight        = w;
            blend->blend_active  = true;
            return;
        }
    }

    // Hard selection: show to_camera of cut i
    clear_blend(*blend);
    set_cinematic_all_off(world);
    set_active(world, c.to_camera, true);
}

}  // namespace gw::seq
