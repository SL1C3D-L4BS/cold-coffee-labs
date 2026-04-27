#pragma once
// engine/render/frame_graph/handles.hpp
//
// Opaque frame-graph handles without pulling Vulkan headers — keeps TU headers
// (e.g. mesh_asset_frame_pass.hpp) clangd-clean when compile_commands is missing
// FetchContent `-I` for volk.

#include <cstdint>

namespace gw::render::frame_graph {

using PassHandle       = std::uint32_t;
using ResourceHandle   = std::uint32_t;

constexpr PassHandle       INVALID_PASS_HANDLE       = ~0u;
constexpr ResourceHandle   INVALID_RESOURCE_HANDLE   = ~0u;

}  // namespace gw::render::frame_graph
