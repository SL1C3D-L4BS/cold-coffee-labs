#pragma once
// engine/world/circle_profile.hpp — Phase 21 W138 (ADR-0108, ADR-0090).
//
// Per-Circle profile: drives GPTM terrain tuning, Circle post-stack
// baselines, and the nine Location Skins (Story Bible §3).
//
// Each of the Sacrilege Nine Circles maps to exactly one `CircleId` and one
// default `LocationSkin`. Editor authoring for Circle/Encounter panels
// writes directly into the `CircleProfile` registry.

#include "engine/world/streaming/chunk_data.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace gw::world {

enum class CircleId : std::uint8_t {
    Violence  = 0, // Vatican Necropolis
    Heresy    = 1, // Colosseum of Echoes
    Lust      = 2, // Sewer of Grace
    Gluttony  = 3, // Hell's Factory
    Greed     = 4, // Rome Above
    Wrath     = 5, // Palazzo of the Unspoken
    Pride     = 6, // Open Arena
    Treachery = 7, // Heaven's Back Door
    Fraud     = 8, // Silentium
    COUNT     = 9,
};

struct FogParams {
    float density{0.01f};
    float height_falloff{0.015f};
    float color_rgb[3]{0.12f, 0.10f, 0.09f};
};

struct LocationSkin {
    std::string_view name{};
    std::string_view skybox_ref{};
    std::string_view ambient_audio_bed{};
    float            ground_tint_hsv[3]{0.0f, 0.0f, 1.0f};
    FogParams        fog{};
    // Baseline CVar overrides per Circle for the horror / Circle post stack.
    float            horror_chromatic_aberration{0.0f};
    float            horror_film_grain{0.05f};
    float            horror_screen_shake{0.0f};
    float            sin_tendril_baseline{0.0f};
    float            ruin_desat_baseline{0.0f};
    float            rapture_whiteout_baseline{0.0f};
};

struct CircleProfile {
    CircleId     id{CircleId::Violence};
    LocationSkin location_skin{};
    FogParams    fog_params{};
    // Link to the SDR/biome-id band this Circle owns (Phase 19-B).
    streaming::BiomeId biome_id{streaming::BiomeId::Arena};
    float            height_bias_m{0.0f};
    float            height_scale_m{48.0f};
};

/// Returns the nine canonical Circle profiles in CircleId order.
[[nodiscard]] const std::array<CircleProfile, static_cast<std::size_t>(CircleId::COUNT)>&
circle_profiles() noexcept;

/// Convenience lookup.
[[nodiscard]] const CircleProfile& circle_profile(CircleId id) noexcept;

/// Returns the Story-Bible Location Skin for a Circle. Equivalent to
/// `circle_profile(id).location_skin` but convenient for editors/tests.
[[nodiscard]] const LocationSkin& location_skin_for(CircleId id) noexcept;

} // namespace gw::world
