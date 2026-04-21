// engine/core/config/cvar_registry.cpp — ADR-0024 Wave 11B.

#include "engine/core/config/cvar_registry.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>

namespace gw::config {

namespace {

bool type_matches(CVarType t, CVarType want) noexcept {
    if (t == want) return true;
    // Enum CVars are stored as int32 — accept Int32 reads/writes.
    if (want == CVarType::Int32 && t == CVarType::Enum) return true;
    return false;
}

template <typename T>
T clamp_range(T v, double lo, double hi) {
    const double d = static_cast<double>(v);
    const double c = std::clamp(d, lo, hi);
    return static_cast<T>(c);
}

} // namespace

bool CVarRegistry::parse_bool_(std::string_view s, bool& out) noexcept {
    auto eq_ci = [](std::string_view a, const char* b) {
        std::size_t i = 0;
        for (; b[i] != '\0' && i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i]))
                != std::tolower(static_cast<unsigned char>(b[i]))) return false;
        }
        return b[i] == '\0' && i == a.size();
    };
    if (eq_ci(s, "true") || eq_ci(s, "1") || eq_ci(s, "on") || eq_ci(s, "yes")) {
        out = true; return true;
    }
    if (eq_ci(s, "false") || eq_ci(s, "0") || eq_ci(s, "off") || eq_ci(s, "no")) {
        out = false; return true;
    }
    return false;
}

CVarId CVarRegistry::insert_(const std::string& name, CVarType t,
                             std::uint32_t flags, std::string_view desc) {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].name == name) {
            return CVarId{static_cast<std::uint32_t>(i + 1)};
        }
    }
    CVarEntry e;
    e.name = name;
    e.description.assign(desc.data(), desc.size());
    e.type = t;
    e.flags = flags;
    entries_.push_back(std::move(e));
    return CVarId{static_cast<std::uint32_t>(entries_.size())};
}

void CVarRegistry::publish_change_(CVarId id, std::uint32_t origin) noexcept {
    ++version_;
    if (!bus_) return;
    events::ConfigCVarChanged ev;
    ev.cvar_id = id.value;
    ev.origin  = origin;
    bus_->publish(ev);
}

CVarRef<bool> CVarRegistry::register_bool(const CVarInit<bool>& init) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto id = insert_(std::string{init.toml_path}, CVarType::Bool,
                      init.flags, init.description);
    auto& e = entries_[id.value - 1];
    e.v_bool = init.default_value;
    e.d_bool = init.default_value;
    return CVarRef<bool>{id};
}

CVarRef<std::int32_t> CVarRegistry::register_i32(const CVarInit<std::int32_t>& init) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto id = insert_(std::string{init.toml_path}, CVarType::Int32,
                      init.flags, init.description);
    auto& e = entries_[id.value - 1];
    e.v_i32 = init.default_value;
    e.d_i32 = init.default_value;
    if (init.has_range) {
        e.has_range = true;
        e.lo = static_cast<double>(init.min_value);
        e.hi = static_cast<double>(init.max_value);
    }
    return CVarRef<std::int32_t>{id};
}

CVarRef<std::int64_t> CVarRegistry::register_i64(const CVarInit<std::int64_t>& init) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto id = insert_(std::string{init.toml_path}, CVarType::Int64,
                      init.flags, init.description);
    auto& e = entries_[id.value - 1];
    e.v_i64 = init.default_value;
    e.d_i64 = init.default_value;
    if (init.has_range) {
        e.has_range = true;
        e.lo = static_cast<double>(init.min_value);
        e.hi = static_cast<double>(init.max_value);
    }
    return CVarRef<std::int64_t>{id};
}

CVarRef<float> CVarRegistry::register_f32(const CVarInit<float>& init) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto id = insert_(std::string{init.toml_path}, CVarType::Float,
                      init.flags, init.description);
    auto& e = entries_[id.value - 1];
    e.v_f32 = init.default_value;
    e.d_f32 = init.default_value;
    if (init.has_range) {
        e.has_range = true;
        e.lo = static_cast<double>(init.min_value);
        e.hi = static_cast<double>(init.max_value);
    }
    return CVarRef<float>{id};
}

CVarRef<double> CVarRegistry::register_f64(const CVarInit<double>& init) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto id = insert_(std::string{init.toml_path}, CVarType::Double,
                      init.flags, init.description);
    auto& e = entries_[id.value - 1];
    e.v_f64 = init.default_value;
    e.d_f64 = init.default_value;
    if (init.has_range) {
        e.has_range = true;
        e.lo = init.min_value;
        e.hi = init.max_value;
    }
    return CVarRef<double>{id};
}

CVarRef<std::string> CVarRegistry::register_string(const CVarInit<std::string>& init) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto id = insert_(std::string{init.toml_path}, CVarType::String,
                      init.flags, init.description);
    auto& e = entries_[id.value - 1];
    e.v_str = init.default_value;
    e.d_str = init.default_value;
    return CVarRef<std::string>{id};
}

CVarRef<std::int32_t> CVarRegistry::register_enum(std::string_view name,
                                                  std::span<const EnumVariant> variants,
                                                  std::int32_t default_value,
                                                  std::uint32_t flags,
                                                  std::string_view desc) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto id = insert_(std::string{name}, CVarType::Enum, flags, desc);
    auto& e = entries_[id.value - 1];
    e.v_i32 = default_value;
    e.d_i32 = default_value;
    e.enum_variants.assign(variants.begin(), variants.end());
    return CVarRef<std::int32_t>{id};
}

CVarId CVarRegistry::find(std::string_view name) const noexcept {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].name == name) {
            return CVarId{static_cast<std::uint32_t>(i + 1)};
        }
    }
    return CVarId{};
}

const CVarEntry* CVarRegistry::entry(CVarId id) const noexcept {
    if (!id.valid()) return nullptr;
    if (id.value > entries_.size()) return nullptr;
    return &entries_[id.value - 1];
}
const CVarEntry* CVarRegistry::entry_by_name(std::string_view name) const noexcept {
    return entry(find(name));
}

std::optional<bool> CVarRegistry::get_bool(CVarId id) const noexcept {
    const auto* e = entry(id);
    if (!e || e->type != CVarType::Bool) return std::nullopt;
    return e->v_bool;
}
std::optional<std::int32_t> CVarRegistry::get_i32(CVarId id) const noexcept {
    const auto* e = entry(id);
    if (!e || !type_matches(e->type, CVarType::Int32)) return std::nullopt;
    return e->v_i32;
}
std::optional<std::int64_t> CVarRegistry::get_i64(CVarId id) const noexcept {
    const auto* e = entry(id);
    if (!e || e->type != CVarType::Int64) return std::nullopt;
    return e->v_i64;
}
std::optional<float> CVarRegistry::get_f32(CVarId id) const noexcept {
    const auto* e = entry(id);
    if (!e || e->type != CVarType::Float) return std::nullopt;
    return e->v_f32;
}
std::optional<double> CVarRegistry::get_f64(CVarId id) const noexcept {
    const auto* e = entry(id);
    if (!e || e->type != CVarType::Double) return std::nullopt;
    return e->v_f64;
}
std::optional<std::string> CVarRegistry::get_string(CVarId id) const {
    const auto* e = entry(id);
    if (!e || e->type != CVarType::String) return std::nullopt;
    return e->v_str;
}

bool CVarRegistry::get_bool_or(std::string_view n, bool fb) const noexcept {
    auto v = get_bool(find(n)); return v.value_or(fb);
}
std::int32_t CVarRegistry::get_i32_or(std::string_view n, std::int32_t fb) const noexcept {
    auto v = get_i32(find(n)); return v.value_or(fb);
}
float CVarRegistry::get_f32_or(std::string_view n, float fb) const noexcept {
    auto v = get_f32(find(n)); return v.value_or(fb);
}
double CVarRegistry::get_f64_or(std::string_view n, double fb) const noexcept {
    auto v = get_f64(find(n)); return v.value_or(fb);
}
std::string CVarRegistry::get_string_or(std::string_view n, std::string_view fb) const {
    auto v = get_string(find(n));
    return v.value_or(std::string{fb});
}

bool CVarRegistry::set_bool(CVarId id, bool v, std::uint32_t origin) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (id.value == 0 || id.value > entries_.size()) return false;
    auto& e = entries_[id.value - 1];
    if (e.type != CVarType::Bool) return false;
    if ((e.flags & kCVarReadOnly) && origin != kOriginProgrammatic) return false;
    if (e.v_bool == v) { publish_change_(id, origin); return true; }
    e.v_bool = v;
    publish_change_(id, origin);
    return true;
}
bool CVarRegistry::set_i32(CVarId id, std::int32_t v, std::uint32_t origin) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (id.value == 0 || id.value > entries_.size()) return false;
    auto& e = entries_[id.value - 1];
    if (!type_matches(e.type, CVarType::Int32)) return false;
    if ((e.flags & kCVarReadOnly) && origin != kOriginProgrammatic) return false;
    if (e.has_range) v = clamp_range<std::int32_t>(v, e.lo, e.hi);
    e.v_i32 = v;
    publish_change_(id, origin);
    return true;
}
bool CVarRegistry::set_i64(CVarId id, std::int64_t v, std::uint32_t origin) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (id.value == 0 || id.value > entries_.size()) return false;
    auto& e = entries_[id.value - 1];
    if (e.type != CVarType::Int64) return false;
    if ((e.flags & kCVarReadOnly) && origin != kOriginProgrammatic) return false;
    if (e.has_range) v = clamp_range<std::int64_t>(v, e.lo, e.hi);
    e.v_i64 = v;
    publish_change_(id, origin);
    return true;
}
bool CVarRegistry::set_f32(CVarId id, float v, std::uint32_t origin) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (id.value == 0 || id.value > entries_.size()) return false;
    auto& e = entries_[id.value - 1];
    if (e.type != CVarType::Float) return false;
    if ((e.flags & kCVarReadOnly) && origin != kOriginProgrammatic) return false;
    if (e.has_range) v = clamp_range<float>(v, e.lo, e.hi);
    e.v_f32 = v;
    publish_change_(id, origin);
    return true;
}
bool CVarRegistry::set_f64(CVarId id, double v, std::uint32_t origin) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (id.value == 0 || id.value > entries_.size()) return false;
    auto& e = entries_[id.value - 1];
    if (e.type != CVarType::Double) return false;
    if ((e.flags & kCVarReadOnly) && origin != kOriginProgrammatic) return false;
    if (e.has_range) v = clamp_range<double>(v, e.lo, e.hi);
    e.v_f64 = v;
    publish_change_(id, origin);
    return true;
}
bool CVarRegistry::set_string(CVarId id, std::string_view v, std::uint32_t origin) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (id.value == 0 || id.value > entries_.size()) return false;
    auto& e = entries_[id.value - 1];
    if (e.type != CVarType::String) return false;
    if ((e.flags & kCVarReadOnly) && origin != kOriginProgrammatic) return false;
    e.v_str.assign(v.data(), v.size());
    publish_change_(id, origin);
    return true;
}

CVarRegistry::SetResult CVarRegistry::set_from_string(std::string_view name,
                                                     std::string_view value,
                                                     std::uint32_t origin) {
    auto id = find(name);
    if (!id.valid()) return SetResult::NotFound;
    // Copy entry pointer while unlocked — entries_ isn't reallocated after register.
    const auto* e = entry(id);
    if (!e) return SetResult::NotFound;
    if (e->flags & kCVarReadOnly) {
        if (origin != kOriginProgrammatic) return SetResult::ReadOnly;
    }
    switch (e->type) {
        case CVarType::Bool: {
            bool b = false;
            if (!parse_bool_(value, b)) return SetResult::ParseError;
            return set_bool(id, b, origin) ? SetResult::Ok : SetResult::TypeMismatch;
        }
        case CVarType::Int32: {
            std::int32_t v = 0;
            auto [p, ec] = std::from_chars(value.data(), value.data() + value.size(), v);
            if (ec != std::errc{} || p != value.data() + value.size()) return SetResult::ParseError;
            if (e->has_range && (v < static_cast<std::int32_t>(e->lo) || v > static_cast<std::int32_t>(e->hi)))
                return SetResult::OutOfRange;
            return set_i32(id, v, origin) ? SetResult::Ok : SetResult::TypeMismatch;
        }
        case CVarType::Int64: {
            std::int64_t v = 0;
            auto [p, ec] = std::from_chars(value.data(), value.data() + value.size(), v);
            if (ec != std::errc{} || p != value.data() + value.size()) return SetResult::ParseError;
            return set_i64(id, v, origin) ? SetResult::Ok : SetResult::TypeMismatch;
        }
        case CVarType::Float: {
            // std::from_chars for float is available in C++17 but spotty on
            // some libcxx. Use strtof via null-terminated buffer for safety.
            std::string buf{value};
            char* endp = nullptr;
            float v = std::strtof(buf.c_str(), &endp);
            if (!endp || *endp != '\0') return SetResult::ParseError;
            if (e->has_range && (static_cast<double>(v) < e->lo || static_cast<double>(v) > e->hi))
                return SetResult::OutOfRange;
            return set_f32(id, v, origin) ? SetResult::Ok : SetResult::TypeMismatch;
        }
        case CVarType::Double: {
            std::string buf{value};
            char* endp = nullptr;
            double v = std::strtod(buf.c_str(), &endp);
            if (!endp || *endp != '\0') return SetResult::ParseError;
            if (e->has_range && (v < e->lo || v > e->hi))
                return SetResult::OutOfRange;
            return set_f64(id, v, origin) ? SetResult::Ok : SetResult::TypeMismatch;
        }
        case CVarType::String:
            return set_string(id, value, origin) ? SetResult::Ok : SetResult::TypeMismatch;
        case CVarType::Enum: {
            for (const auto& ev : e->enum_variants) {
                if (ev.label == value) {
                    return set_i32(id, ev.value, origin) ? SetResult::Ok : SetResult::TypeMismatch;
                }
            }
            // Accept a numeric fallback for robustness.
            std::int32_t v = 0;
            auto [p, ec] = std::from_chars(value.data(), value.data() + value.size(), v);
            if (ec != std::errc{} || p != value.data() + value.size()) return SetResult::ParseError;
            return set_i32(id, v, origin) ? SetResult::Ok : SetResult::TypeMismatch;
        }
    }
    return SetResult::TypeMismatch;
}

void CVarRegistry::reset_to_default(CVarId id, std::uint32_t origin) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (id.value == 0 || id.value > entries_.size()) return;
    auto& e = entries_[id.value - 1];
    switch (e.type) {
        case CVarType::Bool:   e.v_bool = e.d_bool; break;
        case CVarType::Int32:
        case CVarType::Enum:   e.v_i32  = e.d_i32;  break;
        case CVarType::Int64:  e.v_i64  = e.d_i64;  break;
        case CVarType::Float:  e.v_f32  = e.d_f32;  break;
        case CVarType::Double: e.v_f64  = e.d_f64;  break;
        case CVarType::String: e.v_str  = e.d_str;  break;
    }
    publish_change_(id, origin);
}

void CVarRegistry::reset_domain(std::string_view prefix, std::uint32_t origin) {
    // No lock here; we issue individual reset calls which lock internally.
    std::vector<CVarId> to_reset;
    to_reset.reserve(entries_.size());
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].name.rfind(prefix, 0) == 0) {
            to_reset.push_back(CVarId{static_cast<std::uint32_t>(i + 1)});
        }
    }
    for (auto id : to_reset) reset_to_default(id, origin);
}

bool CVarRegistry::cheats_tripped() const noexcept {
    for (const auto& e : entries_) {
        if (!(e.flags & kCVarCheat)) continue;
        switch (e.type) {
            case CVarType::Bool:   if (e.v_bool != e.d_bool) return true; break;
            case CVarType::Int32:
            case CVarType::Enum:   if (e.v_i32  != e.d_i32)  return true; break;
            case CVarType::Int64:  if (e.v_i64  != e.d_i64)  return true; break;
            case CVarType::Float:  if (e.v_f32  != e.d_f32)  return true; break;
            case CVarType::Double: if (e.v_f64  != e.d_f64)  return true; break;
            case CVarType::String: if (e.v_str  != e.d_str)  return true; break;
        }
    }
    return false;
}

} // namespace gw::config
