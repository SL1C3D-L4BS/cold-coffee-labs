// editor/bld_api/editor_bld_api.cpp
// Thread-safe C-ABI surface for BLD (Phase 9) — Surface P, v1.
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md.
#include "editor_bld_api.hpp"

// Full type definitions required — the header only forward-declares these.
#include "editor/selection/selection_context.hpp"
#include "editor/scene/components.hpp"
#include "editor/undo/command_stack.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/scene_file.hpp"
#include "engine/scene/seq/gwseq_codec.hpp"
#include "engine/scene/seq/sequencer_world.hpp"
#include "engine/assets/asset_handle.hpp"
#include "engine/memory/arena_allocator.hpp"
#include "editor/scene/authoring_scene_load.hpp"
#include "editor/scene/components.hpp"
#include "editor/seq/seq_gwseq_rebuild.hpp"
#include "bld/include/bld_ffi_seq_export.h"

#include <cctype>
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

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// ---------------------------------------------------------------------------
// Registrar — opaque handle concrete type (ADR-0007 §2.4). Declared in the
// bld_register.h header as `struct BldRegistrar` forward; this TU defines it.
// Intentionally minimal for v1: only tool/widget tables. User data pointers
// are BLD-owned; we keep the pointer but never dereference or free it.
// ---------------------------------------------------------------------------
struct BldEditorHandle {};  // v1: zero-sized tag; BLD may cast for future state

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

namespace {

[[nodiscard]] const char* skip_ws(const char* p) noexcept {
    while (p && *p != '\0' && std::isspace(static_cast<unsigned char>(*p))) {
        ++p;
    }
    return p;
}

[[nodiscard]] char* alloc_json(std::string_view sv) {
    char* s = static_cast<char*>(std::malloc(sv.size() + 1));
    if (!s) {
        return nullptr;
    }
    std::memcpy(s, sv.data(), sv.size());
    s[sv.size()] = '\0';
    return s;
}

void append_json_string_escaped(std::string& out, std::string_view sv) {
    for (unsigned char uc : sv) {
        const char c = static_cast<char>(uc);
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (uc < 0x20u) {
                char buf[7];
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-signed-bitwise)
                std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(uc));
                out += buf;
            } else {
                out += c;
            }
            break;
        }
    }
}

[[nodiscard]] const char* alloc_json_quoted_string(std::string_view sv) {
    std::string j;
    j.reserve(sv.size() + 2 + 8);
    j.push_back('"');
    append_json_string_escaped(j, sv);
    j.push_back('"');
    return alloc_json(j);
}

[[nodiscard]] int hex_digit_value(char ch) noexcept {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

void push_utf8_codepoint(std::string& out, std::uint32_t cp) {
    if (cp <= 0x7Fu) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | ((cp >> 6u) & 0x1Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else if (cp <= 0xFFFFu) {
        out.push_back(static_cast<char>(0xE0u | ((cp >> 12u) & 0x0Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6u) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else if (cp <= 0x10FFFFu) {
        out.push_back(static_cast<char>(0xF0u | ((cp >> 18u) & 0x07u)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 12u) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6u) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    }
}

[[nodiscard]] bool parse_bool_json(const char* json, bool& out) noexcept {
    const char* p = skip_ws(json);
    if (std::strncmp(p, "true", 4) == 0) {
        out = true;
        return true;
    }
    if (std::strncmp(p, "false", 5) == 0) {
        out = false;
        return true;
    }
    return false;
}

[[nodiscard]] bool parse_u8_json(const char* json, std::uint8_t& out) noexcept {
    const char* p = skip_ws(json);
    char*       e = nullptr;
    const long  v = std::strtol(p, &e, 10);
    if (e == p || v < 0 || v > 255) {
        return false;
    }
    out = static_cast<std::uint8_t>(v);
    return true;
}

[[nodiscard]] bool parse_string_json(const char* json, std::string& out) {
    out.clear();
    const char* p = skip_ws(json);
    if (*p == '"') {
        ++p;
        while (*p != '\0') {
            if (*p == '"') {
                return true;
            }
            if (*p == '\\') {
                ++p;
                if (*p == '\0') {
                    return false;
                }
                switch (*p) {
                case '"':
                    out.push_back('"');
                    break;
                case '\\':
                    out.push_back('\\');
                    break;
                case '/':
                    out.push_back('/');
                    break;
                case 'b':
                    out.push_back('\b');
                    break;
                case 'f':
                    out.push_back('\f');
                    break;
                case 'n':
                    out.push_back('\n');
                    break;
                case 'r':
                    out.push_back('\r');
                    break;
                case 't':
                    out.push_back('\t');
                    break;
                case 'u': {
                    std::uint32_t cp = 0;
                    for (int k = 0; k < 4; ++k) {
                        ++p;
                        const int hv = hex_digit_value(*p);
                        if (hv < 0) {
                            return false;
                        }
                        cp = (cp << 4u) | static_cast<std::uint32_t>(hv);
                    }
                    push_utf8_codepoint(out, cp);
                    break;
                }
                default:
                    // Loose: preserve unknown escapes as the escaped character.
                    out.push_back(*p);
                    break;
                }
                ++p;
                continue;
            }
            out.push_back(*p);
            ++p;
        }
        return false;
    }
    // Unquoted UTF-8 fallback for tooling.
    out = p;
    return !out.empty();
}

[[nodiscard]] bool parse_vec3d_json(const char* json, glm::dvec3& o) noexcept {
    double        x = 0.0;
    double        y = 0.0;
    double        z = 0.0;
    const char*   p = skip_ws(json);
    const int     n = std::sscanf(p, " [ %lf , %lf , %lf ]", &x, &y, &z);
    if (n == 3) {
        o = glm::dvec3{x, y, z};
        return true;
    }
    const int n2 = std::sscanf(p, "[%lf,%lf,%lf]", &x, &y, &z);
    if (n2 == 3) {
        o = glm::dvec3{x, y, z};
        return true;
    }
    return false;
}

[[nodiscard]] bool parse_vec3f_json(const char* json, glm::vec3& o) noexcept {
    float       x = 0.f;
    float       y = 0.f;
    float       z = 0.f;
    const char* p = skip_ws(json);
    const int   n = std::sscanf(p, " [ %f , %f , %f ]", &x, &y, &z);
    if (n == 3) {
        o = glm::vec3{x, y, z};
        return true;
    }
    const int n2 = std::sscanf(p, "[%f,%f,%f]", &x, &y, &z);
    if (n2 == 3) {
        o = glm::vec3{x, y, z};
        return true;
    }
    return false;
}

[[nodiscard]] bool parse_quat_json(const char* json, glm::quat& o) noexcept {
    float       w = 1.f;
    float       x = 0.f;
    float       y = 0.f;
    float       z = 0.f;
    const char* p = skip_ws(json);
    const int   n = std::sscanf(p, " [ %f , %f , %f , %f ]", &w, &x, &y, &z);
    if (n == 4) {
        o = glm::quat{w, x, y, z};
        return true;
    }
    const int n2 = std::sscanf(p, "[%f,%f,%f,%f]", &w, &x, &y, &z);
    if (n2 == 4) {
        o = glm::quat{w, x, y, z};
        return true;
    }
    return false;
}

[[nodiscard]] bool split_component_field(const char* field_path, std::string& comp, std::string& field) {
    if (!field_path) {
        return false;
    }
    const char* dot = std::strchr(field_path, '.');
    if (!dot || dot == field_path) {
        return false;
    }
    comp.assign(field_path, dot);
    field.assign(dot + 1);
    return !comp.empty() && !field.empty();
}

} // namespace

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
GW_EDITOR_API bool gw_editor_set_field(uint64_t entity, const char* field_path, const char* value_json) {
    if (!g_globals.world || !field_path || !value_json) {
        return false;
    }
    gw::editor::scene::ensure_authoring_scene_types_for_load(*g_globals.world);

    const gw::ecs::Entity e = gw::ecs::Entity::from_raw_bits(entity);
    if (!g_globals.world->is_alive(e)) {
        return false;
    }

    std::string comp;
    std::string field;
    if (!split_component_field(field_path, comp, field)) {
        // NameComponent has no GW_REFLECT — accept `Name.value` or bare `Name`.
        if (field_path && std::strcmp(field_path, "Name") == 0) {
            auto* nc = g_globals.world->get_component<gw::editor::scene::NameComponent>(e);
            if (!nc) {
                return false;
            }
            std::string v;
            if (!parse_string_json(value_json, v)) {
                return false;
            }
            const std::size_t n = std::min(v.size(), nc->value.size() - 1);
            std::memcpy(nc->value.data(), v.data(), n);
            nc->value[n] = '\0';
            return true;
        }
        return false;
    }

    if (comp == "TransformComponent") {
        auto* tr = g_globals.world->get_component<gw::editor::scene::TransformComponent>(e);
        if (!tr) {
            return false;
        }
        if (field == "position") {
            return parse_vec3d_json(value_json, tr->position);
        }
        if (field == "scale") {
            return parse_vec3f_json(value_json, tr->scale);
        }
        if (field == "rotation") {
            return parse_quat_json(value_json, tr->rotation);
        }
        return false;
    }

    if (comp == "VisibilityComponent") {
        auto* vis = g_globals.world->get_component<gw::editor::scene::VisibilityComponent>(e);
        if (!vis || field != "visible") {
            return false;
        }
        bool b = false;
        if (!parse_bool_json(value_json, b)) {
            return false;
        }
        vis->visible = b;
        return true;
    }

    if (comp == "BlockoutPrimitiveComponent") {
        auto* bp = g_globals.world->get_component<gw::editor::scene::BlockoutPrimitiveComponent>(e);
        if (!bp) {
            return false;
        }
        if (field == "shape") {
            return parse_u8_json(value_json, bp->shape);
        }
        if (field == "gwmat_rel") {
            std::string v;
            if (!parse_string_json(value_json, v)) {
                return false;
            }
            const std::size_t n = std::min(v.size(), bp->gwmat_rel.size() - 1);
            std::memcpy(bp->gwmat_rel.data(), v.data(), n);
            bp->gwmat_rel[n] = '\0';
            return true;
        }
        return false;
    }

    if (comp == "WorldMatrixComponent") {
        auto* wm = g_globals.world->get_component<gw::editor::scene::WorldMatrixComponent>(e);
        if (!wm) {
            return false;
        }
        if (field == "dirty") {
            return parse_u8_json(value_json, wm->dirty);
        }
        // `world` is cache output of the transform system — not mutable via Surface P.
        return false;
    }

    if (comp == "NameComponent") {
        if (field != "value") {
            return false;
        }
        auto* nc = g_globals.world->get_component<gw::editor::scene::NameComponent>(e);
        if (!nc) {
            return false;
        }
        std::string v;
        if (!parse_string_json(value_json, v)) {
            return false;
        }
        const std::size_t n = std::min(v.size(), nc->value.size() - 1);
        std::memcpy(nc->value.data(), v.data(), n);
        nc->value[n] = '\0';
        return true;
    }

    return false;
}

GW_EDITOR_API const char* gw_editor_get_field(uint64_t entity, const char* field_path) {
    if (!g_globals.world || !field_path) {
        return alloc_json("null");
    }
    gw::editor::scene::ensure_authoring_scene_types_for_load(*g_globals.world);
    const gw::ecs::Entity e = gw::ecs::Entity::from_raw_bits(entity);
    if (!g_globals.world->is_alive(e)) {
        return alloc_json("null");
    }

    std::string comp;
    std::string field;
    if (!split_component_field(field_path, comp, field)) {
        if (std::strcmp(field_path, "Name") == 0) {
            if (const auto* nc = g_globals.world->get_component<gw::editor::scene::NameComponent>(e)) {
                return alloc_json_quoted_string(nc->c_str());
            }
        }
        return alloc_json("null");
    }

    if (comp == "TransformComponent") {
        const auto* tr = g_globals.world->get_component<gw::editor::scene::TransformComponent>(e);
        if (!tr) {
            return alloc_json("null");
        }
        char buf[256]{};
        if (field == "position") {
            std::snprintf(buf, sizeof buf, "[%g,%g,%g]", tr->position.x, tr->position.y, tr->position.z);
            return alloc_json(std::string_view{buf});
        }
        if (field == "scale") {
            std::snprintf(buf, sizeof buf, "[%g,%g,%g]", static_cast<double>(tr->scale.x),
                static_cast<double>(tr->scale.y), static_cast<double>(tr->scale.z));
            return alloc_json(std::string_view{buf});
        }
        if (field == "rotation") {
            std::snprintf(buf, sizeof buf, "[%g,%g,%g,%g]", static_cast<double>(tr->rotation.w),
                static_cast<double>(tr->rotation.x), static_cast<double>(tr->rotation.y),
                static_cast<double>(tr->rotation.z));
            return alloc_json(std::string_view{buf});
        }
        return alloc_json("null");
    }

    if (comp == "VisibilityComponent" && field == "visible") {
        const auto* vis = g_globals.world->get_component<gw::editor::scene::VisibilityComponent>(e);
        if (!vis) {
            return alloc_json("null");
        }
        return alloc_json(vis->visible ? "true" : "false");
    }

    if (comp == "BlockoutPrimitiveComponent" && field == "shape") {
        const auto* bp = g_globals.world->get_component<gw::editor::scene::BlockoutPrimitiveComponent>(e);
        if (!bp) {
            return alloc_json("null");
        }
        char b[32]{};
        std::snprintf(b, sizeof b, "%u", static_cast<unsigned>(bp->shape));
        return alloc_json(std::string_view{b});
    }

    if (comp == "BlockoutPrimitiveComponent" && field == "gwmat_rel") {
        if (const auto* bp = g_globals.world->get_component<gw::editor::scene::BlockoutPrimitiveComponent>(e)) {
            return alloc_json_quoted_string(bp->gwmat_rel.data());
        }
    }

    if (comp == "WorldMatrixComponent") {
        const auto* wm = g_globals.world->get_component<gw::editor::scene::WorldMatrixComponent>(e);
        if (!wm) {
            return alloc_json("null");
        }
        if (field == "dirty") {
            char b[32]{};
            std::snprintf(b, sizeof b, "%u", static_cast<unsigned>(wm->dirty));
            return alloc_json(std::string_view{b});
        }
        if (field == "world") {
            std::string j = "[";
            const glm::dmat4& m = wm->world;
            for (int c = 0; c < 4; ++c) {
                for (int r = 0; r < 4; ++r) {
                    if (c != 0 || r != 0) {
                        j += ',';
                    }
                    char num[48]{};
                    std::snprintf(num, sizeof num, "%.17g", m[c][r]);
                    j += num;
                }
            }
            j += ']';
            return alloc_json(j);
        }
    }

    if (comp == "NameComponent" && field == "value") {
        if (const auto* nc = g_globals.world->get_component<gw::editor::scene::NameComponent>(e)) {
            return alloc_json_quoted_string(nc->c_str());
        }
    }

    return alloc_json("null");
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
// Scene I/O — `gw::scene::save_scene` / `load_scene` over the authoring world.
// BLD and the editor File menu share this path. See
// `docs/operations/bld_set_field_v1_matrix.md` for the field surface; scene
// paths are caller-provided (relative to project or absolute). The caller
// guarantees thread-exclusive access (editor tick, BLD on the same thread;
// ADR-0007 §2.4).
GW_EDITOR_API bool gw_editor_save_scene(const char* path) {
    if (!g_globals.world || !path || path[0] == '\0') return false;
    std::filesystem::path p{path};
    auto r = gw::scene::save_scene(p, *g_globals.world);
    if (!r) {
        gw_editor_log_error(gw::scene::to_string(r.error()).data());
        return false;
    }
    g_globals.active_scene_path_utf8 = path;
    return true;
}

GW_EDITOR_API bool gw_editor_load_scene(const char* path) {
    if (!g_globals.world || !path || path[0] == '\0') return false;
    std::filesystem::path p{path};
    gw::editor::scene::ensure_authoring_scene_types_for_load(*g_globals.world);
    gw::editor::scene::register_authoring_scene_migrations_once();
    auto r = gw::scene::load_scene(p, *g_globals.world);
    if (!r) {
        gw_editor_log_error(gw::scene::to_string(r.error()).data());
        return false;
    }
    g_globals.active_scene_path_utf8 = path;
    return true;
}

GW_EDITOR_API const char* gw_editor_active_scene_path() {
    return g_globals.active_scene_path_utf8.c_str();
}

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_enter_play() {
    if (!g_globals.pie_enter_play || !g_globals.pie_host_context) return false;
    return g_globals.pie_enter_play(g_globals.pie_host_context);
}

GW_EDITOR_API bool gw_editor_stop() {
    if (!g_globals.pie_stop_play || !g_globals.pie_host_context) return false;
    g_globals.pie_stop_play(g_globals.pie_host_context);
    return true;
}

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
