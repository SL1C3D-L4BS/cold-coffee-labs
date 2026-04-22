// engine/render/material/material_instance.cpp — ADR-0075 Wave 17B.

#include "engine/render/material/material_instance.hpp"
#include "engine/render/material/material_template.hpp"

#include <cstring>

namespace gw::render::material {

namespace {

constexpr std::uint64_t kFnvOffset64 = 0xcbf29ce484222325ull;
constexpr std::uint64_t kFnvPrime64  = 0x00000100000001b3ull;
constexpr std::uint64_t kFib64       = 0x9E3779B97F4A7C15ull;

inline std::uint64_t fnv1a(const std::byte* bytes, std::size_t n) noexcept {
    std::uint64_t h = kFnvOffset64;
    for (std::size_t i = 0; i < n; ++i) {
        h ^= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[i]));
        h *= kFnvPrime64;
    }
    return h;
}

inline std::uint64_t diffuse(std::uint64_t h) noexcept {
    h ^= h >> 27;
    h *= kFib64;
    h ^= h >> 31;
    return h;
}

} // namespace

MaterialInstance::MaterialInstance(MaterialInstanceId id,
                                    MaterialInstanceState* s,
                                    const MaterialTemplate* t)
    : id_(id), state_(s), tmpl_(t) {}

bool MaterialInstance::set_f32(std::string_view key, float v) {
    if (!state_ || !tmpl_) return false;
    ParameterValue pv{};
    pv.kind = ParameterValue::Kind::F32;
    pv.f    = v;
    if (!tmpl_->write_param(state_->block, key, pv)) return false;
    touch();
    return true;
}

bool MaterialInstance::set_vec4(std::string_view key, float x, float y, float z, float w) {
    if (!state_ || !tmpl_) return false;
    ParameterValue pv{};
    pv.kind = ParameterValue::Kind::Vec4;
    pv.v    = {x, y, z, w};
    if (!tmpl_->write_param(state_->block, key, pv)) return false;
    touch();
    return true;
}

bool MaterialInstance::set_i32(std::string_view key, std::int32_t v) {
    if (!state_ || !tmpl_) return false;
    ParameterValue pv{};
    pv.kind = ParameterValue::Kind::I32;
    pv.i    = v;
    if (!tmpl_->write_param(state_->block, key, pv)) return false;
    touch();
    return true;
}

bool MaterialInstance::set_texture(std::uint32_t slot, TextureHandle h, std::uint32_t fingerprint) {
    if (!state_) return false;
    if (slot >= state_->textures.size()) return false;
    state_->textures[slot] = TextureSlot{h, fingerprint};
    touch();
    return true;
}

bool MaterialInstance::get(std::string_view key, ParameterValue& out) const {
    if (!state_ || !tmpl_) return false;
    return tmpl_->read_param(state_->block, key, out);
}

void MaterialInstance::touch() noexcept {
    if (!state_) return;
    std::uint64_t h = fnv1a(state_->block.bytes.data(), state_->block.bytes.size());
    for (const auto& ts : state_->textures) {
        const auto id = ts.handle.value;
        const unsigned char b[sizeof(id)] = {
            static_cast<unsigned char>(id         & 0xFFu),
            static_cast<unsigned char>((id >> 8)  & 0xFFu),
            static_cast<unsigned char>((id >> 16) & 0xFFu),
            static_cast<unsigned char>((id >> 24) & 0xFFu),
            static_cast<unsigned char>((id >> 32) & 0xFFu),
            static_cast<unsigned char>((id >> 40) & 0xFFu),
            static_cast<unsigned char>((id >> 48) & 0xFFu),
            static_cast<unsigned char>((id >> 56) & 0xFFu),
        };
        for (auto c : b) { h ^= c; h *= kFnvPrime64; }
    }
    state_->content_hash = diffuse(h);
    ++state_->revision;
}

std::uint64_t MaterialInstance::content_hash() const noexcept {
    return state_ ? state_->content_hash : 0;
}

} // namespace gw::render::material
