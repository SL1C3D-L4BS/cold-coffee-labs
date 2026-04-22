// gameplay/mod_example — Phase 18-C sample gameplay_module-style mod.
//
// Exports the stable C-ABI the engine mod loader looks up. Uses only the public
// mod surface (World, AssetDatabase, ModTransformComponent); no editor or render headers.

#include "engine/assets/asset_db.hpp"
#include "engine/core/log.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scripting/mod_components.hpp"

#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#  define GW_EXAMPLE_MOD_API extern "C" __declspec(dllexport)
#else
#  define GW_EXAMPLE_MOD_API extern "C" __attribute__((visibility("default")))
#endif

namespace {

gw::ecs::Entity g_debug_entity{gw::ecs::Entity::null()};

void stamp_load_if_harness() noexcept {
    const char* const path = std::getenv("GW_MOD_LOAD_STAMP");
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    if (std::FILE* f = std::fopen(path, "ab")) {
        (void)std::fputc('+', f);
        (void)std::fclose(f);
    }
}

}  // namespace

GW_EXAMPLE_MOD_API int gw_mod_on_load(gw::ecs::World* w, gw::assets::AssetDatabase* a) {
    (void)a;
    if (w == nullptr) {
        return 1;
    }
    stamp_load_if_harness();
    g_debug_entity = w->create_entity();
    w->add_component<gw::scripting::ModTransformComponent>(g_debug_entity, gw::scripting::ModTransformComponent{});
    GW_LOG(Info, "example_mod", "example_mod on_load");
    return 0;
}

GW_EXAMPLE_MOD_API void gw_mod_on_unload(gw::ecs::World* w) {
    if (w == nullptr) {
        return;
    }
    if (w->is_alive(g_debug_entity)) {
        w->destroy_entity(g_debug_entity);
    }
    g_debug_entity = gw::ecs::Entity::null();
    GW_LOG(Info, "example_mod", "example_mod on_unload");
}

GW_EXAMPLE_MOD_API void gw_mod_on_tick(gw::ecs::World* w, float dt) {
    (void)dt;
    if (w == nullptr || !w->is_alive(g_debug_entity)) {
        return;
    }
    if (const auto* tr = w->get_component<gw::scripting::ModTransformComponent>(g_debug_entity)) {
        char buf[160];
        (void)std::snprintf(buf, sizeof(buf), "entity pos: %.3f %.3f %.3f", tr->position_x, tr->position_y, tr->position_z);
        GW_LOG(Info, "example_mod", buf);
    }
}
