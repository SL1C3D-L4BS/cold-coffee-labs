// engine/scripting/mod_loader.cpp — dynamic mod load + hot-reload for gameplay_module-style dylibs.

#include "engine/scripting/mod_api.hpp"

#include "engine/assets/asset_db.hpp"
#include "engine/core/log.hpp"
#include "engine/ecs/world.hpp"
#include "engine/platform/fs.hpp"

#include <charconv>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#if defined(_WIN32)
#  define MOD_DLL_SUFFIX ".dll"
#else
#  define MOD_DLL_SUFFIX ".so"
#endif

namespace gw {
namespace scripting {
namespace {

[[nodiscard]] std::string std_path(const Path& p) { return p.string(); }

[[nodiscard]] bool is_sp(const char c) noexcept { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

// Flat JSON: "key" : "value" (no backslash-escaped quotes).
[[nodiscard]] bool parse_quoted(const std::string& text, const char* key, std::string& out) {
    const std::string pat = std::string("\"") + key + "\"";
    const std::size_t     pos = text.find(pat);
    if (pos == std::string::npos) {
        return false;
    }
    std::size_t i = pos + pat.size();
    while (i < text.size() && is_sp(text[i])) ++i;
    if (i >= text.size() || text[i] != ':') {
        return false;
    }
    ++i;
    while (i < text.size() && is_sp(text[i])) ++i;
    if (i >= text.size() || text[i] != '"') {
        return false;
    }
    ++i;
    const std::size_t start = i;
    while (i < text.size() && text[i] != '"') {
        if (text[i] == '\\') {
            return false;
        }
        ++i;
    }
    if (i >= text.size() || text[i] != '"') {
        return false;
    }
    out.assign(text.substr(start, i - start));
    return !out.empty();
}

[[nodiscard]] bool parse_ver(const std::string& s, gw::version::SemVer& v) {
    v   = {};
    if (s.empty()) return false;
    std::uint32_t  a  = 0;
    std::uint32_t  b  = 0;
    std::uint32_t  c  = 0;
    const char*    p  = s.c_str();
    const char*    n  = p + s.size();
    const char*    d1 = static_cast<const char*>(std::memchr(p, '.', static_cast<std::size_t>(n - p)));
    if (d1 == nullptr) {
        return std::from_chars(p, n, a).ec == std::errc();
    }
    (void)std::from_chars(p, d1, a);
    const char* p2 = d1 + 1;
    const char* d2 = static_cast<const char*>(std::memchr(p2, '.', static_cast<std::size_t>(n - p2)));
    if (d2 == nullptr) {
        (void)std::from_chars(p2, n, b);
    } else {
        (void)std::from_chars(p2, d2, b);
        (void)std::from_chars(d2 + 1, n, c);
    }
    v.major = a;
    v.minor = b;
    v.patch = c;
    return true;
}

using OnLoadFn  = int (*)(gw::ecs::World*, gw::assets::AssetDatabase*);
using UnloadFn  = void (*)(gw::ecs::World*);
using OnTickFn  = void (*)(gw::ecs::World*, float);

}  // namespace

ModRegistry::~ModRegistry() {
    for (std::size_t i = 0; i < kMaxLoadedMods; ++i) {
        if (mods_[i].used) {
            unload_slot_(i, world_);
        }
    }
}

std::size_t ModRegistry::find_handle_slot_(const ModHandle h) const noexcept {
    for (std::size_t i = 0; i < kMaxLoadedMods; ++i) {
        if (mods_[i].used && mods_[i].id.value == h.value) {
            return i;
        }
    }
    return kMaxLoadedMods;
}

std::size_t ModRegistry::alloc_slot_() const noexcept {
    for (std::size_t i = 0; i < kMaxLoadedMods; ++i) {
        if (!mods_[i].used) {
            return i;
        }
    }
    return kMaxLoadedMods;
}

void ModRegistry::unload_slot_(const std::size_t slot, gw::ecs::World& world) noexcept {
    if (slot >= kMaxLoadedMods || !mods_[slot].used) {
        return;
    }
    auto& m = mods_[slot];
    if (m.runtime.on_unload) {
        m.runtime.on_unload(world);
    }
    const std::string vlog  = std::to_string(m.manifest.version.major) + "." + std::to_string(m.manifest.version.minor) + "." +
                            std::to_string(m.manifest.version.patch);
    const char* const mid  = m.manifest.mod_id.c_str() != nullptr ? m.manifest.mod_id.c_str() : "<unknown>";
    GW_LOG(Info, "mod", (std::string("mod unload mod_id=") + mid + " version " + vlog).c_str());

    m.runtime = IModRuntime{};
    m.lib     = gw::platform::DynamicLibrary{};
    m.last_mtime  = 0u;
    m.module_path  = Path{};
    m.manifest_path  = Path{};
    m.id     = ModHandle{};
    m.manifest   = ModManifest{};
    m.used  = false;
}

void ModRegistry::poll_hot_reload_(gw::ecs::World& world) {
    for (std::size_t i = 0; i < kMaxLoadedMods; ++i) {
        if (!mods_[i].used) {
            continue;
        }
        if (mods_[i].module_path.empty() || mods_[i].manifest_path.empty()) {
            continue;
        }
        const Path          man_path  = mods_[i].manifest_path;
        const Path          mod_path  = mods_[i].module_path;
        const std::string   mstr   = std_path(mod_path);
        const std::uint64_t st     = gw::platform::FileSystem::last_write_stamp(mod_path);
        if (st == 0u) {
            continue;
        }
        if (mods_[i].last_mtime != 0u && st != mods_[i].last_mtime) {
            GW_LOG(Info, "mod", (std::string("mod hot-reload: ") + mstr).c_str());
            unload_slot_(i, world);
            if (const auto re = load_mod(man_path); !re.has_value()) {
                GW_LOG(Error, "mod", (std::string("mod hot-reload re-load failed after ") + mstr).c_str());
            }
        }
    }
}

gw::core::Result<ModHandle, ModError> ModRegistry::load_mod(const Path& manifest_path) {
    const std::string path_str = std_path(manifest_path);
    if (path_str.empty()) {
        return gw::core::unexpected(ModError::OpenFailed);
    }
    if (gw::platform::FileSystem::last_write_stamp(manifest_path) == 0u) {
        GW_LOG(Error, "mod", (std::string("mod: manifest not found: ") + path_str).c_str());
        return gw::core::unexpected(ModError::OpenFailed);
    }

    std::ifstream in(manifest_path, std::ios::in | std::ios::binary);
    if (!in) {
        return gw::core::unexpected(ModError::OpenFailed);
    }
    std::stringstream buf;
    buf << in.rdbuf();
    const std::string json = std::move(buf).str();
    if (json.empty()) {
        return gw::core::unexpected(ModError::BadManifest);
    }

    std::string mod_id;
    std::string display;
    std::string ver;
    std::string eng;
    if (!parse_quoted(json, "mod_id", mod_id) || !parse_quoted(json, "version", ver) ||
        !parse_quoted(json, "engine_min_version", eng)) {
        return gw::core::unexpected(ModError::BadManifest);
    }
    if (!parse_quoted(json, "display_name", display)) {
        display = mod_id;
    }

    ModManifest man{};
    if (!man.mod_id.assign(mod_id) || !man.display_name.assign(display)) {
        return gw::core::unexpected(ModError::BadManifest);
    }
    if (!parse_ver(ver, man.version) || !parse_ver(eng, man.engine_min_version)) {
        return gw::core::unexpected(ModError::BadManifest);
    }

    if (!gw::version::operator>=(gw::version::ENGINE_VERSION, man.engine_min_version)) {
        return gw::core::unexpected(ModError::VersionMismatch);
    }

    std::string mod_rel;
    if (parse_quoted(json, "gameplay_module_path", mod_rel) && !mod_rel.empty()) {
        if (Path(mod_rel).is_relative()) {
            man.gameplay_module_path = manifest_path.parent_path() / mod_rel;
        } else {
            man.gameplay_module_path = mod_rel;
        }
    } else {
        man.gameplay_module_path = manifest_path.parent_path() / (std::string(mod_id) + MOD_DLL_SUFFIX);
    }

    const std::string dylib = std_path(man.gameplay_module_path);
    if (!gw::platform::FileSystem::exists(man.gameplay_module_path)) {
        GW_LOG(Error, "mod", (std::string("mod: module path missing: ") + dylib).c_str());
        return gw::core::unexpected(ModError::OpenFailed);
    }

    const std::size_t slot = alloc_slot_();
    if (slot >= kMaxLoadedMods) {
        return gw::core::unexpected(ModError::RegistryFull);
    }

    gw::platform::DynamicLibrary lib;
    if (!lib.open(dylib)) {
        return gw::core::unexpected(ModError::LoadFailed);
    }

    auto* const p_load  = reinterpret_cast<OnLoadFn>(lib.find_symbol("gw_mod_on_load"));
    auto* const p_unl   = reinterpret_cast<UnloadFn>(lib.find_symbol("gw_mod_on_unload"));
    auto* const p_tick  = reinterpret_cast<OnTickFn>(lib.find_symbol("gw_mod_on_tick"));
    if (p_load == nullptr || p_unl == nullptr || p_tick == nullptr) {
        return gw::core::unexpected(ModError::SymbolMissing);
    }

    IModRuntime rt{};
    rt.on_load  = [p_load](gw::ecs::World& w, gw::assets::AssetDatabase& a) -> ModLoadResult {
        const int c = p_load(&w, &a);
        if (c != 0) {
            return gw::core::unexpected(ModError::LoadFailed);
        }
        return std::monostate{};
    };
    rt.on_unload = [p_unl](gw::ecs::World& w) {
        p_unl(&w);
    };
    rt.on_tick   = [p_tick](gw::ecs::World& w, f32 d) { p_tick(&w, d); };

    if (const auto lr = rt.on_load(world_, assets_); !lr.has_value()) {
        return gw::core::unexpected(lr.error());
    }

    const ModHandle handle{next_id_++};
    mods_[slot].used  = true;
    mods_[slot].id  = handle;
    mods_[slot].manifest  = man;
    mods_[slot].runtime  = std::move(rt);
    mods_[slot].lib   = std::move(lib);
    mods_[slot].last_mtime  = gw::platform::FileSystem::last_write_stamp(man.gameplay_module_path);
    mods_[slot].manifest_path  = manifest_path;
    mods_[slot].module_path  = man.gameplay_module_path;

    const std::string vlog  = std::to_string(man.version.major) + "." + std::to_string(man.version.minor) + "." +
                            std::to_string(man.version.patch);
    GW_LOG(Info, "mod", (std::string("mod load mod_id=") + mod_id + " version " + vlog + " " + dylib).c_str());
    return handle;
}

void ModRegistry::unload_mod(const ModHandle handle) {
    if (!handle.valid()) {
        return;
    }
    const std::size_t s = find_handle_slot_(handle);
    if (s >= kMaxLoadedMods) {
        return;
    }
    unload_slot_(s, world_);
}

void ModRegistry::tick_all(gw::ecs::World& world, const f32 dt) {
    for (std::size_t i = 0; i < kMaxLoadedMods; ++i) {
        if (mods_[i].used && mods_[i].runtime.on_tick) {
            mods_[i].runtime.on_tick(world, dt);
        }
    }
    poll_hot_reload_(world);
}

bool try_parse_mod_manifest_from_json(const std::string_view json, ModManifest& out) noexcept {
    if (json.empty()) {
        return false;
    }
    const std::string j{json};

    std::string mod_id;
    std::string display;
    std::string ver;
    std::string eng;
    if (!parse_quoted(j, "mod_id", mod_id) || !parse_quoted(j, "version", ver) ||
        !parse_quoted(j, "engine_min_version", eng)) {
        return false;
    }
    if (!parse_quoted(j, "display_name", display)) {
        display = mod_id;
    }

    ModManifest man{};
    if (!man.mod_id.assign(mod_id) || !man.display_name.assign(display)) {
        return false;
    }
    if (!parse_ver(ver, man.version) || !parse_ver(eng, man.engine_min_version)) {
        return false;
    }
    out = std::move(man);
    return true;
}

}  // namespace scripting
}  // namespace gw
