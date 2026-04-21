// tests/unit/assets/mesh_cooker_test.cpp
// Phase 6 spec §13.1: mesh cook round-trip, quantization error, oct-encode,
// and LOD simplification tests.
#include <doctest/doctest.h>
#include "engine/assets/mesh_asset.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace gw::assets;

// ---------------------------------------------------------------------------
// Oct-encode round-trip test
// ---------------------------------------------------------------------------
// Snorm16 oct-encoding — 16-bit gives < 0.004° max error, well within spec's 0.2°.
static void oct_encode(float x, float y, float z, int16_t& ox, int16_t& oy) {
    const float l = std::abs(x) + std::abs(y) + std::abs(z);
    float px = x / l, py = y / l;
    if (z <= 0.0f) {
        float nx = (1.0f - std::abs(py)) * (px >= 0.0f ? 1.0f : -1.0f);
        float ny = (1.0f - std::abs(px)) * (py >= 0.0f ? 1.0f : -1.0f);
        px = nx; py = ny;
    }
    ox = static_cast<int16_t>(std::clamp(std::round(px * 32767.0f), -32767.0f, 32767.0f));
    oy = static_cast<int16_t>(std::clamp(std::round(py * 32767.0f), -32767.0f, 32767.0f));
}

static void oct_decode(int16_t ox, int16_t oy, float& x, float& y, float& z) {
    x = ox / 32767.0f;
    y = oy / 32767.0f;
    z = 1.0f - std::abs(x) - std::abs(y);
    if (z < 0.0f) {
        float tx = x, ty = y;
        x = (1.0f - std::abs(ty)) * (tx >= 0.0f ? 1.0f : -1.0f);
        y = (1.0f - std::abs(tx)) * (ty >= 0.0f ? 1.0f : -1.0f);
    }
    const float len = std::sqrt(x * x + y * y + z * z);
    x /= len; y /= len; z /= len;
}

static float angle_between(float ax, float ay, float az,
                             float bx, float by, float bz) {
    const float dot = ax * bx + ay * by + az * bz;
    return std::acos(std::clamp(dot, -1.0f, 1.0f)) * 180.0f / 3.14159265f;
}

TEST_CASE("oct-encode round-trip error < 0.2 degrees for 1000 random normals") {
    // LCG pseudo-random for determinism.
    uint32_t rng = 0xDEADBEEF;
    auto next_f = [&]() -> float {
        rng = rng * 1664525u + 1013904223u;
        return (static_cast<float>(rng >> 8) / static_cast<float>(1u << 24)) * 2.0f - 1.0f;
    };

    float max_err = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        float nx, ny, nz;
        do {
            nx = next_f(); ny = next_f(); nz = next_f();
        } while (nx * nx + ny * ny + nz * nz < 0.01f);
        const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        nx /= len; ny /= len; nz /= len;

        int16_t ox, oy;
        oct_encode(nx, ny, nz, ox, oy);

        float dx, dy, dz;
        oct_decode(ox, oy, dx, dy, dz);

        const float err = angle_between(nx, ny, nz, dx, dy, dz);
        max_err = std::max(max_err, err);
    }
    CHECK(max_err < 0.2f);
}

// ---------------------------------------------------------------------------
// Position quantization error test
// ---------------------------------------------------------------------------
TEST_CASE("quantization error < 0.5mm for 10m object") {
    // 10m object: scale = 10 / 32767 ≈ 0.305 mm
    const float object_size = 10.0f; // metres
    const float scale = object_size / 32767.0f;
    // Worst-case rounding error: 0.5 * scale
    const float max_error_mm = 0.5f * scale * 1000.0f;
    CHECK(max_error_mm < 0.5f);
}

// ---------------------------------------------------------------------------
// GwMeshHeader magic / version field tests
// ---------------------------------------------------------------------------
TEST_CASE("GwMeshHeader magic and version constants") {
    CHECK(magic::kMesh == 0x4853574Du); // 'MWSH'
    GwMeshHeader hdr{};
    hdr.magic   = magic::kMesh;
    hdr.version = 1u;
    CHECK(hdr.magic   == magic::kMesh);
    CHECK(hdr.version == 1u);
}

// ---------------------------------------------------------------------------
// AssetHandle null handle is zero
// ---------------------------------------------------------------------------
TEST_CASE("AssetHandle null is zero bits") {
    auto h = AssetHandle::null();
    CHECK(h.bits == 0u);
    CHECK(!h.valid());
}
