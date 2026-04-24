#pragma once
// engine/net/gameplay_replay.hpp — compact director snapshot for bug repro (plan Track F3 / week 160).
//
// Uses `BitWriter` / `BitReader` so replay and net can share the same field order.

#include "engine/net/snapshot.hpp"
#include "engine/services/director/schema/director.hpp"

#include <cstdint>
#include <vector>

namespace gw::net::replay {

inline void write_director_request(BitWriter& w,
                                   const gw::services::director::DirectorRequest& r) {
    w.write_u64(r.seed);
    w.write_u64(r.logical_tick);
    w.write_f32(r.player_health_ratio);
    w.write_f32(r.recent_damage_taken);
    w.write_f32(r.recent_kps);
    w.write_f32(r.intensity_ewma);
    w.write_f32(r.normalized_sin);
    w.write_f32(r.normalized_grace);
    w.write_f32(r.time_in_state_s);
    w.write_u8(static_cast<std::uint8_t>(r.current_state));
}

[[nodiscard]] inline bool read_director_request(BitReader& br,
                                                gw::services::director::DirectorRequest& r) noexcept {
    r.seed              = static_cast<std::uint64_t>(br.read_u64());
    r.logical_tick      = static_cast<std::uint64_t>(br.read_u64());
    r.player_health_ratio  = br.read_f32();
    r.recent_damage_taken  = br.read_f32();
    r.recent_kps           = br.read_f32();
    r.intensity_ewma       = br.read_f32();
    r.normalized_sin       = br.read_f32();
    r.normalized_grace     = br.read_f32();
    r.time_in_state_s      = br.read_f32();
    r.current_state =
        static_cast<gw::services::director::IntensityState>(br.read_u8());
    return true;
}

[[nodiscard]] inline std::vector<std::uint8_t> pack_director_request(
    const gw::services::director::DirectorRequest& r) {
    BitWriter w;
    write_director_request(w, r);
    return std::vector<std::uint8_t>(w.view().begin(), w.view().end());
}

} // namespace gw::net::replay
