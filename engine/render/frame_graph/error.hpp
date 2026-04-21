#pragma once

#include <expected>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace gw::render::frame_graph {

// Frame graph error types
enum class FrameGraphErrorType : uint8_t {
    CycleDetected = 0,
    UndeclaredResource = 1,
    InvalidResourceHandle = 2,
    InvalidPassHandle = 3,
    CompilationFailed = 4,
    ExecutionFailed = 5
};

// Detailed error information
struct FrameGraphError {
    FrameGraphErrorType type;
    std::string message;
    
    // Additional context for specific error types
    std::vector<std::string> context;  // e.g., passes in a cycle
    
    FrameGraphError(FrameGraphErrorType t, const std::string& msg)
        : type(t), message(msg) {}
    
    FrameGraphError(FrameGraphErrorType t, const std::string& msg, const std::vector<std::string>& ctx)
        : type(t), message(msg), context(ctx) {}
};

// Result type for frame graph operations.
//
// NOTE: `std::expected` in C++23 is not `[[nodiscard]]` on the type itself,
// and it cannot portably be wrapped in a derived class that still participates
// in `std::expected::and_then` / `or_else` / `transform` (those use a
// `_Is_specialization_v<T, std::expected>` concept that rejects subclasses —
// verified against MSVC STL 14.44). Therefore the discard-warning is applied
// at *function* granularity via `[[nodiscard]]` attributes on each API that
// returns a `Result<T>`. See docs/AUDIT_MAP_2026-04-20.md finding P3-1.
template<typename T>
using Result = std::expected<T, FrameGraphError>;

// Helper functions for creating errors
inline Result<std::monostate> make_cycle_error(const std::vector<std::string>& passes_in_cycle) {
    return std::unexpected(FrameGraphError{
        FrameGraphErrorType::CycleDetected,
        "Cycle detected in frame graph dependency",
        passes_in_cycle
    });
}

inline Result<std::monostate> make_undeclared_resource_error(const std::string& resource_name) {
    return std::unexpected(FrameGraphError{
        FrameGraphErrorType::UndeclaredResource,
        "Resource not declared: " + resource_name
    });
}

inline Result<std::monostate> make_invalid_handle_error(const std::string& handle_type, uint32_t handle) {
    return std::unexpected(FrameGraphError{
        FrameGraphErrorType::InvalidResourceHandle,
        "Invalid " + handle_type + " handle: " + std::to_string(handle)
    });
}

} // namespace gw::render::frame_graph
