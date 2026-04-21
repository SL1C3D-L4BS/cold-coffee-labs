#pragma once
// editor/reflect/reflect.hpp
// Zero-heap, zero-virtual-dispatch macro-based field reflection.
// Spec ref: Phase 7 §3.
//
// Usage: place GW_REFLECT(TypeName, field1, field2, ...) inside the struct
// body AFTER the last field you want to reflect.
//
// Generates a `friend const TypeInfo& describe(T*)` (ADL-found).
// Inspector calls: const TypeInfo& ti = describe(static_cast<T*>(nullptr));
//
// Offset computation happens inside the describe() function body where T is
// already complete — this avoids the incomplete-type restriction of offsetof
// at class scope.

#include "engine/core/type_registry.hpp"
#include "engine/core/field_reflection.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>  // std::declval

namespace gw::editor::reflect {

using FieldInfo = gw::core::FieldDescriptor;
using TypeInfo  = gw::core::TypeInfo;
using FieldFlags = gw::core::FieldFlags;
using gw::core::fnv1a;

}  // namespace gw::editor::reflect

// ---------------------------------------------------------------------------
// Internal helpers — one FieldInfo initialiser per field.
// These are expanded INSIDE the describe() function where T is complete.
// ---------------------------------------------------------------------------
#define GW_FINFO_(T_, f_)                                              \
    ::gw::editor::reflect::FieldInfo {                                 \
        #f_,                                                           \
        ::gw::core::fnv1a(                                             \
            ::gw::core::detail::decorated_type_name<                   \
                decltype(std::declval<T_>().f_)>()),                   \
        static_cast<uint32_t>(offsetof(T_, f_)),                       \
        static_cast<uint32_t>(sizeof(((T_*)nullptr)->f_)),             \
        0u, 0.f, 0.f                                                   \
    }

// Variadic expanders — up to 16 fields.
#define GW_FI1_(T_,  a)                  GW_FINFO_(T_,a)
#define GW_FI2_(T_,  a,b)               GW_FINFO_(T_,a), GW_FINFO_(T_,b)
#define GW_FI3_(T_,  a,b,c)             GW_FI2_(T_,a,b), GW_FINFO_(T_,c)
#define GW_FI4_(T_,  a,b,c,d)           GW_FI3_(T_,a,b,c), GW_FINFO_(T_,d)
#define GW_FI5_(T_,  a,b,c,d,e)         GW_FI4_(T_,a,b,c,d), GW_FINFO_(T_,e)
#define GW_FI6_(T_,  a,b,c,d,e,f)       GW_FI5_(T_,a,b,c,d,e), GW_FINFO_(T_,f)
#define GW_FI7_(T_,  a,b,c,d,e,f,g)     GW_FI6_(T_,a,b,c,d,e,f), GW_FINFO_(T_,g)
#define GW_FI8_(T_,  a,b,c,d,e,f,g,h)   GW_FI7_(T_,a,b,c,d,e,f,g), GW_FINFO_(T_,h)
#define GW_FI9_(T_,  a,...)              GW_FINFO_(T_,a), GW_FI8_(T_, __VA_ARGS__)
#define GW_FI10_(T_, a,...)              GW_FINFO_(T_,a), GW_FI9_(T_, __VA_ARGS__)
#define GW_FI11_(T_, a,...)              GW_FINFO_(T_,a), GW_FI10_(T_, __VA_ARGS__)
#define GW_FI12_(T_, a,...)              GW_FINFO_(T_,a), GW_FI11_(T_, __VA_ARGS__)
#define GW_FI13_(T_, a,...)              GW_FINFO_(T_,a), GW_FI12_(T_, __VA_ARGS__)
#define GW_FI14_(T_, a,...)              GW_FINFO_(T_,a), GW_FI13_(T_, __VA_ARGS__)
#define GW_FI15_(T_, a,...)              GW_FINFO_(T_,a), GW_FI14_(T_, __VA_ARGS__)
#define GW_FI16_(T_, a,...)              GW_FINFO_(T_,a), GW_FI15_(T_, __VA_ARGS__)

// Count helper.
#define GW_REFL_COUNT_(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N
#define GW_REFL_COUNT(...)                                             \
    GW_REFL_COUNT_(__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define GW_FI_DISPATCH_(N_, T_, ...) GW_FI##N_##_(T_, __VA_ARGS__)
#define GW_FI_DISPATCH(N_, T_, ...)  GW_FI_DISPATCH_(N_, T_, __VA_ARGS__)

// ---------------------------------------------------------------------------
// GW_REFLECT(TypeName, field1, field2, ...)
//
// Placed inside the struct body after the last field.
// The describe() function body sees a COMPLETE T_ (all fields declared above
// the macro), so offsetof is valid there.
// ---------------------------------------------------------------------------
#define GW_REFLECT(T_, ...)                                                \
    friend const ::gw::editor::reflect::TypeInfo&                          \
    describe(T_*) noexcept {                                               \
        static const std::array<::gw::editor::reflect::FieldInfo,          \
                                GW_REFL_COUNT(__VA_ARGS__)>                 \
        kFields = {                                                        \
            GW_FI_DISPATCH(GW_REFL_COUNT(__VA_ARGS__), T_, __VA_ARGS__)   \
        };                                                                 \
        static const ::gw::editor::reflect::TypeInfo kInfo{               \
            #T_,                                                           \
            ::gw::core::fnv1a(#T_),                                        \
            std::span<const ::gw::editor::reflect::FieldInfo>{kFields}    \
        };                                                                 \
        return kInfo;                                                      \
    }
