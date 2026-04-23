#pragma once

#include <string_view>

namespace gw {
namespace core {

[[noreturn]] void assert_fail(const char* expr, const char* file, int line, std::string_view message);

[[noreturn]] void unimplemented_fail(const char* tag, const char* file, int line, std::string_view message);

}  // namespace core
}  // namespace gw

#define GW_ASSERT(expr, msg)                                    \
    do {                                                        \
        if (!(expr)) {                                          \
            ::gw::core::assert_fail(#expr, __FILE__, __LINE__, (msg)); \
        }                                                       \
    } while (false)

// GW_UNIMPLEMENTED(tag, message)
// Mark a code path that has not been implemented yet. Hitting it at runtime
// aborts with a structured diagnostic so stubs cannot silently succeed.
// `tag` is a short tracking identifier (e.g. "P20-FRAMEGRAPH-ALIASING") that
// can be grep'd from logs and issue trackers.
#define GW_UNIMPLEMENTED(tag, msg)                                         \
    ::gw::core::unimplemented_fail((tag), __FILE__, __LINE__, (msg))

// GW_UNREACHABLE — the compiler's unreachable hint, enforced at runtime in
// debug builds by an abort. Use at the end of an exhaustive switch or when
// the type system guarantees no control flow reaches here.
#if defined(NDEBUG)
#  if defined(__clang__) || defined(__GNUC__)
#    define GW_UNREACHABLE() __builtin_unreachable()
#  elif defined(_MSC_VER)
#    define GW_UNREACHABLE() __assume(0)
#  else
#    define GW_UNREACHABLE() ((void)0)
#  endif
#else
#  define GW_UNREACHABLE() ::gw::core::assert_fail("GW_UNREACHABLE", __FILE__, __LINE__, "reached an unreachable path")
#endif
