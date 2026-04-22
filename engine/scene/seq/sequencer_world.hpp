#pragma once
// engine/scene/seq/sequencer_world.hpp
// Timeline ECS integration — Phase 18-A.

#include "engine/assets/asset_handle.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/seq/gwseq_codec.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gw::config {
class CVarRegistry;
}

namespace gw::seq {

/// Owns decoded `.gwseq` readers keyed by `AssetHandle::bits`.
class SequencerWorld {
public:
    SequencerWorld() noexcept = default;

    SequencerWorld(const SequencerWorld&)            = delete;
    SequencerWorld& operator=(const SequencerWorld&) = delete;

    SequencerWorld(SequencerWorld&&) noexcept            = default;
    SequencerWorld& operator=(SequencerWorld&&) noexcept = default;

    /// Optional CVar sink for `SeqFloatTrackComponent` / `SeqBool`-style data.
    void set_cvar_registry(gw::config::CVarRegistry* reg) noexcept { cvars_ = reg; }

    /// Installs or replaces a reader for `asset`.
    [[nodiscard]] SeqResult register_sequence(gw::assets::AssetHandle asset,
                                              std::vector<std::uint8_t> bytes);

    /// Removes a cached reader, if present.
    void unregister_sequence(gw::assets::AssetHandle asset) noexcept;

    /// Returns nullptr when the asset is unknown.
    [[nodiscard]] GwseqReader* reader_for(gw::assets::AssetHandle asset) noexcept;

    /// Returns nullptr when the asset is unknown.
    [[nodiscard]] const GwseqReader* reader_for(gw::assets::AssetHandle asset) const noexcept;

    /// Drains pending event callbacks raised during the last `tick` call.
    void drain_event_callbacks(std::vector<std::uint32_t>& out) noexcept;

    /// Clears readers, pending events, and internal bookkeeping.
    void clear() noexcept;

    /// Records a cinematic/event callback id raised while sampling.
    void post_pending_event(std::uint32_t callback_id) noexcept;

private:
    friend struct SequencerSystem;

    std::unordered_map<std::uint64_t, GwseqReader> readers_{};
    std::vector<std::uint32_t>                     pending_events_{};
    gw::config::CVarRegistry*                      cvars_{nullptr};
};

/// Playback + track binding components (strict POD).
struct SeqPlayerComponent {
    gw::assets::AssetHandle seq_asset_id{};
    double                  play_head_frame = 0.0;
    bool                    playing         = false;
    bool                    loop            = false;
    float                   playback_rate   = 1.f;
};

struct SeqTransformTrackComponent {
    std::uint32_t track_id = 0;
    gw::ecs::Entity         target_entity = gw::ecs::Entity::null();
};

struct SeqEventTrackComponent {
    std::uint32_t track_id     = 0;
    std::uint32_t callback_id = 0;
};

struct SeqFloatTrackComponent {
    std::uint32_t track_id        = 0;
    std::uint32_t target_cvar_id = 0;
};

/// Host-side façade for advancing sequencers and applying sampled values.
struct SequencerSystem {
    /// Advances every `SeqPlayerComponent` on `world`, evaluates bound tracks,
    /// and mutates live ECS state (transforms, CVars, event queue).
    ///
    /// Threading: intended for the main thread after physics and before render
    /// (see `docs/06_ARCHITECTURE.md` execution graph).
    static void tick(SequencerWorld& tapes, gw::ecs::World& world,
                     gw::config::CVarRegistry* cvars, float dt) noexcept;
};

}  // namespace gw::seq
