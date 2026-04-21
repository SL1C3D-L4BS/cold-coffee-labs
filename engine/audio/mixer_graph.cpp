// engine/audio/mixer_graph.cpp
#include "engine/audio/mixer_graph.hpp"

#include <cmath>

namespace gw::audio {

// Canonical parent map — frozen topology from 19_AUDIO_DESIGN §5.
BusId MixerGraph::parent_of(BusId bus) noexcept {
    switch (bus) {
        case BusId::Master:             return BusId::Master;  // sentinel
        case BusId::SFX:                return BusId::Master;
        case BusId::SFX_Weapons:        return BusId::SFX;
        case BusId::SFX_Environment:    return BusId::SFX;
        case BusId::SFX_UI:             return BusId::SFX;
        case BusId::Music:              return BusId::Master;
        case BusId::Music_Combat:       return BusId::Music;
        case BusId::Music_Exploration:  return BusId::Music;
        case BusId::Music_Space:        return BusId::Music;
        case BusId::Dialogue:           return BusId::Master;
        case BusId::Dialogue_NPC:       return BusId::Dialogue;
        case BusId::Dialogue_Radio:     return BusId::Dialogue;
        case BusId::Ambience:           return BusId::Master;
        case BusId::Ambience_Wind:      return BusId::Ambience;
        case BusId::Ambience_Water:     return BusId::Ambience;
        case BusId::Ambience_Space:     return BusId::Ambience;
        default:                        return BusId::Master;
    }
}

bool MixerGraph::is_sfx_subtree(BusId bus) noexcept {
    switch (bus) {
        case BusId::SFX:
        case BusId::SFX_Weapons:
        case BusId::SFX_Environment:
        case BusId::SFX_UI:
        case BusId::Ambience:
        case BusId::Ambience_Wind:
        case BusId::Ambience_Water:
        case BusId::Ambience_Space:
            return true;
        default:
            return false;
    }
}

MixerGraph::MixerGraph() {
    for (uint16_t i = 0; i < kBusCount; ++i) {
        nodes_[i].id = static_cast<BusId>(i);
        nodes_[i].parent = parent_of(static_cast<BusId>(i));
        nodes_[i].volume = 1.0f;
        nodes_[i].muted = false;
        nodes_[i].reverb = ReverbPreset::None;
        nodes_[i].duck_db = 0.0f;
    }
    // Default per-bus volumes from ADR-0019 §2.5 canonical mixing:
    nodes_[static_cast<uint16_t>(BusId::Music)].volume = 0.8f;
    nodes_[static_cast<uint16_t>(BusId::Ambience)].volume = 0.8f;
}

void MixerGraph::set_bus_volume(BusId bus, float volume) noexcept {
    auto i = static_cast<uint16_t>(bus);
    if (i >= kBusCount) return;
    nodes_[i].volume = volume < 0.0f ? 0.0f : volume;
}

float MixerGraph::bus_volume(BusId bus) const noexcept {
    auto i = static_cast<uint16_t>(bus);
    if (i >= kBusCount) return 0.0f;
    return nodes_[i].volume;
}

void MixerGraph::set_bus_mute(BusId bus, bool muted) noexcept {
    auto i = static_cast<uint16_t>(bus);
    if (i >= kBusCount) return;
    nodes_[i].muted = muted;
}

bool MixerGraph::bus_muted(BusId bus) const noexcept {
    auto i = static_cast<uint16_t>(bus);
    if (i >= kBusCount) return true;
    return nodes_[i].muted;
}

void MixerGraph::set_reverb_preset(BusId bus, ReverbPreset preset) noexcept {
    auto i = static_cast<uint16_t>(bus);
    if (i >= kBusCount) return;
    nodes_[i].reverb = preset;
}

ReverbPreset MixerGraph::bus_reverb(BusId bus) const noexcept {
    auto i = static_cast<uint16_t>(bus);
    if (i >= kBusCount) return ReverbPreset::None;
    return nodes_[i].reverb;
}

void MixerGraph::apply_duck_db(BusId bus, float db) noexcept {
    auto i = static_cast<uint16_t>(bus);
    if (i >= kBusCount) return;
    nodes_[i].duck_db += db;
}

void MixerGraph::reset_ducking() noexcept {
    for (auto& n : nodes_) n.duck_db = 0.0f;
}

float MixerGraph::compute_effective_gain(BusId bus) const noexcept {
    if (in_vacuum_ && is_sfx_subtree(bus)) return 0.0f;

    float gain = 1.0f;
    BusId cur = bus;
    // Cycle-safety: Master's parent is itself; we break on that.
    for (int hops = 0; hops < kBusCount + 1; ++hops) {
        auto i = static_cast<uint16_t>(cur);
        if (i >= kBusCount) break;
        const auto& n = nodes_[i];
        if (n.muted) return 0.0f;
        gain *= n.volume;
        // Apply duck dB as linear gain multiplier.
        if (n.duck_db != 0.0f) {
            gain *= std::pow(10.0f, n.duck_db / 20.0f);
        }
        if (cur == BusId::Master) break;
        cur = n.parent;
    }
    return gain;
}

bool MixerGraph::topology_matches_canonical() const noexcept {
    for (uint16_t i = 0; i < kBusCount; ++i) {
        const auto& n = nodes_[i];
        if (n.id != static_cast<BusId>(i)) return false;
        if (n.parent != parent_of(static_cast<BusId>(i))) return false;
    }
    return true;
}

} // namespace gw::audio
