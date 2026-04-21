#pragma once
// editor/scene/components.hpp
// Phase 7 scene components — the minimum viable component set the editor
// operates on. Hand-written with GW_REFLECT so the Inspector renders fields
// automatically and the ComponentRegistry picks up their TypeInfo via ADL.
//
// Spec ref: Phase 7 §4, §9, §10.
//
// Future: Phase 8 will migrate these to `engine/scene/` once GW_REFLECT moves
// into `engine/core/` (reflect.hpp is currently editor-scoped by history).

#include "editor/reflect/reflect.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace gw::editor::scene {

// ---------------------------------------------------------------------------
// NameComponent — fixed 64-char display name.
// Trivially copyable; no heap allocation. Edited via the Inspector's first
// row and displayed in the Outliner.
// ---------------------------------------------------------------------------
struct NameComponent {
    std::array<char, 64> value{};

    NameComponent() noexcept {
        value[0] = '\0';
    }

    explicit NameComponent(std::string_view name) noexcept {
        const auto n = name.size() < value.size() - 1 ? name.size() : value.size() - 1;
        std::memcpy(value.data(), name.data(), n);
        value[n] = '\0';
    }

    [[nodiscard]] const char* c_str() const noexcept { return value.data(); }
    [[nodiscard]] char*       data()  noexcept       { return value.data(); }

    // NOTE: no GW_REFLECT — NameComponent is rendered by the Inspector via a
    // dedicated `InputText` widget (not a field-generic one). The outliner
    // reads `.c_str()` directly.
};

// ---------------------------------------------------------------------------
// TransformComponent — world-local translation / rotation / scale.
// For Phase 7 this is single-precision; Phase 8 will migrate to double for
// world-space per CLAUDE.md non-negotiable #17 (floating origin).
// ---------------------------------------------------------------------------
struct TransformComponent {
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};
    glm::vec3 scale   {1.f, 1.f, 1.f};

    GW_REFLECT(TransformComponent, position, rotation, scale)
};

// ---------------------------------------------------------------------------
// VisibilityComponent — simple boolean flag used by the outliner to dim rows
// and by the renderer (Phase 8) to cull entities out of the forward pass.
// ---------------------------------------------------------------------------
struct VisibilityComponent {
    bool visible = true;
    // Pad to 4 bytes so the struct is trivially copyable with a predictable
    // alignment regardless of the target platform ABI.
    std::uint8_t _pad[3]{};

    GW_REFLECT(VisibilityComponent, visible)
};

static_assert(sizeof(NameComponent)        == 64);
static_assert(sizeof(VisibilityComponent)  == 4);

} // namespace gw::editor::scene
