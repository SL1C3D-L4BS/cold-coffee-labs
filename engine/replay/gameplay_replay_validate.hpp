#pragma once
// engine/replay/gameplay_replay_validate.hpp
// JSON-shaped gameplay replay validation (see bld/bld-replay gameplay_schema)
// for fuzzing and tools without pulling the Rust bincode path into C++.

#include <string_view>

namespace gw::replay {

/// Returns true when `json` contains a `header` object with `version` == 1
/// and a `frames` array whose objects' `tick` fields are strictly increasing.
/// Tolerant of whitespace; designed for LibFuzzer stability (no throws).
[[nodiscard]] bool validate_gameplay_replay_json_loose(std::string_view json) noexcept;

} // namespace gw::replay
