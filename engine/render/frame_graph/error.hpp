#pragma once

#include <expected>
#include <string>
#include <vector>
#include <variant>

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

// Result type for frame graph operations
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
