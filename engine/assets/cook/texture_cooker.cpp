// engine/assets/cook/texture_cooker.cpp
// TextureCooker: source image → UASTC KTX2 → .gwtex
//
// Pipeline:
//  [1] Read source bytes + compute source hash
//  [2] Decode image via stb_image (LDR) or tinyexr (HDR)
//  [3] Mip-chain generation with Kaiser filter (via basisu)
//  [4] Encode to UASTC (q0 for debug, q2 for release)
//  [5] Pack KTX2 container
//  [6] Write .gwtex = GwTexHeader + KTX2 bytes

#include "texture_cooker.hpp"
#include "../texture_asset.hpp"
#include "cook_manifest.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

// basis_universal encoder
#include <basisu_comp.h>
#include <basisu_transcoder.h>

// stb_image for LDR sources (header-only, implementation in this TU)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

// xxHash for content addressing
#include <xxhash.h>

// VkFormat constants we need (not including full vulkan.h here).
// Only the subset we use for texture encoding decisions.
namespace vkfmt {
    static constexpr uint32_t BC7_UNORM  = 145;
    static constexpr uint32_t BC7_SRGB   = 146;
    static constexpr uint32_t BC5_UNORM  = 143;
    static constexpr uint32_t BC6H_UFLOAT= 148;
    static constexpr uint32_t BC1_UNORM  = 131;
}

namespace gw::assets::cook {

static constexpr uint32_t kRuleVersion = rule_version::kTexture;
uint32_t TextureCooker::rule_version() const { return kRuleVersion; }

// Determine VkFormat from filename convention:
//   *_n.png / *_normal.png → BC5 (RG, linear normal map)
//   *_orm.png              → BC7 linear
//   *_hdr.exr / *.hdr      → BC6H
//   else                   → BC7 sRGB or linear
static uint32_t infer_vk_format(const std::filesystem::path& path, bool& out_srgb) {
    const std::string stem = path.stem().string();
    const std::string ext  = path.extension().string();

    auto ends_with = [&](const std::string& s) {
        return stem.size() >= s.size() &&
               stem.substr(stem.size() - s.size()) == s;
    };

    if (ext == ".hdr" || ext == ".exr") { out_srgb = false; return vkfmt::BC6H_UFLOAT; }
    if (ends_with("_n") || ends_with("_normal")) { out_srgb = false; return vkfmt::BC5_UNORM; }
    if (ends_with("_orm") || ends_with("_rough") || ends_with("_metal")) {
        out_srgb = false; return vkfmt::BC7_UNORM;
    }
    out_srgb = true;
    return vkfmt::BC7_SRGB;
}

AssetResult<CookResult> TextureCooker::cook(const CookContext& ctx) const {
    const auto t0 = std::chrono::steady_clock::now();

    // ------------------------------------------------------------------
    // 1. Read source bytes
    // ------------------------------------------------------------------
    std::ifstream f(ctx.source_path, std::ios::binary | std::ios::ate);
    if (!f) return std::unexpected(AssetError{AssetErrorCode::FileNotFound,
                                               ctx.source_path.string().c_str()});
    const auto src_size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> src_bytes(src_size);
    f.read(reinterpret_cast<char*>(src_bytes.data()),
           static_cast<std::streamsize>(src_size));

    XXH128_hash_t src_xxh = XXH3_128bits(src_bytes.data(), src_bytes.size());
    CookKey source_hash{src_xxh.high64, src_xxh.low64};

    // Cook key
    XXH3_state_t* xstate = XXH3_createState();
    XXH3_128bits_reset(xstate);
    XXH3_128bits_update(xstate, src_bytes.data(), src_bytes.size());
    XXH3_128bits_update(xstate, &kRuleVersion, sizeof(kRuleVersion));
    const uint8_t plat = static_cast<uint8_t>(ctx.platform);
    const uint8_t cfg  = static_cast<uint8_t>(ctx.config);
    XXH3_128bits_update(xstate, &plat, 1);
    XXH3_128bits_update(xstate, &cfg,  1);
    XXH128_hash_t ck_xxh = XXH3_128bits_digest(xstate);
    XXH3_freeState(xstate);
    CookKey cook_key{ck_xxh.high64, ck_xxh.low64};

    // ------------------------------------------------------------------
    // 2. Decode image
    // ------------------------------------------------------------------
    int w = 0, h = 0, channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(
        src_bytes.data(), static_cast<int>(src_bytes.size()),
        &w, &h, &channels, 4); // force RGBA
    if (!pixels) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "stbi decode failed"});
    }

    bool is_srgb = false;
    const uint32_t vk_format = infer_vk_format(ctx.source_path, is_srgb);

    // ------------------------------------------------------------------
    // 3-5. basis_universal encode → KTX2
    // ------------------------------------------------------------------
    basisu::basis_compressor_params params{};
    params.m_source_images.resize(1);
    params.m_source_images[0].init(pixels, static_cast<uint32_t>(w),
                                   static_cast<uint32_t>(h), 4);
    stbi_image_free(pixels);

    params.m_uastc             = true;
    params.m_pack_uastc_flags  = (ctx.config == CookConfig::Release)
                                    ? basisu::cPackUASTCLevelDefault : basisu::cPackUASTCLevelFastest;
    params.m_perceptual        = false; // deterministic: disable perceptual
    params.m_mip_gen           = true;
    params.m_mip_filter        = "kaiser";
    params.m_mip_smallest_dimension = 4;
    params.m_create_ktx2_file  = true;
    params.m_ktx2_uastc_supercompression = basist::KTX2_SS_ZSTANDARD;
    params.m_ktx2_srgb_transfer_func = is_srgb;
    params.m_no_selector_rdo   = true;  // deterministic
    params.m_no_endpoint_rdo   = true;  // deterministic

    basisu::job_pool jpool(1);
    params.m_pJob_pool = &jpool;

    basisu::basis_compressor compressor;
    if (!compressor.init(params)) {
        return std::unexpected(AssetError{AssetErrorCode::CookFailed,
                                          "basisu init failed"});
    }

    const auto process_result = compressor.process();
    if (process_result != basisu::basis_compressor::cECSuccess) {
        return std::unexpected(AssetError{AssetErrorCode::CookFailed,
                                          "basisu encode failed"});
    }

    const auto& ktx2_buf = compressor.get_output_ktx2_file();

    // ------------------------------------------------------------------
    // 6. Write .gwtex = GwTexHeader + KTX2 bytes
    // ------------------------------------------------------------------
    GwTexHeader hdr{};
    hdr.magic        = magic::kTexture;
    hdr.version      = 1;
    hdr.tex_type     = 0; // 2D
    hdr.channels     = 4;
    hdr.width        = static_cast<uint16_t>(w);
    hdr.height       = static_cast<uint16_t>(h);
    hdr.depth_layers = 1;
    // Compute mip count from image dimensions (matches basisu's mip chain).
    {
        uint32_t dim = static_cast<uint32_t>(std::max(w, h));
        uint8_t mips = 1;
        while (dim > 4) { dim >>= 1; ++mips; }
        hdr.mip_count = mips;
    }
    hdr.vk_format    = vk_format;
    hdr.color_space  = is_srgb ? 1 : 0;
    hdr.has_alpha    = (channels == 4) ? 1 : 0;
    hdr.ktx2_size    = static_cast<uint32_t>(ktx2_buf.size());
    hdr.ktx2_offset  = static_cast<uint32_t>(sizeof(GwTexHeader));

    std::filesystem::create_directories(ctx.output_path.parent_path());
    std::ofstream of(ctx.output_path, std::ios::binary | std::ios::trunc);
    if (!of) return std::unexpected(AssetError{AssetErrorCode::CookFailed,
                                                ctx.output_path.string().c_str()});
    of.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    of.write(reinterpret_cast<const char*>(ktx2_buf.data()),
             static_cast<std::streamsize>(ktx2_buf.size()));

    const auto t1 = std::chrono::steady_clock::now();
    const uint32_t ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    CookResult result{};
    result.cook_key     = cook_key;
    result.cook_time_ms = ms;
    return result;
}

} // namespace gw::assets::cook
