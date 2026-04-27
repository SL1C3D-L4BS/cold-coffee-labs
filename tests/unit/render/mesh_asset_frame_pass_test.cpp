// tests/unit/render/mesh_asset_frame_pass_test.cpp — MeshAsset pass slot on frame graph.
#include <doctest/doctest.h>

#include "engine/render/mesh_asset_frame_pass.hpp"
#include "engine/render/frame_graph/frame_graph.hpp"
#include "engine/render/frame_graph/types.hpp"

using namespace gw::render::frame_graph;

TEST_CASE("mesh_asset_frame_pass — registers MeshAssetDraw between depth and color") {
    FrameGraph fg;

    TextureDesc color{};
    color.width  = 800;
    color.height = 600;
    color.format = VK_FORMAT_R8G8B8A8_UNORM;
    color.name   = "color_rt";
    auto color_h = fg.declare_texture(color);
    REQUIRE(color_h.has_value());

    TextureDesc depth{};
    depth.width  = 800;
    depth.height = 600;
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.name   = "depth_rt";
    auto depth_h = fg.declare_texture(depth);
    REQUIRE(depth_h.has_value());

    auto mesh = gw::render::register_mesh_asset_draw_pass(fg, color_h.value(), depth_h.value());
    REQUIRE(mesh.has_value());

    REQUIRE(fg.compile().has_value());
    CHECK(fg.execution_order().size() == 1u);
}
