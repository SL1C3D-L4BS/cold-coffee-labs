#pragma once
// engine/assets/image_decode_rgba8.hpp — LDR RGBA8 decode for editor thumbnails / tools.
// stb_image implementation lives in cook/texture_cooker.cpp (same static library).

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace gw::assets {

struct DecodedImageRgba8 {
    std::vector<std::uint8_t> rgba;
    int                       width  = 0;
    int                       height = 0;
};

[[nodiscard]] std::optional<DecodedImageRgba8> decode_image_rgba8_from_file(
    const std::filesystem::path& path) noexcept;

} // namespace gw::assets
