// editor/bld_api/editor_bld_api.cpp
// Thread-safe C-ABI surface for BLD (Phase 9) — Surface P, v1.
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md.
#include "editor_bld_api.hpp"

// Full type definitions required — the header only forward-declares these.
#include "editor/selection/selection_context.hpp"
#include "editor/scene/components.hpp"
#include "editor/undo/command_stack.hpp"
#include "engine/ecs/hierarchy.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/migration.hpp"
#include "engine/scene/scene_file.hpp"
#include "engine/scene/seq/gwseq_codec.hpp"
#include "engine/scene/seq/sequencer_world.hpp"
#include "engine/assets/asset_handle.hpp"
#include "engine/memory/arena_allocator.hpp"
#include "editor/seq/seq_gwseq_rebuild.hpp"
#include "bld/include/bld_ffi_seq_export.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Registrar — opaque handle concrete type (ADR-0007 §2.4). Declared in the
// bld_register.h header as `struct BldRegistrar` forward; this TU defines it.
// Intentionally minimal for v1: only tool/widget tables. User data pointers
// are BLD-owned; we keep the pointer but never dereference or free it.
// ---------------------------------------------------------------------------
struct BldEditorHandle {};  // placeholder — no fields crossed today

struct BldRegistrar {
    struct Tool {
        std::string   id;
        std::string   name;
        std::string   tooltip;
        std::string   icon;
        BldToolFn     fn        = nullptr;
        void*         user_data = nullptr;
        std::uint32_t tool_id   = 0;
    };
    struct Widget {
        std::uint64_t component_hash = 0;
        BldWidgetFn   fn             = nullptr;
        void*         user_data      = nullptr;
        std::uint32_t widget_id      = 0;
    };

    std::vector<Tool>                         tools;
    std::unordered_map<std::string, std::size_t> tool_by_id;  // id -> tools[] index
    std::vector<Widget>                       widgets;
    std::uint32_t next_tool_id   = 1;
    std::uint32_t next_widget_id = 1;
};

namespace gw::editor::bld_api {
    EditorGlobals g_globals;  // definition

    namespace {
        // The process-wide registrar is kept here so gw_editor_run_tool can
        // look up tools by id without the caller carrying the pointer. The
        // ADR allows a single registrar per BLD load; for tests we keep it
        // simple and give acquire_registrar() exclusive ownership.
        std::mutex                            g_reg_mu;
        std::unique_ptr<::BldRegistrar>       g_live_registrar;
    }

    BldRegistrar* acquire_registrar() {
        std::lock_guard lock{g_reg_mu};
        if (g_live_registrar) return nullptr;  // one-at-a-time in v1
        g_live_registrar = std::make_unique<::BldRegistrar>();
        return g_live_registrar.get();
    }

    void release_registrar(BldRegistrar* reg) {
        std::lock_guard lock{g_reg_mu};
        if (!reg || reg != g_live_registrar.get()) return;
        g_live_registrar.reset();
    }

    bool invoke_tool(const char* tool_id) {
        if (!tool_id) return false;
        std::lock_guard lock{g_reg_mu};
        if (!g_live_registrar) return false;
        auto it = g_live_registrar->tool_by_id.find(tool_id);
        if (it == g_live_registrar->tool_by_id.end()) return false;
        auto& t = g_live_registrar->tools[it->second];
        if (!t.fn) return false;
        t.fn(t.user_data);
        return true;
    }
}

using namespace gw::editor::bld_api;

// ---------------------------------------------------------------------------
GW_EDITOR_API const char* gw_editor_version() {
    return "gw_editor/0.7.0";
}

// ---------------------------------------------------------------------------
GW_EDITOR_API uint64_t gw_editor_get_primary_selection() {
    if (!g_globals.selection) return 0u;
    return g_globals.selection->primary().raw_bits();
}

GW_EDITOR_API uint32_t gw_editor_get_selection_count() {
    if (!g_globals.selection) return 0u;
    return g_globals.selection->count();
}

GW_EDITOR_API void gw_editor_set_selection(const uint64_t* handles,
                                            uint32_t        count) {
    if (!g_globals.selection || !handles || count == 0) return;
    g_globals.selection->clear();
    for (uint32_t i = 0; i < count; ++i)
        g_globals.selection->toggle(gw::ecs::Entity::from_raw_bits(handles[i]));
}

// ---------------------------------------------------------------------------
GW_EDITOR_API uint64_t gw_editor_create_entity(const char* name) {
    if (!g_globals.world) return 0u;
    const auto e = g_globals.world->create_entity();
    g_globals.world->add_component(e,
        gw::editor::scene::NameComponent{name ? name : "Entity"});
    g_globals.world->add_component(e, gw::editor::scene::TransformComponent{});
    return e.raw_bits();
}

GW_EDITOR_API bool gw_editor_destroy_entity(uint64_t handle) {
    if (!g_globals.world) return false;
    const gw::ecs::Entity e = gw::ecs::Entity::from_raw_bits(handle);
    if (!g_globals.world->is_alive(e)) return false;
    g_globals.world->destroy_entity(e);
    return true;
}

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_set_field(uint64_t    /*entity*/,
                                        const char* /*field_path*/,
                                        const char* /*value_json*/) {
    return false;  // TODO(Phase 8)
}

GW_EDITOR_API const char* gw_editor_get_field(uint64_t    /*entity*/,
                                               const char* /*field_path*/) {
    char* s = static_cast<char*>(std::malloc(3));
    if (s) { s[0]='n'; s[1]='u'; s[2]='\0'; }
    return s;
}

GW_EDITOR_API void gw_free_string(const char* s) {
    std::free(const_cast<char*>(s)); // NOLINT
}

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_undo() {
    if (!g_globals.cmd_stack || !g_globals.cmd_stack->can_undo()) return false;
    g_globals.cmd_stack->undo();
    return true;
}

GW_EDITOR_API bool gw_editor_redo() {
    if (!g_globals.cmd_stack || !g_globals.cmd_stack->can_redo()) return false;
    g_globals.cmd_stack->redo();
    return true;
}

GW_EDITOR_API const char* gw_editor_command_stack_summary(uint32_t max_entries) {
    char tmp[4096] = {};
    if (g_globals.cmd_stack)
        g_globals.cmd_stack->summary(tmp, sizeof(tmp), max_entries);
    const auto len = std::strlen(tmp);
    char* s = static_cast<char*>(std::malloc(len + 1));
    if (s) std::memcpy(s, tmp, len + 1);
    return s;
}

// ---------------------------------------------------------------------------
// Scene I/O — Phase 8 week 043. Bridges the C-ABI boundary into
// engine/scene/scene_file so BLD and the editor menu share one code path.
// The authoring World is the target on both sides; the caller guarantees
// thread-exclusive access (editor tick only, BLD tools run on the same
// thread by ADR-0007 §2.4).
namespace {

void ensure_editor_component_types(gw::ecs::World& w) {
    // Eagerly seed the registry so load_scene can resolve stable_hash →
    // live ComponentTypeId even in an empty editor world. Idempotent.
    (void)w.component_registry().id_of<gw::editor::scene::NameComponent>();
    (void)w.component_registry().id_of<gw::editor::scene::TransformComponent>();
    (void)w.component_registry().id_of<gw::editor::scene::VisibilityComponent>();
    (void)w.component_registry().id_of<gw::editor::scene::WorldMatrixComponent>();
    (void)w.component_registry().id_of<gw::ecs::HierarchyComponent>();
}

// Register editor-owned component migrations once, idempotently. Invoked
// lazily from gw_editor_load_scene so plain tests that never call load_scene
// don't pay the cost — and so BLD plugins can override migrations *before*
// first load by calling MigrationRegistry::global() themselves.
//
// v1→v2 TransformComponent: position widened from glm::vec3 (12 bytes) to
// glm::dvec3 (24 bytes). Everything else unchanged.
void register_editor_migrations_once() {
    static std::once_flag done;
    std::call_once(done, []{
        using gw::editor::scene::TransformComponent;
        // v1 layout was vec3+quat+vec3 = 40 bytes (no padding — vec3 has
        // 4-byte alignment so the trailing vec3 sits flush).
        constexpr std::uint32_t kV1Size = 12u + 16u + 12u;
        // v2 pulls `position` up to dvec3 (8-byte alignment), which forces the
        // whole struct to alignof(double). sizeof() therefore rounds up to the
        // next multiple of 8 — 52 logical bytes + 4 trailing padding = 56.
        static_assert(sizeof(TransformComponent) == 56u,
                      "TransformComponent v2 must land at 56 bytes "
                      "(dvec3+quat+vec3 + 4B trailing padding under 8B alignment)");

        gw::scene::MigrationRegistry::global().register_fn<TransformComponent>(
            kV1Size,
            [](std::span<const std::uint8_t> src,
               std::span<std::uint8_t>       dst) {
                if (src.size() != kV1Size ||
                    dst.size() != sizeof(TransformComponent)) {
                    return false;
                }
                // v1 layout: float pos[3] | float quat[4] | float scale[3]
                // v2 layout: double pos[3] | float quat[4] | float scale[3]
                float pos_f[3];
                std::memcpy(pos_f, src.data(), sizeof(pos_f));
                const std::uint8_t* src_after_pos = src.data() + sizeof(pos_f);

                TransformComponent t{};
                t.position = glm::dvec3(
                    static_cast<double>(pos_f[0]),
                    static_cast<double>(pos_f[1]),
                    static_cast<double>(pos_f[2]));
                std::memcpy(&t.rotation, src_after_pos, sizeof(t.rotation));
                std::memcpy(&t.scale,    src_after_pos + sizeof(t.rotation),
                            sizeof(t.scale));
                std::memcpy(dst.data(), &t, sizeof(t));
                return true;
            },
            "TransformComponent: widen position vec3->dvec3");
    });
}

} // namespace

GW_EDITOR_API bool gw_editor_save_scene(const char* path) {
    if (!g_globals.world || !path || path[0] == '\0') return false;
    std::filesystem::path p{path};
    auto r = gw::scene::save_scene(p, *g_globals.world);
    if (!r) {
        gw_editor_log_error(gw::scene::to_string(r.error()).data());
        return false;
    }
    return true;
}

GW_EDITOR_API bool gw_editor_load_scene(const char* path) {
    if (!g_globals.world || !path || path[0] == '\0') return false;
    std::filesystem::path p{path};
    ensure_editor_component_types(*g_globals.world);
    register_editor_migrations_once();
    auto r = gw::scene::load_scene(p, *g_globals.world);
    if (!r) {
        gw_editor_log_error(gw::scene::to_string(r.error()).data());
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_enter_play() { return false; }
GW_EDITOR_API bool gw_editor_stop()       { return false; }

// ---------------------------------------------------------------------------
// Registrar surface (ADR-0007 §2.4).
// ---------------------------------------------------------------------------
GW_EDITOR_API uint32_t bld_registrar_register_tool(BldRegistrar* reg,
                                                    const char*    id,
                                                    const char*    name,
                                                    const char*    tooltip_utf8,
                                                    const char*    icon_name,
                                                    BldToolFn      fn,
                                                    void*          user_data) {
    if (!reg || !id || !fn) return 0u;
    if (reg->tool_by_id.count(id)) return 0u;  // duplicate id
    BldRegistrar::Tool t;
    t.id        = id;
    t.name      = name         ? name         : "";
    t.tooltip   = tooltip_utf8 ? tooltip_utf8 : "";
    t.icon      = icon_name    ? icon_name    : "";
    t.fn        = fn;
    t.user_data = user_data;
    t.tool_id   = reg->next_tool_id++;
    const std::size_t idx = reg->tools.size();
    reg->tool_by_id.emplace(t.id, idx);
    reg->tools.push_back(std::move(t));
    return reg->tools.back().tool_id;
}

GW_EDITOR_API uint32_t bld_registrar_register_widget(BldRegistrar* reg,
                                                      uint64_t     component_hash,
                                                      BldWidgetFn  draw_fn,
                                                      void*        user_data) {
    if (!reg || !draw_fn) return 0u;
    BldRegistrar::Widget w;
    w.component_hash = component_hash;
    w.fn             = draw_fn;
    w.user_data      = user_data;
    w.widget_id      = reg->next_widget_id++;
    reg->widgets.push_back(w);
    return reg->widgets.back().widget_id;
}

GW_EDITOR_API bool gw_editor_run_tool(const char* tool_id) {
    return gw::editor::bld_api::invoke_tool(tool_id);
}

// ---------------------------------------------------------------------------
// Logging surface (ADR-0007 §2.4). When no sink is installed we fall back
// to stderr so BLD's early-init output isn't silently dropped.
// ---------------------------------------------------------------------------
GW_EDITOR_API void gw_editor_log(uint32_t level, const char* message_utf8) {
    if (!message_utf8) return;
    const uint32_t clamped = level > 2u ? 2u : level;
    if (g_globals.log_sink) {
        g_globals.log_sink(clamped, message_utf8);
        return;
    }
    static const char* const kLabels[] = {"info", "warn", "error"};
    std::fprintf(stderr, "[bld/%s] %s\n", kLabels[clamped], message_utf8);
}

GW_EDITOR_API void gw_editor_log_error(const char* message_utf8) {
    gw_editor_log(2u, message_utf8);
}

// ---------------------------------------------------------------------------
// Surface P v2 run_command — sequencer BLD tools (Phase 18-B).
// Opcode layout (stable, little-endian where applicable):
//   0x0001  create_sequence   payload: u32 fr, u32 dur, then null-terminated name
//   0x0002  add_transform     payload: u64 seq_handle_bits, u32 new_track_id (0 = auto)
//   0x0003  add_keyframe      (reserved — panel + CommandStack path; returns 0)
//   0x0004  play              payload: u64 seq_handle_bits, u8 loop
//   0x0005  export_summary    payload: u64 seq_handle_bits; writes JSON to g_globals.seq_tool_last_json
// ---------------------------------------------------------------------------
namespace {

[[nodiscard]] std::uint64_t next_synthetic_seq_bits() noexcept {
    // Synthetic handle namespace for in-memory-only gwseq blobs (64-bit).
    static std::uint64_t s = 0x0001'0000'0000'0001ull;
    return s++;
}

}  // namespace

std::uint64_t gw::editor::bld_api::dispatch_run_command(const std::uint32_t opcode, const void* payload,
                                                        const std::uint32_t bytes) noexcept {
    g_globals.seq_tool_last_json.clear();

    switch (opcode) {
        case 0x0001u: {
            if (bytes < 8u || g_globals.sequencer_world == nullptr) return 0u;
            const auto* p = static_cast<const std::uint8_t*>(payload);
            const std::uint32_t fr  = static_cast<std::uint32_t>(p[0]) |
                                     (static_cast<std::uint32_t>(p[1]) << 8u) |
                                     (static_cast<std::uint32_t>(p[2]) << 16u) |
                                     (static_cast<std::uint32_t>(p[3]) << 24u);
            const std::uint32_t dur = static_cast<std::uint32_t>(p[4]) |
                                     (static_cast<std::uint32_t>(p[5]) << 8u) |
                                     (static_cast<std::uint32_t>(p[6]) << 16u) |
                                     (static_cast<std::uint32_t>(p[7]) << 24u);
            const char* name = reinterpret_cast<const char*>(p + 8u);
            if (std::strlen(name) + 9u > bytes) return 0u;
            (void)name;  // name reserved for on-disk path once AssetDatabase wiring lands
            std::vector<std::uint8_t> blob;
            gw::seq::GwseqWriter      w(blob);
            if (!w.write_header(fr, dur).has_value()) return 0u;
            if (!w.finalise().has_value()) return 0u;
            const std::uint64_t       hbits = next_synthetic_seq_bits();
            gw::assets::AssetHandle   h{};
            h.bits = hbits;
            if (!g_globals.sequencer_world->register_sequence(h, std::move(blob)).has_value()) return 0u;
            return hbits;
        }
        case 0x0002u: {
            if (bytes < 12u || g_globals.sequencer_world == nullptr) return 0u;
            const auto* p  = static_cast<const std::uint8_t*>(payload);
            const auto  hb = static_cast<std::uint64_t>(p[0]) | (static_cast<std::uint64_t>(p[1]) << 8u) |
                            (static_cast<std::uint64_t>(p[2]) << 16u) |
                            (static_cast<std::uint64_t>(p[3]) << 24u) |
                            (static_cast<std::uint64_t>(p[4]) << 32u) |
                            (static_cast<std::uint64_t>(p[5]) << 40u) |
                            (static_cast<std::uint64_t>(p[6]) << 48u) |
                            (static_cast<std::uint64_t>(p[7]) << 56u);
            const std::uint32_t want = static_cast<std::uint32_t>(p[8]) |
                                      (static_cast<std::uint32_t>(p[9]) << 8u) |
                                      (static_cast<std::uint32_t>(p[10]) << 16u) |
                                      (static_cast<std::uint32_t>(p[11]) << 24u);
            gw::assets::AssetHandle in{};
            in.bits = hb;
            gw::seq::GwseqReader* r = g_globals.sequencer_world->reader_for(in);
            if (r == nullptr) return 0u;
            std::uint32_t new_id = want;
            if (new_id == 0u) {
                new_id = 1u;
                for (const auto& t : r->tracks()) {
                    if (t.track_id >= new_id) new_id = t.track_id + 1u;
                }
            }
            std::vector<std::uint8_t> out;
            if (const auto res = gw::editor::seq::append_empty_position_transform_track(*r, out, new_id);
                !res.has_value()) {
                return 0u;
            }
            g_globals.sequencer_world->unregister_sequence(in);
            if (!g_globals.sequencer_world->register_sequence(in, std::move(out)).has_value()) return 0u;
            return static_cast<std::uint64_t>(new_id);
        }
        case 0x0004u: {
            if (bytes < 9u || g_globals.world == nullptr) return 0u;
            const auto*         p   = static_cast<const std::uint8_t*>(payload);
            const std::uint64_t hb  = static_cast<std::uint64_t>(p[0]) | (static_cast<std::uint64_t>(p[1]) << 8u) |
                                     (static_cast<std::uint64_t>(p[2]) << 16u) |
                                     (static_cast<std::uint64_t>(p[3]) << 24u) |
                                     (static_cast<std::uint64_t>(p[4]) << 32u) |
                                     (static_cast<std::uint64_t>(p[5]) << 40u) |
                                     (static_cast<std::uint64_t>(p[6]) << 48u) |
                                     (static_cast<std::uint64_t>(p[7]) << 56u);
            const bool          loop = p[8] != 0u;
            gw::assets::AssetHandle h{};
            h.bits = hb;
            if (g_globals.seq_player_entity_bits == 0u) return 0u;
            const auto ent = gw::ecs::Entity::from_raw_bits(g_globals.seq_player_entity_bits);
            if (auto* pl = g_globals.world->get_component<gw::seq::SeqPlayerComponent>(ent)) {
                pl->seq_asset_id  = h;
                pl->playing       = true;
                pl->loop          = loop;
                pl->play_head_frame = 0.0;
            } else {
                return 0u;
            }
            return 1u;
        }
        case 0x0005u: {
            if (bytes < 8u || g_globals.sequencer_world == nullptr) return 0u;
            const auto* p  = static_cast<const std::uint8_t*>(payload);
            const auto  hb = static_cast<std::uint64_t>(p[0]) | (static_cast<std::uint64_t>(p[1]) << 8u) |
                            (static_cast<std::uint64_t>(p[2]) << 16u) |
                            (static_cast<std::uint64_t>(p[3]) << 24u) |
                            (static_cast<std::uint64_t>(p[4]) << 32u) |
                            (static_cast<std::uint64_t>(p[5]) << 40u) |
                            (static_cast<std::uint64_t>(p[6]) << 48u) |
                            (static_cast<std::uint64_t>(p[7]) << 56u);
            gw::assets::AssetHandle h{};
            h.bits = hb;
            gw::seq::GwseqReader* reader = g_globals.sequencer_world->reader_for(h);
            if (reader == nullptr) return 0u;
            gw::memory::ArenaAllocator arena(64u * 1024u);
            std::uint32_t            kf_total = 0u;
            std::ostringstream       o;
            o << "{\"frame_rate\":" << reader->header().frame_rate
              << ",\"duration_frames\":" << reader->header().duration_frames
              << ",\"tracks\":[";
            bool first = true;
            for (const auto& tr : reader->tracks()) {
                if (!first) o << ',';
                first = false;
                if (tr.track_type == gw::seq::GwseqTrackType::Transform) {
                    gw::seq::GwseqTrackView<gw::seq::Vec3_f64> v3{};
                    if (const auto rr = reader->read_track(tr.track_id, arena, v3);
                        rr.has_value() && !v3.empty()) {
                        o << "{\"id\":" << tr.track_id << ",\"kind\":\"transform_pos\",\"keyframes\":" << v3.frames.size()
                          << "}";
                        kf_total += static_cast<std::uint32_t>(v3.frames.size());
                    } else {
                        arena.reset();
                        gw::seq::GwseqTrackView<gw::seq::GwseqQuat> qv{};
                        if (const auto rq = reader->read_track(tr.track_id, arena, qv); rq.has_value()) {
                            o << "{\"id\":" << tr.track_id << ",\"kind\":\"transform_rot\",\"keyframes\":" << qv.frames.size()
                              << "}";
                            kf_total += static_cast<std::uint32_t>(qv.frames.size());
                        }
                    }
                } else if (tr.track_type == gw::seq::GwseqTrackType::Float) {
                    gw::seq::GwseqTrackView<float> fv{};
                    (void)reader->read_track(tr.track_id, arena, fv);
                    o << "{\"id\":" << tr.track_id << ",\"kind\":\"float\",\"keyframes\":" << fv.frames.size() << "}";
                    kf_total += static_cast<std::uint32_t>(fv.frames.size());
                } else {
                    o << "{\"id\":" << tr.track_id << ",\"kind\":\"other\",\"keyframes\":" << tr.keyframe_count
                      << "}";
                    kf_total += tr.keyframe_count;
                }
                arena.reset();
            }
            o << "],\"keyframe_count\":" << kf_total
              << ",\"duration_s\":" << (reader->header().frame_rate > 0u
                                            ? static_cast<double>(reader->header().duration_frames) /
                                                  static_cast<double>(reader->header().frame_rate)
                                            : 0.0)
              << "}";
            g_globals.seq_tool_last_json = o.str();
            bld_ffi_seq_export_set_json(g_globals.seq_tool_last_json.c_str());
            return 1u;
        }
        case 0x0003u: return 0u;
        default: return 0u;
    }
}
