// tests/unit/world/circle_profile_test.cpp — Phase 21 W138 (ADR-0108).

#include "engine/world/circle_profile.hpp"

#include <doctest/doctest.h>

#include <cstring>

using gw::world::CircleId;
using gw::world::circle_profile;
using gw::world::circle_profiles;
using gw::world::location_skin_for;

TEST_CASE("circle_profiles: exactly nine canonical entries, ordered by CircleId") {
    const auto& profiles = circle_profiles();
    CHECK(profiles.size() == static_cast<std::size_t>(CircleId::COUNT));
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        CHECK(static_cast<std::size_t>(profiles[i].id) == i);
    }
}

TEST_CASE("location_skins: each Circle has a non-empty name/skybox/audio bed") {
    for (std::size_t i = 0; i < static_cast<std::size_t>(CircleId::COUNT); ++i) {
        const auto id   = static_cast<CircleId>(i);
        const auto& skin = location_skin_for(id);
        CHECK(!skin.name.empty());
        CHECK(!skin.skybox_ref.empty());
        CHECK(!skin.ambient_audio_bed.empty());
    }
}

TEST_CASE("location_skins: each Circle has a unique name") {
    const auto& profiles = circle_profiles();
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        for (std::size_t j = i + 1; j < profiles.size(); ++j) {
            const bool equal = profiles[i].location_skin.name ==
                               profiles[j].location_skin.name;
            CHECK_FALSE(equal);
        }
    }
}

TEST_CASE("circle_profile: lookup matches table ordering") {
    for (std::size_t i = 0; i < static_cast<std::size_t>(CircleId::COUNT); ++i) {
        const auto id       = static_cast<CircleId>(i);
        const auto& profile = circle_profile(id);
        CHECK(profile.id == id);
    }
}

TEST_CASE("location_skins: Violence Circle maps to Vatican Necropolis") {
    const auto& skin = location_skin_for(CircleId::Violence);
    // Avoid std::string_view::find in CHECK so doctest stringifier can format.
    const bool found = skin.name.find("Vatican") != decltype(skin.name)::npos;
    CHECK(found);
}
