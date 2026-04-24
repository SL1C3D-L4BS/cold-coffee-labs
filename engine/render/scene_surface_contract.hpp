#pragma once
// engine/render/scene_surface_contract.hpp — viewport ↔ runtime mesh/material identity (plan Track A viewport-runtime-parity).
//
// Both the editor viewport and the shipping scene pass must key draws from the
// same cooked asset identity (hash + material slot). This struct is the stable
// CPU-side token; GPU resources resolve through the shared cook / material DB.

#include <cstdint>

namespace gw::render {

struct SceneMeshDrawToken {
    std::uint64_t mesh_content_hash = 0; ///< Cook-time content hash (e.g. BLAKE3 lo word or stable id).
    std::uint32_t material_id       = 0; ///< Cooked material index / stable id.
    std::uint32_t lod_bias          = 0; ///< Authoring or streaming LOD hint.
};

} // namespace gw::render
