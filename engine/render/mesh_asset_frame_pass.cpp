// No include of mesh_asset_frame_pass.hpp: this TU's definition is the first
// declaration here; callers/tests include the .hpp. Satisfies clangd include-cleaner.
#include "engine/render/frame_graph/frame_graph.hpp"

namespace gw::render {

frame_graph::Result<frame_graph::PassHandle> register_mesh_asset_draw_pass(
    frame_graph::FrameGraph&            graph,
    frame_graph::ResourceHandle         color_target,
    frame_graph::ResourceHandle         depth_target) {
    frame_graph::PassDesc pass{"MeshAssetDraw"};
    // Writes both attachments so the graph does not require a synthetic depth
    // producer when only this pass is registered (tests); real pipelines add
    // depth-prep / G-buffer passes ahead of this slot.
    pass.writes.push_back(color_target);
    pass.writes.push_back(depth_target);
    pass.execute = [](frame_graph::CommandBuffer& /*cmd*/) {
        // Ordering + barrier contract only; `editor/render/editor_scene_pass.cpp`
        // records real `MeshAsset` draws for the editor viewport today.
    };
    return graph.add_pass(std::move(pass));
}

}  // namespace gw::render
