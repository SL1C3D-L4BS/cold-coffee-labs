// engine/render/material/material_world.cpp — ADR-0075 Wave 17B.

#include "engine/render/material/material_world.hpp"

#include "engine/core/config/cvar_registry.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gw::render::material {

struct MaterialWorld::Impl {
    MaterialConfig                                          cfg{};
    gw::config::CVarRegistry*                               cvars{nullptr};
    gw::events::CrossSubsystemBus*                          bus{nullptr};
    gw::jobs::Scheduler*                                    sched{nullptr};
    bool                                                    inited{false};
    std::uint64_t                                           steps{0};

    // Templates live in a dense vector; id == index+1.
    std::vector<std::unique_ptr<MaterialTemplate>>          templates;
    std::unordered_map<std::string, MaterialTemplateId>     templates_by_name;

    // Instances live in a dense vector with a free list.
    std::vector<std::unique_ptr<MaterialInstanceState>>     instance_states;
    std::vector<std::uint32_t>                              free_list;

    std::unordered_set<std::uint32_t>                       reload_queue;
    MaterialStats                                           stats{};

    // Helper to validate an id and return a live index.
    [[nodiscard]] std::uint32_t live_index(MaterialInstanceId id) const noexcept {
        if (id.value == 0 || id.value > instance_states.size()) return 0;
        if (!instance_states[id.value - 1]) return 0;
        return id.value;
    }

    [[nodiscard]] MaterialInstanceState* state_of(MaterialInstanceId id) noexcept {
        const auto idx = live_index(id);
        return idx == 0 ? nullptr : instance_states[idx - 1].get();
    }

    [[nodiscard]] const MaterialTemplate* template_of(MaterialInstanceId id) const noexcept {
        if (id.value == 0 || id.value > instance_states.size()) return nullptr;
        const auto& st = instance_states[id.value - 1];
        if (!st) return nullptr;
        const auto& parent = st->parent;
        if (parent.value == 0 || parent.value > templates.size()) return nullptr;
        return templates[parent.value - 1].get();
    }

    void pull_cvars() {
        if (!cvars) return;
        cfg.default_template     = cvars->get_string_or("mat.default_template", cfg.default_template);
        cfg.instance_budget      = static_cast<std::uint32_t>(
            cvars->get_i32_or("mat.instance_budget", static_cast<std::int32_t>(cfg.instance_budget)));
        cfg.texture_cache_mb     = static_cast<std::uint32_t>(
            cvars->get_i32_or("mat.texture_cache_mb", static_cast<std::int32_t>(cfg.texture_cache_mb)));
        cfg.auto_reload          = cvars->get_bool_or("mat.auto_reload", cfg.auto_reload);
        cfg.preview_sphere       = cvars->get_bool_or("mat.preview_sphere", cfg.preview_sphere);
        cfg.quality              = parse_quality(cvars->get_string_or("mat.quality_tier", std::string{to_string(cfg.quality)}));
        cfg.sheen_enabled        = cvars->get_bool_or("mat.sheen_enabled", cfg.sheen_enabled);
        cfg.transmission_enabled = cvars->get_bool_or("mat.transmission_enabled", cfg.transmission_enabled);
    }

    void seed_default_templates() {
        auto add_opaque = [&](std::string name, PbrExtension ext) {
            MaterialTemplateDesc d;
            d.name       = std::move(name);
            d.extensions = ext;
            d.quality    = cfg.quality;
            d.parameter_keys = {"base_color", "metallic_roughness", "emissive"};
            auto tmpl = std::make_unique<MaterialTemplate>(std::move(d));
            for (const auto& k : {"base_color", "metallic_roughness", "emissive"}) {
                (void)tmpl->register_param(k, ParameterValue::Kind::Vec4);
            }
            // default base_color = (1,1,1,1).
            ParameterValue bc{}; bc.kind = ParameterValue::Kind::Vec4; bc.v = {1, 1, 1, 1};
            tmpl->set_default_param("base_color", bc);
            const auto name_str = tmpl->desc().name;
            templates.push_back(std::move(tmpl));
            const MaterialTemplateId id{static_cast<std::uint32_t>(templates.size())};
            templates_by_name[name_str] = id;
            ++stats.templates;
        };
        add_opaque("pbr_opaque/metal_rough",  PbrExtension::None);
        add_opaque("pbr_opaque/dielectric",   PbrExtension::Specular);
        add_opaque("pbr_opaque/clearcoat",    PbrExtension::Clearcoat);
        if (cfg.sheen_enabled)        add_opaque("pbr_opaque/sheen",        PbrExtension::Sheen);
        if (cfg.transmission_enabled) add_opaque("pbr_opaque/transmission", PbrExtension::Transmission);
    }
};

MaterialWorld::MaterialWorld() : impl_(std::make_unique<Impl>()) {}
MaterialWorld::~MaterialWorld() { shutdown(); }

bool MaterialWorld::initialize(MaterialConfig cfg,
                                gw::config::CVarRegistry* cvars,
                                gw::events::CrossSubsystemBus* bus,
                                gw::jobs::Scheduler* scheduler) {
    shutdown();
    impl_->cfg   = std::move(cfg);
    impl_->cvars = cvars;
    impl_->bus   = bus;
    impl_->sched = scheduler;
    impl_->pull_cvars();
    impl_->seed_default_templates();
    impl_->inited = true;
    return true;
}

void MaterialWorld::shutdown() {
    impl_->instance_states.clear();
    impl_->templates.clear();
    impl_->templates_by_name.clear();
    impl_->free_list.clear();
    impl_->reload_queue.clear();
    impl_->stats = {};
    impl_->inited = false;
}

bool MaterialWorld::initialized() const noexcept { return impl_->inited; }

void MaterialWorld::step(double dt_seconds) {
    (void)dt_seconds;
    if (!impl_->inited) return;
    ++impl_->steps;
    if (impl_->cfg.auto_reload && !impl_->reload_queue.empty()) {
        for (auto value : impl_->reload_queue) {
            (void)reload_instance(MaterialInstanceId{value});
        }
        impl_->reload_queue.clear();
    }
}

MaterialTemplateId MaterialWorld::create_template(MaterialTemplateDesc desc) {
    if (!impl_->inited) return {};
    if (auto it = impl_->templates_by_name.find(desc.name); it != impl_->templates_by_name.end()) {
        return it->second;
    }
    auto tmpl = std::make_unique<MaterialTemplate>(std::move(desc));
    const auto name = tmpl->desc().name;
    impl_->templates.push_back(std::move(tmpl));
    MaterialTemplateId id{static_cast<std::uint32_t>(impl_->templates.size())};
    impl_->templates_by_name[name] = id;
    ++impl_->stats.templates;
    return id;
}

const MaterialTemplate* MaterialWorld::get_template(MaterialTemplateId id) const noexcept {
    if (id.value == 0 || id.value > impl_->templates.size()) return nullptr;
    return impl_->templates[id.value - 1].get();
}

MaterialTemplateId MaterialWorld::find_template_by_name(std::string_view name) const noexcept {
    auto it = impl_->templates_by_name.find(std::string{name});
    return it == impl_->templates_by_name.end() ? MaterialTemplateId{} : it->second;
}

std::size_t MaterialWorld::template_count() const noexcept {
    return impl_->templates.size();
}

MaterialInstanceId MaterialWorld::create_instance(MaterialTemplateId parent) {
    if (!impl_->inited) return {};
    if (parent.value == 0 || parent.value > impl_->templates.size()) return {};
    if (impl_->stats.instances >= impl_->cfg.instance_budget) return {};

    auto st = std::make_unique<MaterialInstanceState>();
    st->parent = parent;
    impl_->templates[parent.value - 1]->populate_default(st->block);

    std::uint32_t idx;
    if (!impl_->free_list.empty()) {
        idx = impl_->free_list.back();
        impl_->free_list.pop_back();
        impl_->instance_states[idx - 1] = std::move(st);
    } else {
        impl_->instance_states.push_back(std::move(st));
        idx = static_cast<std::uint32_t>(impl_->instance_states.size());
    }

    MaterialInstanceId id{idx};
    MaterialInstance tmp(id, impl_->instance_states[idx - 1].get(), impl_->templates[parent.value - 1].get());
    tmp.touch();
    ++impl_->stats.instances;
    impl_->stats.instance_high_water = std::max(impl_->stats.instance_high_water, impl_->stats.instances);
    return id;
}

MaterialInstance MaterialWorld::get_instance(MaterialInstanceId id) noexcept {
    auto* s = impl_->state_of(id);
    if (!s) return {};
    return MaterialInstance(id, s, impl_->template_of(id));
}

void MaterialWorld::destroy_instance(MaterialInstanceId id) noexcept {
    if (id.value == 0 || id.value > impl_->instance_states.size()) return;
    if (!impl_->instance_states[id.value - 1]) return;
    impl_->instance_states[id.value - 1].reset();
    impl_->free_list.push_back(id.value);
    if (impl_->stats.instances > 0) --impl_->stats.instances;
}

bool MaterialWorld::set_param_f32(MaterialInstanceId id, std::string_view key, float v) {
    auto inst = get_instance(id);
    if (!inst.valid()) return false;
    if (!inst.set_f32(key, v)) return false;
    ++impl_->stats.param_uploads;
    return true;
}

bool MaterialWorld::set_param_vec4(MaterialInstanceId id, std::string_view key,
                                     float x, float y, float z, float w) {
    auto inst = get_instance(id);
    if (!inst.valid()) return false;
    if (!inst.set_vec4(key, x, y, z, w)) return false;
    ++impl_->stats.param_uploads;
    return true;
}

bool MaterialWorld::set_param_i32(MaterialInstanceId id, std::string_view key, std::int32_t v) {
    auto inst = get_instance(id);
    if (!inst.valid()) return false;
    if (!inst.set_i32(key, v)) return false;
    ++impl_->stats.param_uploads;
    return true;
}

bool MaterialWorld::set_texture(MaterialInstanceId id, std::uint32_t slot,
                                  TextureHandle h, std::uint32_t fingerprint) {
    auto inst = get_instance(id);
    if (!inst.valid()) return false;
    if (!inst.set_texture(slot, h, fingerprint)) return false;
    ++impl_->stats.texture_binds;
    return true;
}

bool MaterialWorld::get_param(MaterialInstanceId id, std::string_view key, ParameterValue& out) const {
    if (id.value == 0 || id.value > impl_->instance_states.size()) return false;
    const auto& st = impl_->instance_states[id.value - 1];
    if (!st) return false;
    const auto* tmpl = impl_->template_of(id);
    if (!tmpl) return false;
    return tmpl->read_param(st->block, key, out);
}

bool MaterialWorld::reload_instance(MaterialInstanceId id) {
    auto inst = get_instance(id);
    if (!inst.valid()) return false;
    // Reload = re-copy default + bump revision; tests assert revision bumps.
    const auto* tmpl = impl_->template_of(id);
    if (!tmpl) return false;
    tmpl->populate_default(inst.state()->block);
    inst.touch();
    ++impl_->stats.reloads;
    return true;
}

std::size_t MaterialWorld::reload_all() {
    std::size_t n = 0;
    for (std::uint32_t i = 1; i <= impl_->instance_states.size(); ++i) {
        if (impl_->instance_states[i - 1]) {
            if (reload_instance(MaterialInstanceId{i})) ++n;
        }
    }
    return n;
}

void MaterialWorld::request_reload(MaterialInstanceId id) {
    if (id.value) impl_->reload_queue.insert(id.value);
}

std::vector<std::string> MaterialWorld::list_templates() const {
    std::vector<std::string> out;
    out.reserve(impl_->templates.size());
    for (const auto& t : impl_->templates) out.push_back(t->desc().name);
    return out;
}

std::vector<MaterialInstanceId> MaterialWorld::list_instances() const {
    std::vector<MaterialInstanceId> out;
    for (std::uint32_t i = 1; i <= impl_->instance_states.size(); ++i) {
        if (impl_->instance_states[i - 1]) out.push_back(MaterialInstanceId{i});
    }
    return out;
}

std::string MaterialWorld::describe_instance(MaterialInstanceId id) const {
    if (id.value == 0 || id.value > impl_->instance_states.size()) return {};
    const auto& st = impl_->instance_states[id.value - 1];
    if (!st) return {};
    const auto* tmpl = impl_->template_of(id);
    if (!tmpl) return {};
    char buf[384];
    std::snprintf(buf, sizeof(buf),
                  "{\"id\":%u,\"template\":\"%s\",\"revision\":%u,\"hash\":\"0x%016llx\"}",
                  id.value,
                  tmpl->desc().name.c_str(),
                  st->revision,
                  static_cast<unsigned long long>(st->content_hash));
    return buf;
}

std::string MaterialWorld::preview_sphere_description(MaterialTemplateId id) const {
    const auto* t = get_template(id);
    if (!t) return {};
    char buf[192];
    std::snprintf(buf, sizeof(buf),
                  "{\"mesh\":\"sphere\",\"radius\":0.5,\"template\":\"%s\"}",
                  t->desc().name.c_str());
    return buf;
}

MaterialStats             MaterialWorld::stats() const noexcept { return impl_->stats; }
const MaterialConfig&     MaterialWorld::config() const noexcept { return impl_->cfg; }
std::uint64_t             MaterialWorld::step_count() const noexcept { return impl_->steps; }

} // namespace gw::render::material
