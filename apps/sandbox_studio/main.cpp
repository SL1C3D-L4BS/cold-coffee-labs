// apps/sandbox_studio — Phase 18 *Studio Ready* exit gate: .gwseq playback + mod + BLD seq tools.

#include "apps/sandbox_studio/build_test_gwseq.hpp"

#include "bld/include/bld_ffi_seq_export.h"
#include "editor/bld_api/editor_bld_api.hpp"
#include "engine/assets/asset_handle.hpp"
#include "engine/assets/asset_db.hpp"
#include "engine/assets/vfs/virtual_fs.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"
#include "engine/platform/fs.hpp"
#include "engine/scene/seq/sequencer_world.hpp"
#include "engine/scene/seq/seq_camera.hpp"
#include "engine/scripting/mod_api.hpp"
#include "editor/scene/components.hpp"
#include "engine/core/crash_reporter.hpp"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#ifndef CCL_REPO_ROOT
#define CCL_REPO_ROOT "."
#endif
#ifndef GW_BUILD_ROOT
#define GW_BUILD_ROOT "."
#endif

namespace {

[[nodiscard]] bool finite_dvec3(const glm::dvec3& p) noexcept {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}

void run_command_write_create_block(std::uint8_t* p, std::uint32_t fr, std::uint32_t dur, const char* name) {
    p[0] = static_cast<std::uint8_t>(fr & 0xFFu);
    p[1] = static_cast<std::uint8_t>((fr >> 8u) & 0xFFu);
    p[2] = static_cast<std::uint8_t>((fr >> 16u) & 0xFFu);
    p[3] = static_cast<std::uint8_t>((fr >> 24u) & 0xFFu);
    p[4] = static_cast<std::uint8_t>(dur & 0xFFu);
    p[5] = static_cast<std::uint8_t>((dur >> 8u) & 0xFFu);
    p[6] = static_cast<std::uint8_t>((dur >> 16u) & 0xFFu);
    p[7] = static_cast<std::uint8_t>((dur >> 24u) & 0xFFu);
    (void)std::snprintf(reinterpret_cast<char*>(p + 8u), 256u, "%s", name);
}

}  // namespace

int main() {
    gw::core::crash::install_handlers();
    bool    seq_ok    = false;
    bool    mod_ok    = false;
    bool    bld_ok    = false;
    int     fail_line = 0;

    const std::filesystem::path repo = std::filesystem::path(CCL_REPO_ROOT);
    const std::filesystem::path test_gwseq = repo / "assets" / "test" / "test.gwseq";

    std::vector<std::uint8_t> gwseq_bytes;
    if (gw::platform::FileSystem::exists(test_gwseq)) {
        if (const auto rb = gw::platform::FileSystem::read_bytes(test_gwseq); rb.has_value() && !rb->empty()) {
            const auto& b = *rb;
            gwseq_bytes.resize(b.size());
            for (std::size_t i = 0; i < b.size(); ++i) {
                gwseq_bytes[i] = static_cast<std::uint8_t>(b[i]);
            }
        }
    }
    if (gwseq_bytes.empty()) {
        gwseq_bytes = gw::studio_sandbox::make_test_gwseq_bytes();
    }

    gw::ecs::World                 world;
    gw::seq::SequencerWorld        tapes;
    gw::seq::CinematicCameraSystem cine{};

    gw::assets::AssetHandle seq_h{};
    seq_h.bits = 0x5EED5EEDull;
    if (!tapes.register_sequence(seq_h, std::move(gwseq_bytes)).has_value()) {
        std::printf("STUDIO assert fail: register_sequence at %d\n", __LINE__);
        return 1;
    }

    if (tapes.reader_for(seq_h) == nullptr) {
        std::printf("STUDIO assert fail: reader_for at %d\n", __LINE__);
        return 1;
    }
    const std::uint32_t duration_frames = tapes.reader_for(seq_h)->header().duration_frames;

    const auto cam  = world.create_entity();
    const auto play = world.create_entity();
    world.add_component<gw::editor::scene::TransformComponent>(cam, gw::editor::scene::TransformComponent{});
    {
        gw::seq::CinematicCameraComponent cam_c{};
        cam_c.active  = true;
        cam_c.fov_deg = 60.f;
        world.add_component<gw::seq::CinematicCameraComponent>(cam, std::move(cam_c));
    }
    {
        gw::seq::SeqPlayerComponent pl{};
        pl.seq_asset_id    = seq_h;
        pl.play_head_frame = 0.0;
        pl.playing         = true;
        pl.loop            = false;
        pl.playback_rate   = 1.f;
        world.add_component<gw::seq::SeqPlayerComponent>(play, std::move(pl));
    }
    world.add_component<gw::seq::SeqTransformTrackComponent>(play, gw::seq::SeqTransformTrackComponent{.track_id = 1u, .target_entity = cam});

    const float  dt  = 1.f / 60.f;
    bool         any_non_finite = false;
    for (std::uint32_t f = 0; f < duration_frames; ++f) {
        gw::seq::SequencerSystem::tick(tapes, world, nullptr, dt);
        cine.tick(world);
        if (const auto* tr = world.get_component<gw::editor::scene::TransformComponent>(cam)) {
            if (!finite_dvec3(tr->position)) {
                any_non_finite = true;
            }
        }
    }
    if (!any_non_finite) {
        seq_ok = true;
    } else {
        fail_line = __LINE__;
    }

    {
        using namespace gw::editor::bld_api;
        g_globals.world                  = &world;
        g_globals.sequencer_world         = &tapes;
        g_globals.seq_tool_last_json.clear();
        g_globals.seq_player_entity_bits  = play.raw_bits();
        g_globals.cinematic_system     = &cine;

        constexpr const char* kName = "sandbox_studio";
        std::uint8_t         create_buf[300]{};
        run_command_write_create_block(create_buf, 60u, 120u, kName);
        const std::uint32_t  payload_bytes = 8u + static_cast<std::uint32_t>(std::strlen(kName)) + 1u;
        const std::uint64_t  hbits          = dispatch_run_command(0x0001u, create_buf, payload_bytes);
        if (hbits == 0u) {
            fail_line = __LINE__;
        } else {
            std::uint8_t addtr[12]{};
            for (int b = 0; b < 8; ++b) {
                addtr[static_cast<std::size_t>(b)] = static_cast<std::uint8_t>((hbits >> (8u * static_cast<std::uint64_t>(b))) & 0xFFu);
            }
            if (dispatch_run_command(0x0002u, addtr, sizeof(addtr)) == 0u) {
                fail_line = __LINE__;
            } else {
                std::uint8_t playp[9]{};
                for (int b = 0; b < 8; ++b) {
                    playp[static_cast<std::size_t>(b)] = static_cast<std::uint8_t>((hbits >> (8u * static_cast<std::uint64_t>(b))) & 0xFFu);
                }
                playp[8] = 0u;
                if (dispatch_run_command(0x0004u, playp, sizeof(playp)) == 0u) {
                    fail_line = __LINE__;
                } else {
                    std::uint8_t ex[8]{};
                    for (int b = 0; b < 8; ++b) {
                        ex[static_cast<std::size_t>(b)] = static_cast<std::uint8_t>((hbits >> (8u * static_cast<std::uint64_t>(b))) & 0xFFu);
                    }
                    if (dispatch_run_command(0x0005u, ex, sizeof(ex)) == 0u) {
                        fail_line = __LINE__;
                    } else {
                        bld_ffi_seq_export_set_json(g_globals.seq_tool_last_json.c_str());
                        bld_ok = !g_globals.seq_tool_last_json.empty();
                    }
                }
            }
        }
        (void)dispatch_run_command(0x0003u, nullptr, 0u);
        g_globals.world                 = nullptr;
        g_globals.sequencer_world     = nullptr;
        g_globals.cinematic_system    = nullptr;
        g_globals.seq_tool_last_json.clear();
        g_globals.seq_player_entity_bits   = 0u;
    }

    if (!bld_ok && fail_line == 0) {
        fail_line = __LINE__;
    }

    {
        // GPU-less Phase-18 exit gate: ModRegistry only needs VFS + manifest IO.
        // Ship mesh loads use `AssetDatabase(VulkanDevice&, vfs)` (see
        // `docs/ENGINE_EDITOR_RUNTIME_WIRING.md`); this binary intentionally does
        // not bootstrap Vulkan.
        gw::assets::vfs::VirtualFilesystem vfs;
        gw::assets::AssetDatabase           assets{gw::assets::AssetDatabaseModHarnessTag{}, vfs};
        gw::scripting::ModRegistry    mods(world, assets);
        const std::filesystem::path   man = std::filesystem::path(GW_BUILD_ROOT) / "mods" / "mod_manifest.json";
        if (!gw::platform::FileSystem::exists(man)) {
            if (fail_line == 0) {
                fail_line = __LINE__;
            }
        } else {
            if (const auto loaded = mods.load_mod(man); loaded.has_value()) {
                for (int f = 0; f < 10; ++f) {
                    mods.tick_all(world, 1.f / 60.f);
                }
                mods.unload_mod(loaded.value());
                mod_ok = true;
            } else if (fail_line == 0) {
                fail_line = __LINE__;
            }
        }
    }

    if (seq_ok && mod_ok && bld_ok) {
        std::printf("STUDIO READY — seq=OK mod=OK bld_tools=OK\n");
        return 0;
    }
    std::printf("STUDIO NOT READY (seq=%d mod=%d bld=%d) assert near line %d\n", static_cast<int>(seq_ok),
                 static_cast<int>(mod_ok), static_cast<int>(bld_ok), fail_line);
    return 1;
}
