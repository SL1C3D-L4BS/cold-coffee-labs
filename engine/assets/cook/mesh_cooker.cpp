// engine/assets/cook/mesh_cooker.cpp
// MeshCooker: glTF 2.0 → .gwmesh
//
// Pipeline:
//  [1] fastgltf parse       → in-memory document
//  [2] Primitive extraction → positions, normals, UVs, joints, weights
//  [3] MikkTSpace tangents  → unindexed triangles → tangent + bitangent sign
//  [4] meshopt weld/reindex → meshopt_generateVertexRemap
//  [5] meshopt cache opt    → vertex cache + overdraw + vertex fetch
//  [6] LOD chain            → meshopt_simplify × 4 levels
//  [7] Quantization         → positions int16, normals/tangents oct-encoded,
//                             UVs unorm16
//  [8] .gwmesh serialise    → flat binary, mmap-ready

#include "mesh_cooker.hpp"
#include "../mesh_asset.hpp"
#include "cook_manifest.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

// fastgltf
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

// MikkTSpace
#include <mikktspace.h>

// meshoptimizer
#include <meshoptimizer.h>

// xxHash for cook-key computation
#include <xxhash.h>

namespace gw::assets::cook {

// ---- Rule version (bump when cook transform logic changes) ----------------
static constexpr uint32_t kRuleVersion = rule_version::kMesh;
uint32_t MeshCooker::rule_version() const { return kRuleVersion; }

// ---- Intermediate mesh data -----------------------------------------------

struct RawVertex {
    float pos[3];
    float normal[3];
    float tangent[4]; // xyz = tangent direction, w = sign
    float uv0[2];
    float uv1[2];
    uint8_t color[4]{255, 255, 255, 255};
};

struct CookedVertex {
    int16_t  pos[3];   // quantized
    int8_t   n[2];     // oct-encoded normal
    int8_t   t[2];     // oct-encoded tangent (sign in t[1] sign bit encoded separately)
    uint16_t uv0[2];   // unorm16
    uint16_t uv1[2];
    uint8_t  color[4];
    int16_t  pad;      // align stream1 to 16 bytes
};

// ---- Oct encoding ---------------------------------------------------------
static void oct_encode(float x, float y, float z, int8_t& ox, int8_t& oy) {
    const float l = std::abs(x) + std::abs(y) + std::abs(z);
    float px = x / l;
    float py = y / l;
    if (z <= 0.0f) {
        float nx = (1.0f - std::abs(py)) * (px >= 0.0f ? 1.0f : -1.0f);
        float ny = (1.0f - std::abs(px)) * (py >= 0.0f ? 1.0f : -1.0f);
        px = nx; py = ny;
    }
    ox = static_cast<int8_t>(std::clamp(std::round(px * 127.0f), -127.0f, 127.0f));
    oy = static_cast<int8_t>(std::clamp(std::round(py * 127.0f), -127.0f, 127.0f));
}

// ---- Position quantization ------------------------------------------------
struct QuantParams {
    float scale[3];
    float bias[3];
};

static QuantParams compute_quant(const float* aabb_min, const float* aabb_max) {
    QuantParams q{};
    for (int i = 0; i < 3; ++i) {
        q.scale[i] = (aabb_max[i] - aabb_min[i]) / 32767.0f;
        q.bias[i]  = aabb_min[i];
    }
    return q;
}

static int16_t quantize_pos(float v, float scale, float bias) {
    if (scale < 1e-9f) return 0;
    const float q = (v - bias) / scale;
    return static_cast<int16_t>(std::clamp(std::round(q), -32767.0f, 32767.0f));
}

// ---- MikkTSpace context ---------------------------------------------------
struct MikkContext {
    std::vector<RawVertex>& verts;
    std::vector<uint32_t>&  indices;
};

static int mikk_get_num_faces(const SMikkTSpaceContext* c) {
    auto* ctx = static_cast<MikkContext*>(c->m_pUserData);
    return static_cast<int>(ctx->indices.size() / 3);
}
static int mikk_get_num_verts(const SMikkTSpaceContext*, int) { return 3; }
static void mikk_get_pos(const SMikkTSpaceContext* c, float p[3], int fi, int vi) {
    auto* ctx = static_cast<MikkContext*>(c->m_pUserData);
    const auto& v = ctx->verts[ctx->indices[fi * 3 + vi]];
    p[0] = v.pos[0]; p[1] = v.pos[1]; p[2] = v.pos[2];
}
static void mikk_get_normal(const SMikkTSpaceContext* c, float n[3], int fi, int vi) {
    auto* ctx = static_cast<MikkContext*>(c->m_pUserData);
    const auto& v = ctx->verts[ctx->indices[fi * 3 + vi]];
    n[0] = v.normal[0]; n[1] = v.normal[1]; n[2] = v.normal[2];
}
static void mikk_get_uv(const SMikkTSpaceContext* c, float uv[2], int fi, int vi) {
    auto* ctx = static_cast<MikkContext*>(c->m_pUserData);
    const auto& v = ctx->verts[ctx->indices[fi * 3 + vi]];
    uv[0] = v.uv0[0]; uv[1] = v.uv0[1];
}
static void mikk_set_tangent(const SMikkTSpaceContext* c,
                              const float t[3], float sign,
                              int fi, int vi)
{
    auto* ctx = static_cast<MikkContext*>(c->m_pUserData);
    auto& v = ctx->verts[ctx->indices[fi * 3 + vi]];
    v.tangent[0] = t[0]; v.tangent[1] = t[1];
    v.tangent[2] = t[2]; v.tangent[3] = sign;
}

// ---- Write helper ---------------------------------------------------------
static void write_u8 (std::vector<uint8_t>& buf, uint8_t  v) { buf.push_back(v); }
static void write_u16(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v));
    buf.push_back(static_cast<uint8_t>(v >> 8));
}
static void write_u32(std::vector<uint8_t>& buf, uint32_t v) {
    for (int i = 0; i < 4; ++i) buf.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
static void write_f32(std::vector<uint8_t>& buf, float v) {
    uint32_t tmp;
    std::memcpy(&tmp, &v, 4);
    write_u32(buf, tmp);
}
template<typename T>
static void write_bytes(std::vector<uint8_t>& buf, const T* data, std::size_t n) {
    const auto* p = reinterpret_cast<const uint8_t*>(data);
    buf.insert(buf.end(), p, p + n);
}
static void pad_to(std::vector<uint8_t>& buf, std::size_t align) {
    while (buf.size() % align) buf.push_back(0);
}

// ---- Main cook implementation ---------------------------------------------

AssetResult<CookResult> MeshCooker::cook(const CookContext& ctx) const {
    const auto t0 = std::chrono::steady_clock::now();

    // ------------------------------------------------------------------
    // 1. Read source bytes and compute source hash
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
    CookKey source_hash{ src_xxh.high64, src_xxh.low64 };

    // Cook key = hash(source_bytes + rule_version + platform + config)
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
    CookKey cook_key{ ck_xxh.high64, ck_xxh.low64 };

    // ------------------------------------------------------------------
    // 2. fastgltf parse
    // ------------------------------------------------------------------
    fastgltf::Parser parser;
    auto buf_expected = fastgltf::GltfDataBuffer::FromBytes(
        reinterpret_cast<const std::byte*>(src_bytes.data()), src_bytes.size());
    if (!buf_expected) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "fastgltf buffer create failed"});
    }
    auto& data_buf = buf_expected.get();

    const auto ext = ctx.source_path.extension().string();
    constexpr auto gltf_opts =
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages;

    auto expected_asset = (ext == ".glb")
        ? parser.loadGltfBinary(data_buf, ctx.source_path.parent_path(), gltf_opts)
        : parser.loadGltf(data_buf, ctx.source_path.parent_path(), gltf_opts);

    if (expected_asset.error() != fastgltf::Error::None) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "fastgltf parse failed"});
    }
    const auto& asset = expected_asset.get();

    // ------------------------------------------------------------------
    // 3. Collect all primitives into raw vertex / index lists
    // ------------------------------------------------------------------
    std::vector<RawVertex> all_verts;
    std::vector<uint32_t>  all_indices;

    // AABB tracking
    float aabb_min[3]{ 1e30f, 1e30f, 1e30f };
    float aabb_max[3]{-1e30f,-1e30f,-1e30f };

    for (const auto& mesh : asset.meshes) {
        for (const auto& prim : mesh.primitives) {
            if (prim.type != fastgltf::PrimitiveType::Triangles) continue;

            // Positions (required)
            const auto* pos_attr = prim.findAttribute("POSITION");
            if (!pos_attr) continue;
            const auto& pos_acc = asset.accessors[pos_attr->accessorIndex];

            const std::size_t vert_count = pos_acc.count;
            const std::size_t base_idx   = all_verts.size();
            all_verts.resize(base_idx + vert_count);

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset, pos_acc,
                [&](fastgltf::math::fvec3 p, std::size_t i) {
                    all_verts[base_idx + i].pos[0] = p.x();
                    all_verts[base_idx + i].pos[1] = p.y();
                    all_verts[base_idx + i].pos[2] = p.z();
                    for (int k = 0; k < 3; ++k) {
                        aabb_min[k] = std::min(aabb_min[k], p[k]);
                        aabb_max[k] = std::max(aabb_max[k], p[k]);
                    }
                });

            // Normals (optional — generate flat if missing)
            const auto* norm_attr = prim.findAttribute("NORMAL");
            if (norm_attr) {
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, asset.accessors[norm_attr->accessorIndex],
                    [&](fastgltf::math::fvec3 n, std::size_t i) {
                        all_verts[base_idx + i].normal[0] = n.x();
                        all_verts[base_idx + i].normal[1] = n.y();
                        all_verts[base_idx + i].normal[2] = n.z();
                    });
            } else {
                for (std::size_t i = 0; i < vert_count; ++i)
                    all_verts[base_idx + i].normal[1] = 1.0f; // Y-up default
            }

            // UVs
            const auto* uv0_attr = prim.findAttribute("TEXCOORD_0");
            if (uv0_attr) {
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                    asset, asset.accessors[uv0_attr->accessorIndex],
                    [&](fastgltf::math::fvec2 uv, std::size_t i) {
                        all_verts[base_idx + i].uv0[0] = uv.x();
                        all_verts[base_idx + i].uv0[1] = uv.y();
                    });
            }

            // Indices
            const std::size_t idx_base = all_indices.size();
            if (prim.indicesAccessor.has_value()) {
                const auto& idx_acc = asset.accessors[*prim.indicesAccessor];
                all_indices.resize(idx_base + idx_acc.count);
                fastgltf::iterateAccessorWithIndex<uint32_t>(
                    asset, idx_acc,
                    [&](uint32_t idx, std::size_t ii) {
                        all_indices[idx_base + ii] =
                            static_cast<uint32_t>(base_idx) + idx;
                    });
            } else {
                // Non-indexed: generate sequential
                for (std::size_t i = 0; i < vert_count; ++i)
                    all_indices.push_back(static_cast<uint32_t>(base_idx + i));
            }
        }
    }

    if (all_verts.empty() || all_indices.empty()) {
        return std::unexpected(AssetError{AssetErrorCode::CorruptData,
                                          "no triangle primitives in glTF"});
    }

    // ------------------------------------------------------------------
    // 4. MikkTSpace tangents (generate before reindexing)
    // ------------------------------------------------------------------
    MikkContext mikk_ctx{all_verts, all_indices};
    SMikkTSpaceInterface mikk_iface{};
    mikk_iface.m_getNumFaces          = mikk_get_num_faces;
    mikk_iface.m_getNumVerticesOfFace = mikk_get_num_verts;
    mikk_iface.m_getPosition          = mikk_get_pos;
    mikk_iface.m_getNormal            = mikk_get_normal;
    mikk_iface.m_getTexCoord          = mikk_get_uv;
    mikk_iface.m_setTSpaceBasic       = mikk_set_tangent;

    SMikkTSpaceContext mikk_ctx_s{};
    mikk_ctx_s.m_pInterface = &mikk_iface;
    mikk_ctx_s.m_pUserData  = &mikk_ctx;
    genTangSpaceDefault(&mikk_ctx_s);

    // ------------------------------------------------------------------
    // 5. meshoptimizer: weld + optimize
    // ------------------------------------------------------------------
    std::vector<uint32_t> remap(all_indices.size());
    const std::size_t unique_count = meshopt_generateVertexRemap(
        remap.data(), all_indices.data(), all_indices.size(),
        all_verts.data(), all_verts.size(), sizeof(RawVertex));

    std::vector<RawVertex>  opt_verts(unique_count);
    std::vector<uint32_t>   opt_indices(all_indices.size());
    meshopt_remapVertexBuffer(opt_verts.data(), all_verts.data(),
                               all_verts.size(), sizeof(RawVertex), remap.data());
    meshopt_remapIndexBuffer(opt_indices.data(), all_indices.data(),
                              all_indices.size(), remap.data());

    meshopt_optimizeVertexCache(opt_indices.data(), opt_indices.data(),
                                 opt_indices.size(), unique_count);
    meshopt_optimizeOverdraw(opt_indices.data(), opt_indices.data(),
                              opt_indices.size(),
                              &opt_verts[0].pos[0], unique_count, sizeof(RawVertex), 1.05f);
    meshopt_optimizeVertexFetch(opt_verts.data(), opt_indices.data(),
                                 opt_indices.size(), opt_verts.data(),
                                 unique_count, sizeof(RawVertex));

    // ------------------------------------------------------------------
    // 6. LOD chain (4 levels: 100%, 50%, 25%, 12%)
    // ------------------------------------------------------------------
    constexpr std::size_t kNumLODs = 4;
    constexpr float lod_ratios[kNumLODs] = {1.0f, 0.5f, 0.25f, 0.12f};
    std::array<std::vector<uint32_t>, kNumLODs> lod_indices;
    lod_indices[0] = opt_indices;

    for (std::size_t lod = 1; lod < kNumLODs; ++lod) {
        const std::size_t target = static_cast<std::size_t>(
            opt_indices.size() * lod_ratios[lod]);
        // Minimum: at least 1 triangle (3 indices).
        const std::size_t clamped = std::max(target, std::size_t(3));
        lod_indices[lod].resize(opt_indices.size());
        float lod_err = 0.0f;
        const std::size_t result_count = meshopt_simplify(
            lod_indices[lod].data(),
            opt_indices.data(), opt_indices.size(),
            &opt_verts[0].pos[0], unique_count, sizeof(RawVertex),
            clamped, 0.01f, 0, &lod_err);
        lod_indices[lod].resize(result_count);
    }

    // ------------------------------------------------------------------
    // 7. Quantize and pack cooked vertices
    // ------------------------------------------------------------------
    const QuantParams qp = compute_quant(aabb_min, aabb_max);

    // Stream 0: position only (int16[3] + int16 pad = 8 bytes/vert)
    struct S0 { int16_t pos[3]; int16_t pad; };
    std::vector<S0> stream0(unique_count);

    // Stream 1: attributes (2+2+4+4+4 bytes = 16 bytes/vert)
    struct S1 { int8_t n[2]; int8_t t[2]; uint16_t uv0[2]; uint16_t uv1[2]; uint8_t col[4]; };
    std::vector<S1> stream1(unique_count);

    for (std::size_t i = 0; i < unique_count; ++i) {
        const RawVertex& rv = opt_verts[i];
        stream0[i].pos[0] = quantize_pos(rv.pos[0], qp.scale[0], qp.bias[0]);
        stream0[i].pos[1] = quantize_pos(rv.pos[1], qp.scale[1], qp.bias[1]);
        stream0[i].pos[2] = quantize_pos(rv.pos[2], qp.scale[2], qp.bias[2]);
        stream0[i].pad    = 0;

        oct_encode(rv.normal[0], rv.normal[1], rv.normal[2],
                   stream1[i].n[0], stream1[i].n[1]);
        oct_encode(rv.tangent[0], rv.tangent[1], rv.tangent[2],
                   stream1[i].t[0], stream1[i].t[1]);

        stream1[i].uv0[0] = static_cast<uint16_t>(
            std::clamp(std::round(rv.uv0[0] * 65535.0f), 0.0f, 65535.0f));
        stream1[i].uv0[1] = static_cast<uint16_t>(
            std::clamp(std::round(rv.uv0[1] * 65535.0f), 0.0f, 65535.0f));
        stream1[i].uv1[0] = static_cast<uint16_t>(
            std::clamp(std::round(rv.uv1[0] * 65535.0f), 0.0f, 65535.0f));
        stream1[i].uv1[1] = static_cast<uint16_t>(
            std::clamp(std::round(rv.uv1[1] * 65535.0f), 0.0f, 65535.0f));
        std::memcpy(stream1[i].col, rv.color, 4);
    }

    // Index buffers: UINT16 if ≤ 65535 verts, else UINT32.
    const bool use_u16 = (unique_count <= 65535);
    std::vector<uint16_t> index_u16;
    std::vector<uint32_t> index_u32;

    for (std::size_t lod = 0; lod < kNumLODs; ++lod) {
        if (use_u16) {
            for (uint32_t idx : lod_indices[lod])
                index_u16.push_back(static_cast<uint16_t>(idx));
        } else {
            for (uint32_t idx : lod_indices[lod])
                index_u32.push_back(idx);
        }
    }

    // ------------------------------------------------------------------
    // 8. Serialise to .gwmesh
    // ------------------------------------------------------------------
    std::vector<uint8_t> out;
    out.reserve(1 << 20); // 1 MiB initial

    // Reserve space for header (80 bytes).
    const std::size_t header_pos = out.size();
    out.resize(out.size() + sizeof(GwMeshHeader), 0u);

    // LOD table.
    pad_to(out, 16);
    const uint32_t lod_table_off = static_cast<uint32_t>(out.size());
    uint32_t running_idx_offset = 0;
    for (std::size_t lod = 0; lod < kNumLODs; ++lod) {
        const uint32_t count = static_cast<uint32_t>(lod_indices[lod].size());
        write_u32(out, running_idx_offset);
        write_u32(out, count);
        running_idx_offset += count * (use_u16 ? 2u : 4u);
    }

    // Single submesh.
    pad_to(out, 16);
    const uint32_t submesh_tab_off = static_cast<uint32_t>(out.size());
    // submesh entry: name_hash(4), mat_idx(2), lod_first(2), lods(4×8=32), vstart(4), vcount(4)
    write_u32(out, 0u); // name_hash
    write_u16(out, 0u); // material_idx
    write_u16(out, 0u); // lod_first
    for (std::size_t lod = 0; lod < kNumLODs; ++lod) {
        write_u32(out, static_cast<uint32_t>(lod_indices[lod].size() == 0 ? 0 :
            [&]{uint32_t off=0; for(std::size_t l=0;l<lod;++l) off += static_cast<uint32_t>(lod_indices[l].size())*(use_u16?2u:4u); return off;}()));
        write_u32(out, static_cast<uint32_t>(lod_indices[lod].size()));
    }
    write_u32(out, 0u); // vertex_start
    write_u32(out, static_cast<uint32_t>(unique_count));

    // Stream 0.
    pad_to(out, 16);
    const uint32_t s0_offset = static_cast<uint32_t>(out.size());
    const uint32_t s0_size   = static_cast<uint32_t>(stream0.size() * sizeof(S0));
    write_bytes(out, stream0.data(), s0_size);

    // Stream 1.
    pad_to(out, 16);
    const uint32_t s1_offset = static_cast<uint32_t>(out.size());
    const uint32_t s1_size   = static_cast<uint32_t>(stream1.size() * sizeof(S1));
    write_bytes(out, stream1.data(), s1_size);

    // Index buffer.
    pad_to(out, 16);
    const uint32_t idx_offset = static_cast<uint32_t>(out.size());
    uint32_t idx_size = 0;
    if (use_u16) {
        idx_size = static_cast<uint32_t>(index_u16.size() * 2u);
        write_bytes(out, index_u16.data(), idx_size);
    } else {
        idx_size = static_cast<uint32_t>(index_u32.size() * 4u);
        write_bytes(out, index_u32.data(), idx_size);
    }

    // Fill in header.
    GwMeshHeader hdr{};
    hdr.magic           = magic::kMesh;
    hdr.version         = 1;
    hdr.flags           = 0;
    hdr.lod_count       = static_cast<uint8_t>(kNumLODs);
    hdr.submesh_count   = 1;
    std::memcpy(hdr.aabb_min, aabb_min, 12);
    std::memcpy(hdr.aabb_max, aabb_max, 12);
    std::memcpy(hdr.pos_scale, qp.scale, 12);
    std::memcpy(hdr.pos_bias,  qp.bias,  12);
    hdr.stream0_offset  = s0_offset;
    hdr.stream0_size    = s0_size;
    hdr.stream1_offset  = s1_offset;
    hdr.stream1_size    = s1_size;
    hdr.index_offset    = idx_offset;
    hdr.index_size      = idx_size;
    hdr.lod_table_off   = lod_table_off;
    hdr.submesh_table_off = submesh_tab_off;
    std::memcpy(out.data() + header_pos, &hdr, sizeof(hdr));

    // ------------------------------------------------------------------
    // 9. Write output file
    // ------------------------------------------------------------------
    std::filesystem::create_directories(ctx.output_path.parent_path());
    std::ofstream of(ctx.output_path, std::ios::binary | std::ios::trunc);
    if (!of) return std::unexpected(AssetError{AssetErrorCode::CookFailed,
                                                ctx.output_path.string().c_str()});
    of.write(reinterpret_cast<const char*>(out.data()),
             static_cast<std::streamsize>(out.size()));

    const auto t1 = std::chrono::steady_clock::now();
    const uint32_t ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    CookResult result{};
    result.cook_key     = cook_key;
    result.cook_time_ms = ms;
    result.cache_hit    = false;
    return result;
}

} // namespace gw::assets::cook
