// editor/modules/module_manifest.cpp — Part B §12.2 scaffold.

#include "editor/modules/module_manifest.hpp"

#include <array>

namespace gw::editor::modules {

namespace {
constexpr std::size_t MAX_MODULES = 32;

struct Registry {
    std::array<const ModuleManifest*, MAX_MODULES> manifests{};
    std::size_t                                    count = 0;
};

Registry& registry() noexcept {
    static Registry r{};
    return r;
}
} // namespace

void register_module(const ModuleManifest& manifest) noexcept {
    auto& r = registry();
    if (r.count < MAX_MODULES) {
        r.manifests[r.count++] = &manifest;
    }
}

std::size_t module_count() noexcept { return registry().count; }

const ModuleManifest* module_at(std::size_t index) noexcept {
    auto& r = registry();
    return index < r.count ? r.manifests[index] : nullptr;
}

} // namespace gw::editor::modules
