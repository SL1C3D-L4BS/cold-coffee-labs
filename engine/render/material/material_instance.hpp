#pragma once
// engine/render/material/material_instance.hpp — ADR-0075 Wave 17B.

#include "engine/render/material/material_types.hpp"

#include <cstdint>
#include <string_view>

namespace gw::render::material {

class MaterialTemplate;

// A MaterialInstance is a thin value-type view over a MaterialInstanceState.
// The owner is always the MaterialWorld; external code addresses instances
// by MaterialInstanceId and pokes their state through MaterialWorld's
// set_param/set_texture.
class MaterialInstance {
public:
    MaterialInstance() = default;
    MaterialInstance(MaterialInstanceId id, MaterialInstanceState* s, const MaterialTemplate* t);

    [[nodiscard]] MaterialInstanceId         id()   const noexcept { return id_; }
    [[nodiscard]] const MaterialTemplate*    tmpl() const noexcept { return tmpl_; }
    [[nodiscard]] MaterialInstanceState*     state() noexcept      { return state_; }
    [[nodiscard]] const MaterialInstanceState* state() const noexcept { return state_; }
    [[nodiscard]] bool                       valid() const noexcept { return state_ != nullptr; }

    bool set_f32   (std::string_view key, float v);
    bool set_vec4  (std::string_view key, float x, float y, float z, float w);
    bool set_i32   (std::string_view key, std::int32_t v);
    bool set_texture(std::uint32_t slot, TextureHandle h, std::uint32_t fingerprint = 0);

    [[nodiscard]] bool get(std::string_view key, ParameterValue& out) const;

    // Recompute content_hash + bump revision. Called internally after any
    // mutator. Exposed publicly so MaterialWorld can batch.
    void touch() noexcept;
    [[nodiscard]] std::uint64_t content_hash() const noexcept;

private:
    MaterialInstanceId      id_{};
    MaterialInstanceState*  state_{nullptr};
    const MaterialTemplate* tmpl_{nullptr};
};

} // namespace gw::render::material
