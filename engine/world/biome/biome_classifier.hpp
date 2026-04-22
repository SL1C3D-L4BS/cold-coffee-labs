#pragma once
// engine/world/biome/biome_classifier.hpp — Phase 19-B Sacrilege circle classifier (SDR-driven fields).

#include "engine/core/span.hpp"
#include "engine/world/streaming/chunk_data.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/universe/hec.hpp"
#include "engine/world/universe/sdr_noise.hpp"

#include <memory>

namespace gw::world::biome {

struct BiomeParams {
    float temperature{}; ///< [-1, 1]
    float humidity{}; ///< [-1, 1]
    float elevation{}; ///< [-1, 1]
    float chaos{}; ///< [0, 1]
};

/// Samples four independent SDR scalar fields (HEC-stratified seeds) then applies the Sacrilege
/// Nine Circles decision tree. `sdr_basis_seed` is typically Σ_chunk from `UniverseSeedManager`.
class BiomeClassifier {
public:
    explicit BiomeClassifier(const gw::universe::UniverseSeed& sdr_basis_seed) noexcept;

    /// Deterministic rule evaluation on explicit parameters (SIMD bulk path uses this kernel).
    [[nodiscard]] gw::world::streaming::BiomeId classify_params(BiomeParams p) const noexcept;

    /// Samples SDR at `world_pos` (f64 metres) for all four fields, then `classify_params`.
    [[nodiscard]] gw::world::streaming::BiomeId classify(const gw::world::streaming::Vec3_f64& world_pos) const noexcept;

    /// Element-wise agreement with `classify` when `out.size() == positions.size()`.
    void classify_bulk(gw::core::Span<const gw::world::streaming::Vec3_f64> positions,
                       gw::core::Span<gw::world::streaming::BiomeId>        out) const noexcept;

private:
    gw::universe::UniverseSeed                 basis_{};
    std::unique_ptr<gw::universe::SdrNoise> noise_temperature_{};
    std::unique_ptr<gw::universe::SdrNoise> noise_humidity_{};
    std::unique_ptr<gw::universe::SdrNoise> noise_elevation_{};
    std::unique_ptr<gw::universe::SdrNoise> noise_chaos_{};
};

} // namespace gw::world::biome
