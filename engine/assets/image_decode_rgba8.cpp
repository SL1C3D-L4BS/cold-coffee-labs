// engine/assets/image_decode_rgba8.cpp

#include "engine/assets/image_decode_rgba8.hpp"

#include <fstream>
#include <vector>

// Declarations only — STB_IMAGE_IMPLEMENTATION is in cook/texture_cooker.cpp.
#include <stb_image.h>

namespace gw::assets {

std::optional<DecodedImageRgba8> decode_image_rgba8_from_file(
    const std::filesystem::path& path) noexcept {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.good()) return std::nullopt;
    const auto sz = static_cast<std::size_t>(f.tellg());
    if (sz == 0) return std::nullopt;
    f.seekg(0);
    std::vector<unsigned char> bytes(sz);
    f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(sz));
    if (!f.good()) return std::nullopt;

    int w = 0, h = 0, ch = 0;
    stbi_uc* pix = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()),
                                         &w, &h, &ch, STBI_rgb_alpha);
    if (!pix || w <= 0 || h <= 0) {
        if (pix) stbi_image_free(pix);
        return std::nullopt;
    }
    DecodedImageRgba8 out{};
    out.width  = w;
    out.height = h;
    const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u;
    out.rgba.assign(pix, pix + n);
    stbi_image_free(pix);
    return out;
}

} // namespace gw::assets
