#pragma once
// engine/gameai/blackboard.hpp — Phase 13 Wave 13E (ADR-0043 §3).
//
// Deterministic key-value store backing a BehaviorTree instance. Keys are
// short strings; values are a closed union of the 8 kinds defined in the
// .gwreplay v2 BTRE section (docs/determinism/phase13_replay_protocol.md).

#include "engine/gameai/gameai_types.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace gw::gameai {

enum class BBKind : std::uint8_t {
    Int32    = 0,
    Float    = 1,
    Double   = 2,
    Bool     = 3,
    Vec3     = 4,
    DVec3    = 5,
    Entity   = 6,
    StringId = 7,
};

struct BBValue {
    BBKind       kind{BBKind::Bool};
    std::int32_t i32{0};
    float        f32{0.0f};
    double       f64{0.0};
    bool         b{false};
    glm::vec3    v3{0.0f};
    glm::dvec3   dv3{0.0};
    EntityId     entity{kEntityNone};
    std::uint32_t string_id{0};
};

class Blackboard {
public:
    Blackboard() = default;

    // Set / get. `set` records a monotonically-increasing write count so the
    // replay writer can emit only the deltas since the last frame.
    void set(std::string_view key, const BBValue& v);

    [[nodiscard]] bool has(std::string_view key) const noexcept;
    [[nodiscard]] BBValue get(std::string_view key) const noexcept;

    // Typed convenience setters (used by vscript IR run-time).
    void set_int   (std::string_view k, std::int32_t v)   { set(k, BBValue{BBKind::Int32,    v}); }
    void set_float (std::string_view k, float v)          { set(k, BBValue{BBKind::Float,    0, v}); }
    void set_double(std::string_view k, double v)         { set(k, BBValue{BBKind::Double,   0, 0.0f, v}); }
    void set_bool  (std::string_view k, bool v)           { set(k, BBValue{BBKind::Bool,     0, 0.0f, 0.0, v}); }
    void set_vec3  (std::string_view k, const glm::vec3& v);
    void set_dvec3 (std::string_view k, const glm::dvec3& v);
    void set_entity(std::string_view k, EntityId e);
    void set_string_id(std::string_view k, std::uint32_t id);

    // Number of writes since last `reset_write_log`. Used to size replay
    // frames without scanning the whole map.
    [[nodiscard]] std::uint64_t write_count() const noexcept { return write_count_; }
    void reset_write_log() noexcept { write_count_ = 0; }

    // Iteration for determinism hash.
    [[nodiscard]] const std::unordered_map<std::string, BBValue>& entries() const noexcept { return map_; }

    void clear() noexcept { map_.clear(); write_count_ = 0; }

private:
    std::unordered_map<std::string, BBValue> map_{};
    std::uint64_t                            write_count_{0};
};

} // namespace gw::gameai
