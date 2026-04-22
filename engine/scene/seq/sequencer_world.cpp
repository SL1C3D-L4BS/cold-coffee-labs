// engine/scene/seq/sequencer_world.cpp

#include "engine/scene/seq/sequencer_world.hpp"

#include "engine/core/config/cvar.hpp"
#include "engine/core/config/cvar_registry.hpp"
#include "engine/memory/arena_allocator.hpp"

#include "editor/scene/components.hpp"

#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gw::seq {

namespace {

template <typename K>
[[nodiscard]] std::size_t find_segment(const K* frames, std::size_t n, double frame) noexcept {
    if (n == 0u) return 0u;
    if (frame <= static_cast<double>(frames[0].frame)) return 0u;
    if (frame >= static_cast<double>(frames[n - 1].frame)) return n - 1u;
    for (std::size_t i = 0; i + 1u < n; ++i) {
        const double a = static_cast<double>(frames[i].frame);
        const double b = static_cast<double>(frames[i + 1u].frame);
        if (frame >= a && frame < b) return i;
    }
    return n - 2u;
}

[[nodiscard]] glm::dvec3 catmull_rom(const glm::dvec3& p0, const glm::dvec3& p1, const glm::dvec3& p2,
                                     const glm::dvec3& p3, double t) noexcept {
    const double t2 = t * t;
    const double t3 = t2 * t;
    return 0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
                  (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

[[nodiscard]] double hermite_scalar(double v0, double v1, double m0, double m1, double t) noexcept {
    const double t2 = t * t;
    const double t3 = t2 * t;
    const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    const double h10 = t3 - 2.0 * t2 + t;
    const double h01 = -2.0 * t3 + 3.0 * t2;
    const double h11 = t3 - t2;
    return h00 * v0 + h10 * m0 + h01 * v1 + h11 * m1;
}

[[nodiscard]] float sample_float_track(std::span<const GwseqKeyframe<float>> frames,
                                       double head) noexcept {
    if (frames.empty()) return 0.f;
    const std::size_t i = find_segment(frames.data(), frames.size(), head);
    if (i + 1u >= frames.size()) return frames.back().value;

    const auto& k0 = frames[i];
    const auto& k1 = frames[i + 1u];
    const double fa = static_cast<double>(k0.frame);
    const double fb = static_cast<double>(k1.frame);
    if (fb <= fa) return k0.value;

    const double t = (head - fa) / (fb - fa);
    if (k1.interpolation == GwseqInterpolation::Step || k0.interpolation == GwseqInterpolation::Step) {
        return t < 1.0 ? k0.value : k1.value;
    }
    if (k0.interpolation == GwseqInterpolation::Bezier || k1.interpolation == GwseqInterpolation::Bezier) {
        const double m0 = static_cast<double>(k0.tangent_out.x);
        const double m1 = static_cast<double>(k1.tangent_in.x);
        const double s =
            hermite_scalar(static_cast<double>(k0.value), static_cast<double>(k1.value), m0, m1, t);
        return static_cast<float>(s);
    }
    return static_cast<float>(static_cast<double>(k0.value) * (1.0 - t) +
                               static_cast<double>(k1.value) * t);
}

[[nodiscard]] glm::dvec3 sample_vec3_track(std::span<const GwseqKeyframe<Vec3_f64>> frames,
                                           double head) noexcept {
    if (frames.empty()) return {};
    const std::size_t n = frames.size();
    const std::size_t i = find_segment(frames.data(), n, head);
    if (i + 1u >= n) return frames.back().value;

    const auto& k0 = frames[i];
    const auto& k1 = frames[i + 1u];
    const double fa = static_cast<double>(k0.frame);
    const double fb = static_cast<double>(k1.frame);
    if (fb <= fa) return k0.value;

    const double t = (head - fa) / (fb - fa);
    if (k1.interpolation == GwseqInterpolation::Step || k0.interpolation == GwseqInterpolation::Step) {
        return t < 1.0 ? k0.value : k1.value;
    }
    if (k0.interpolation == GwseqInterpolation::Linear && k1.interpolation == GwseqInterpolation::Linear) {
        return glm::mix(k0.value, k1.value, t);
    }

    const auto pick = [&](std::ptrdiff_t idx) -> glm::dvec3 {
        if (idx < 0) return frames.front().value;
        if (static_cast<std::size_t>(idx) >= n) return frames.back().value;
        return frames[static_cast<std::size_t>(idx)].value;
    };
    const glm::dvec3 p0 = pick(static_cast<std::ptrdiff_t>(i) - 1);
    const glm::dvec3 p1 = frames[i].value;
    const glm::dvec3 p2 = frames[i + 1u].value;
    const glm::dvec3 p3 = pick(static_cast<std::ptrdiff_t>(i) + 2);
    return catmull_rom(p0, p1, p2, p3, t);
}

[[nodiscard]] glm::dquat sample_quat_track(std::span<const GwseqKeyframe<GwseqQuat>> frames,
                                           double head) noexcept {
    if (frames.empty()) return glm::dquat(1.0, 0.0, 0.0, 0.0);
    const std::size_t n = frames.size();
    const std::size_t i = find_segment(frames.data(), n, head);
    if (i + 1u >= n) return frames.back().value;

    const auto& k0 = frames[i];
    const auto& k1 = frames[i + 1u];
    const double fa = static_cast<double>(k0.frame);
    const double fb = static_cast<double>(k1.frame);
    if (fb <= fa) return k0.value;

    const float t = static_cast<float>((head - fa) / (fb - fa));
    if (k1.interpolation == GwseqInterpolation::Step || k0.interpolation == GwseqInterpolation::Step) {
        return t < 1.0f ? k0.value : k1.value;
    }
    const glm::quat q0(static_cast<float>(k0.value.w), static_cast<float>(k0.value.x),
                       static_cast<float>(k0.value.y), static_cast<float>(k0.value.z));
    const glm::quat q1(static_cast<float>(k1.value.w), static_cast<float>(k1.value.x),
                       static_cast<float>(k1.value.y), static_cast<float>(k1.value.z));
    const glm::quat qs = glm::normalize(glm::slerp(q0, q1, t));
    return glm::dquat(static_cast<double>(qs.w), static_cast<double>(qs.x), static_cast<double>(qs.y),
                    static_cast<double>(qs.z));
}

}  // namespace

SeqResult SequencerWorld::register_sequence(const gw::assets::AssetHandle asset,
                                            std::vector<std::uint8_t>   bytes) {
    GwseqReader r;
    const auto opened = r.open_bytes(std::move(bytes));
    if (!opened.has_value()) return opened;
    readers_[asset.bits] = std::move(r);
    return SeqOk{};
}

void SequencerWorld::unregister_sequence(const gw::assets::AssetHandle asset) noexcept {
    readers_.erase(asset.bits);
}

GwseqReader* SequencerWorld::reader_for(const gw::assets::AssetHandle asset) noexcept {
    const auto it = readers_.find(asset.bits);
    if (it == readers_.end()) return nullptr;
    return &it->second;
}

const GwseqReader* SequencerWorld::reader_for(const gw::assets::AssetHandle asset) const noexcept {
    const auto it = readers_.find(asset.bits);
    if (it == readers_.end()) return nullptr;
    return &it->second;
}

void SequencerWorld::post_pending_event(const std::uint32_t callback_id) noexcept {
    pending_events_.push_back(callback_id);
}

void SequencerWorld::drain_event_callbacks(std::vector<std::uint32_t>& out) noexcept {
    out.insert(out.end(), pending_events_.begin(), pending_events_.end());
    pending_events_.clear();
}

void SequencerWorld::clear() noexcept {
    readers_.clear();
    pending_events_.clear();
}

void SequencerSystem::tick(SequencerWorld& tapes, gw::ecs::World& world,
                           gw::config::CVarRegistry* cvars, const float dt) noexcept {
    gw::config::CVarRegistry* reg = cvars != nullptr ? cvars : tapes.cvars_;
    tapes.pending_events_.clear();

    gw::memory::ArenaAllocator arena(64u * 1024u);

    world.for_each<SeqPlayerComponent>([&](gw::ecs::Entity entity, SeqPlayerComponent& player) {
        if (!player.playing) return;
        GwseqReader* reader = tapes.reader_for(player.seq_asset_id);
        if (reader == nullptr) return;

        const double rate = static_cast<double>(reader->header().frame_rate) *
                            static_cast<double>(player.playback_rate);
        const double prev = player.play_head_frame;
        player.play_head_frame += static_cast<double>(dt) * rate;

        const double dur = static_cast<double>(reader->header().duration_frames);
        if (dur > 0.0 && player.play_head_frame >= dur) {
            if (player.loop) {
                player.play_head_frame = std::fmod(std::max(0.0, player.play_head_frame), dur);
            } else {
                player.play_head_frame = dur;
                player.playing         = false;
            }
        }

        if (auto* tr = world.get_component<SeqTransformTrackComponent>(entity)) {
            GwseqTrackView<Vec3_f64> v3{};
            GwseqTrackView<GwseqQuat> rq{};
            const SeqResult r3 = reader->read_track(tr->track_id, arena, v3);
            if (r3.has_value() && !v3.empty()) {
                if (world.is_alive(tr->target_entity)) {
                    if (auto* tc =
                            world.get_component<gw::editor::scene::TransformComponent>(tr->target_entity)) {
                        const glm::dvec3 s = sample_vec3_track(v3.frames, player.play_head_frame);
                        tc->position = s;
                    }
                }
            } else {
                arena.reset();
                const SeqResult rq_r = reader->read_track(tr->track_id, arena, rq);
                if (rq_r.has_value() && !rq.empty()) {
                    if (world.is_alive(tr->target_entity)) {
                        if (auto* tc = world.get_component<gw::editor::scene::TransformComponent>(
                                tr->target_entity)) {
                            const glm::dquat dq = sample_quat_track(rq.frames, player.play_head_frame);
                            tc->rotation =
                                glm::quat(static_cast<float>(dq.w), static_cast<float>(dq.x),
                                          static_cast<float>(dq.y), static_cast<float>(dq.z));
                        }
                    }
                }
            }
            arena.reset();
        }

        if (auto* ev = world.get_component<SeqEventTrackComponent>(entity)) {
            GwseqTrackView<bool> evw{};
            const SeqResult er = reader->read_track(ev->track_id, arena, evw);
            if (er.has_value() && !evw.empty()) {
                for (const auto& kf : evw.frames) {
                    const double f = static_cast<double>(kf.frame);
                    if (prev < f && player.play_head_frame >= f && kf.value) {
                        tapes.post_pending_event(ev->callback_id);
                    }
                }
            }
            arena.reset();
        }

        if (reg != nullptr) {
            if (auto* ft = world.get_component<SeqFloatTrackComponent>(entity)) {
                GwseqTrackView<float> fv{};
                const SeqResult fr = reader->read_track(ft->track_id, arena, fv);
                if (fr.has_value() && !fv.empty()) {
                    const float v = sample_float_track(fv.frames, player.play_head_frame);
                    (void)reg->set_f32(gw::config::CVarId{ft->target_cvar_id}, v,
                                       gw::config::kOriginProgrammatic);
                }
                arena.reset();
            }
        }
    });
}

}  // namespace gw::seq
