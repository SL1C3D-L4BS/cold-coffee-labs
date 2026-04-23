// engine/world/location_skins.cpp — Phase 21 W138 (ADR-0108).

#include "engine/world/circle_profile.hpp"

namespace gw::world {

namespace {

using streaming::BiomeId;

constexpr std::array<CircleProfile, static_cast<std::size_t>(CircleId::COUNT)> kProfiles{{
    // Violence — Vatican Necropolis: stone ossuary under holy light.
    CircleProfile{
        CircleId::Violence,
        LocationSkin{
            "Vatican Necropolis",
            "skybox/vatican_necropolis.cube",
            "audio/beds/vatican_necropolis.opus",
            {0.08f, 0.18f, 0.92f},
            FogParams{0.018f, 0.014f, {0.22f, 0.18f, 0.14f}},
            0.10f, 0.12f, 0.06f,
            0.15f, 0.00f, 0.00f,
        },
        FogParams{0.018f, 0.014f, {0.22f, 0.18f, 0.14f}},
        BiomeId::Arena, 0.0f, 52.0f,
    },
    // Heresy — Colosseum of Echoes: cold stone rings, bone-white tint.
    CircleProfile{
        CircleId::Heresy,
        LocationSkin{
            "Colosseum of Echoes",
            "skybox/colosseum_echoes.cube",
            "audio/beds/colosseum_echoes.opus",
            {0.55f, 0.08f, 0.88f},
            FogParams{0.012f, 0.020f, {0.18f, 0.20f, 0.22f}},
            0.08f, 0.10f, 0.04f,
            0.25f, 0.05f, 0.00f,
        },
        FogParams{0.012f, 0.020f, {0.18f, 0.20f, 0.22f}},
        BiomeId::HereticLibrary, -1.0f, 44.0f,
    },
    // Lust — Sewer of Grace: sickly green-tinted wet channels.
    CircleProfile{
        CircleId::Lust,
        LocationSkin{
            "Sewer of Grace",
            "skybox/sewer_of_grace.cube",
            "audio/beds/sewer_of_grace.opus",
            {0.28f, 0.45f, 0.70f},
            FogParams{0.030f, 0.010f, {0.10f, 0.20f, 0.14f}},
            0.14f, 0.18f, 0.10f,
            0.30f, 0.10f, 0.00f,
        },
        FogParams{0.030f, 0.010f, {0.10f, 0.20f, 0.14f}},
        BiomeId::FleshTunnel, -4.0f, 40.0f,
    },
    // Gluttony — Hell's Factory: industrial ember + smoke.
    CircleProfile{
        CircleId::Gluttony,
        LocationSkin{
            "Hell's Factory",
            "skybox/hells_factory.cube",
            "audio/beds/hells_factory.opus",
            {0.05f, 0.65f, 0.90f},
            FogParams{0.022f, 0.008f, {0.32f, 0.16f, 0.08f}},
            0.12f, 0.22f, 0.12f,
            0.20f, 0.10f, 0.00f,
        },
        FogParams{0.022f, 0.008f, {0.32f, 0.16f, 0.08f}},
        BiomeId::IndustrialForge, 0.0f, 56.0f,
    },
    // Greed — Rome Above: gilded palaces, warm amber haze.
    CircleProfile{
        CircleId::Greed,
        LocationSkin{
            "Rome Above",
            "skybox/rome_above.cube",
            "audio/beds/rome_above.opus",
            {0.11f, 0.40f, 0.95f},
            FogParams{0.009f, 0.022f, {0.42f, 0.30f, 0.18f}},
            0.06f, 0.08f, 0.02f,
            0.10f, 0.00f, 0.00f,
        },
        FogParams{0.009f, 0.022f, {0.42f, 0.30f, 0.18f}},
        BiomeId::LimboIsland, 4.0f, 48.0f,
    },
    // Wrath — Palazzo of the Unspoken: blood-red chapel architecture.
    CircleProfile{
        CircleId::Wrath,
        LocationSkin{
            "Palazzo of the Unspoken",
            "skybox/palazzo_unspoken.cube",
            "audio/beds/palazzo_unspoken.opus",
            {0.98f, 0.70f, 0.75f},
            FogParams{0.020f, 0.012f, {0.30f, 0.10f, 0.10f}},
            0.18f, 0.20f, 0.16f,
            0.40f, 0.05f, 0.00f,
        },
        FogParams{0.020f, 0.012f, {0.30f, 0.10f, 0.10f}},
        BiomeId::Battlefield, 0.0f, 46.0f,
    },
    // Pride — Open Arena: exposed sky, bright, Violence-adjacent.
    CircleProfile{
        CircleId::Pride,
        LocationSkin{
            "Open Arena",
            "skybox/open_arena.cube",
            "audio/beds/open_arena.opus",
            {0.60f, 0.10f, 0.98f},
            FogParams{0.005f, 0.030f, {0.75f, 0.74f, 0.72f}},
            0.04f, 0.06f, 0.02f,
            0.05f, 0.00f, 0.00f,
        },
        FogParams{0.005f, 0.030f, {0.75f, 0.74f, 0.72f}},
        BiomeId::Arena, 0.0f, 36.0f,
    },
    // Treachery — Heaven's Back Door: cold marble, absence-of-colour.
    CircleProfile{
        CircleId::Treachery,
        LocationSkin{
            "Heaven's Back Door",
            "skybox/heavens_back_door.cube",
            "audio/beds/heavens_back_door.opus",
            {0.58f, 0.05f, 0.70f},
            FogParams{0.014f, 0.028f, {0.14f, 0.18f, 0.22f}},
            0.10f, 0.08f, 0.06f,
            0.15f, 0.20f, 0.10f,
        },
        FogParams{0.014f, 0.028f, {0.14f, 0.18f, 0.22f}},
        BiomeId::FrozenCavern, -2.0f, 42.0f,
    },
    // Fraud — Silentium: mirror dimension, monochrome, airless.
    CircleProfile{
        CircleId::Fraud,
        LocationSkin{
            "Silentium",
            "skybox/silentium.cube",
            "audio/beds/silentium.opus",
            {0.00f, 0.00f, 0.60f},
            FogParams{0.004f, 0.050f, {0.08f, 0.08f, 0.09f}},
            0.05f, 0.10f, 0.20f,
            0.05f, 0.60f, 0.10f,
        },
        FogParams{0.004f, 0.050f, {0.08f, 0.08f, 0.09f}},
        BiomeId::Void, 0.0f, 28.0f,
    },
}};

} // namespace

const std::array<CircleProfile, static_cast<std::size_t>(CircleId::COUNT)>& circle_profiles() noexcept {
    return kProfiles;
}

const CircleProfile& circle_profile(CircleId id) noexcept {
    return kProfiles[static_cast<std::size_t>(id)];
}

const LocationSkin& location_skin_for(CircleId id) noexcept {
    return circle_profile(id).location_skin;
}

} // namespace gw::world
