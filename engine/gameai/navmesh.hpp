#pragma once
// engine/gameai/navmesh.hpp — Phase 13 Wave 13E (ADR-0044).
//
// Grid-tiled navmesh with a Recast-compatible tile coordinate scheme.
// Query API is what callers see; the null backend uses a tiny grid-based
// Dijkstra. The Recast/Detour backend (`GW_AI_RECAST`) plugs in via link-
// time substitution; API is identical.

#include "engine/gameai/gameai_types.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace gw::gameai {

// -----------------------------------------------------------------------------
// Tile coordinate (x, z) in the world's navmesh grid.
// -----------------------------------------------------------------------------
struct TileCoord {
    std::int32_t x{kInvalidTile};
    std::int32_t z{kInvalidTile};
    [[nodiscard]] constexpr bool valid() const noexcept {
        return x != kInvalidTile && z != kInvalidTile;
    }
    [[nodiscard]] constexpr bool operator==(const TileCoord& rhs) const noexcept {
        return x == rhs.x && z == rhs.z;
    }
};

// -----------------------------------------------------------------------------
// Navmesh description. Tile size, cell size, agent shape. The `walkable`
// span is a row-major grid of (tiles_x * cells_per_tile) × (tiles_z * cells_per_tile)
// bools — true where an agent can stand. The null backend treats each
// walkable cell as a navigable polygon.
// -----------------------------------------------------------------------------
struct NavmeshDesc {
    std::string   name{};
    std::int32_t  tile_count_x{1};
    std::int32_t  tile_count_z{1};
    float         tile_size_m{32.0f};       // per-tile, ADR-0044 §3 default
    float         cell_size_m{0.3f};
    float         agent_radius_m{0.3f};
    float         agent_height_m{1.8f};
    float         agent_max_climb_m{0.4f};
    float         agent_max_slope_deg{45.0f};
    glm::vec3     origin_ws{0.0f};          // world-space origin of tile (0,0)
    std::vector<std::uint8_t> walkable{};   // row-major walkability grid
};

struct NavmeshInfo {
    std::int32_t tile_count_x{0};
    std::int32_t tile_count_z{0};
    std::uint64_t content_hash{0};
    std::uint32_t walkable_cells{0};
};

// -----------------------------------------------------------------------------
// Path-query I/O.
// -----------------------------------------------------------------------------
enum class PathStatus : std::uint8_t {
    Ok                 = 0,
    NoPath             = 1,
    PartialPath        = 2,
    StartNotOnMesh     = 3,
    EndNotOnMesh       = 4,
    NavmeshNotLoaded   = 5,
};

struct PathQueryInput {
    NavmeshHandle navmesh{};
    EntityId      entity{kEntityNone};
    glm::vec3     start_ws{0.0f};
    glm::vec3     end_ws{0.0f};
    float         max_length_m{500.0f};
};

struct PathQueryOutput {
    PathStatus             status{PathStatus::NoPath};
    std::vector<glm::vec3> waypoints{};     // straight-line corners
    float                  length_m{0.0f};
    std::uint64_t          result_hash{0};  // FNV-1a-64 of the result
};

// Deterministic content hash — FNV-1a-64 over (tile layout + walkable grid).
[[nodiscard]] std::uint64_t navmesh_content_hash(const NavmeshDesc& desc) noexcept;

} // namespace gw::gameai
