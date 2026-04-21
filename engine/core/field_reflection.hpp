#pragma once
// engine/core/field_reflection.hpp
// Lightweight offset/size-based field descriptor — zero heap, no virtual
// dispatch, no RTTI.  Used by the engine's serialization primitives.
//
// The editor-facing reflection with GW_REFLECT macro and widget dispatch lives
// in editor/reflect/reflect.hpp (Phase 7 §3).  This engine-side header is the
// minimal substrate shared by both.

#include "type_registry.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <array>

namespace gw::core {

// ---------------------------------------------------------------------------
// FieldDescriptor
// ---------------------------------------------------------------------------
struct FieldDescriptor {
    const char* name    = nullptr;
    uint32_t    type_id = 0;      // FNV1a of the field's type name
    uint32_t    offset  = 0;      // offsetof(struct, field)
    uint32_t    size    = 0;      // sizeof(field type)
    uint32_t    flags   = 0;      // widget hint bits (range, color, angle, …)
    float       min_val = 0.f;
    float       max_val = 0.f;
};

// Field flag bits (matches editor/reflect/reflect.hpp FieldFlags).
enum FieldFlags : uint32_t {
    kFieldRange    = 1u << 0,
    kFieldReadOnly = 1u << 1,
    kFieldColor    = 1u << 2,
    kFieldAngle    = 1u << 3,
    kFieldHidden   = 1u << 4,
};

// ---------------------------------------------------------------------------
// TypeInfo — a complete reflected type, returned by describe(T*) (ADL).
// ---------------------------------------------------------------------------
struct TypeInfo {
    const char*               type_name = nullptr;
    uint32_t                  type_id   = 0;
    std::span<const FieldDescriptor> fields;
};

// ---------------------------------------------------------------------------
// Helpers for pointer-to-member → offsetof equivalent.
// Usage:  field_offset<MyStruct, float>(&MyStruct::my_field)
// ---------------------------------------------------------------------------
template<typename Struct, typename Field>
[[nodiscard]] constexpr uint32_t field_offset(Field Struct::* pm) noexcept {
    // C++17 offsetof alternative via null pointer trick — defined behaviour
    // for standard-layout types; UB for non-standard-layout (use offsetof).
    const Struct* s = nullptr;
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    return static_cast<uint32_t>(
        reinterpret_cast<const uint8_t*>(&(s->*pm)) -
        reinterpret_cast<const uint8_t*>(s));
}

}  // namespace gw::core
