#pragma once
// engine/render/mesh_asset_frame_pass.hpp
//
// Registers a graphics pass named **MeshAssetDraw** on the shared frame graph.
// This header intentionally avoids `frame_graph.hpp` / `types.hpp` so clangd
// does not require `volk.h` on the TU include path (see `frame_graph/handles.hpp`).

#include "engine/render/frame_graph/error.hpp"
#include "engine/render/frame_graph/handles.hpp"

namespace gw::render::frame_graph {
class FrameGraph;
}

namespace gw::render {

/// Declares the canonical opaque pass that consumes `MeshAsset` GPU buffers in
/// the frame graph ordering layer (ordering-only stub execute until a runtime
/// pass wires real draws).
[[nodiscard]] frame_graph::Result<frame_graph::PassHandle> register_mesh_asset_draw_pass(
    frame_graph::FrameGraph&            graph,
    frame_graph::ResourceHandle         color_target,
    frame_graph::ResourceHandle         depth_target);

}  // namespace gw::render
