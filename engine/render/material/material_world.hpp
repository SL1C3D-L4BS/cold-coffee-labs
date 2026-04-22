#pragma once
// engine/render/material/material_world.hpp — ADR-0075 Wave 17B PIMPL façade.

#include "engine/render/material/events_material.hpp"
#include "engine/render/material/material_config.hpp"
#include "engine/render/material/material_instance.hpp"
#include "engine/render/material/material_template.hpp"
#include "engine/render/material/material_types.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::config     { class CVarRegistry; }
namespace gw::events     { class CrossSubsystemBus; }
namespace gw::jobs       { class Scheduler; }

namespace gw::render::material {

class MaterialWorld {
public:
    MaterialWorld();
    ~MaterialWorld();
    MaterialWorld(const MaterialWorld&)            = delete;
    MaterialWorld& operator=(const MaterialWorld&) = delete;

    [[nodiscard]] bool initialize(MaterialConfig cfg,
                                   gw::config::CVarRegistry* cvars      = nullptr,
                                   gw::events::CrossSubsystemBus* bus   = nullptr,
                                   gw::jobs::Scheduler* scheduler       = nullptr);
    void               shutdown();
    [[nodiscard]] bool initialized() const noexcept;

    // Per-frame tick. Drains the reload chain. No-op when initialized()==false.
    void step(double dt_seconds);

    // --- templates ---
    [[nodiscard]] MaterialTemplateId       create_template(MaterialTemplateDesc desc);
    [[nodiscard]] const MaterialTemplate*  get_template(MaterialTemplateId) const noexcept;
    [[nodiscard]] MaterialTemplateId       find_template_by_name(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t              template_count() const noexcept;

    // --- instances ---
    [[nodiscard]] MaterialInstanceId create_instance(MaterialTemplateId);
    [[nodiscard]] MaterialInstance   get_instance(MaterialInstanceId) noexcept;
    void                             destroy_instance(MaterialInstanceId) noexcept;

    bool set_param_f32 (MaterialInstanceId, std::string_view key, float v);
    bool set_param_vec4(MaterialInstanceId, std::string_view key, float x, float y, float z, float w);
    bool set_param_i32 (MaterialInstanceId, std::string_view key, std::int32_t v);
    bool set_texture   (MaterialInstanceId, std::uint32_t slot, TextureHandle, std::uint32_t fingerprint = 0);

    [[nodiscard]] bool get_param(MaterialInstanceId, std::string_view key, ParameterValue& out) const;

    // Hot-reload chain (ADR-0075 §2.4).
    [[nodiscard]] bool        reload_instance(MaterialInstanceId);
    [[nodiscard]] std::size_t reload_all();
    void                      request_reload(MaterialInstanceId);  // queued for next step()

    // Authoring (Wave 17C).
    [[nodiscard]] std::vector<std::string> list_templates() const;
    [[nodiscard]] std::vector<MaterialInstanceId> list_instances() const;
    [[nodiscard]] std::string describe_instance(MaterialInstanceId) const;
    [[nodiscard]] std::string preview_sphere_description(MaterialTemplateId) const;

    // Stats + config + bus.
    [[nodiscard]] MaterialStats             stats() const noexcept;
    [[nodiscard]] const MaterialConfig&     config() const noexcept;
    [[nodiscard]] std::uint64_t             step_count() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::render::material
