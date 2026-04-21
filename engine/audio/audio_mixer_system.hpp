#pragma once
// engine/audio/audio_mixer_system.hpp — runtime evaluator for the
// declarative ducking / muting rules in audio_mixing.toml (ADR-0019 §2.5).
//
// Rules evaluated each frame against a GameState snapshot. This type is a
// simple data-driven state machine: no codegen, no reflection — the rule
// parser is a minimal expression parser with four supported operators
// (<, >, <=, >=). It is intentionally boring.

#include "engine/audio/audio_service.hpp"

#include <string>
#include <vector>

namespace gw::audio {

// GameState is the union of inputs rules care about. Fields are looked up
// by name in conditions like "combat_intensity > 0.5".
struct GameState {
    float combat_intensity{0.0f};
    float atmospheric_density{1.0f};
    float player_health_pct{1.0f};
    bool  in_menu{false};
    bool  in_vehicle{false};
    bool  in_space{false};
};

enum class CondOp : uint8_t { Greater, Less, GreaterEq, LessEq, Eq };

struct MixingRule {
    std::string name;
    std::string field;     // GameState field name
    CondOp      op{CondOp::Greater};
    float       threshold{0.0f};

    BusId       bus{BusId::Master};
    float       delta_db{0.0f};
    bool        mute_when_active{false};
};

class AudioMixerSystem {
public:
    // Parse a subset of TOML that we care about for mixing rules.
    // Format per ADR-0019:
    //   [rule.<name>]
    //   when = "field op value"
    //   bus  = "master|sfx|..."
    //   delta_db = -6.0
    //   mute = false
    //
    // On a parse error, returns false and leaves `rules` empty. The parser
    // is permissive on whitespace but strict on structure — we are the
    // source of truth for the file, not a general-purpose TOML reader.
    static bool parse_toml(const std::string& source, std::vector<MixingRule>& rules);

    // Apply rules to an AudioService's mixer state given a GameState.
    // Ducking is cleared each frame by AudioService::update; this function
    // accumulates.
    static void apply(const std::vector<MixingRule>& rules,
                      const GameState& state,
                      AudioService& service);

    // Look up a bus by its canonical name.
    [[nodiscard]] static BusId bus_from_string(const std::string&) noexcept;
    [[nodiscard]] static std::string bus_to_string(BusId) noexcept;
};

} // namespace gw::audio
