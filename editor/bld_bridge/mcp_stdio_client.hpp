#pragma once
// editor/bld_bridge/mcp_stdio_client.hpp
// Minimal Content-Length framed JSON-RPC over bidirectional pipes to
// `gw_bld_server` (ADR-0011). Used by the BLD Copilot panel when the MCP
// provider is selected.

#include <memory>
#include <string>

namespace gw::editor::bld {

/// Escape a UTF-8 string for inclusion inside JSON string quotes.
[[nodiscard]] std::string json_escape_string(std::string_view s);

/// Owns a `gw_bld_server` child with redirected stdio; synchronous request/response.
class McpStdioSession final {
public:
    McpStdioSession();
    ~McpStdioSession();

    McpStdioSession(const McpStdioSession&)            = delete;
    McpStdioSession& operator=(const McpStdioSession&) = delete;
    McpStdioSession(McpStdioSession&&) noexcept;
    McpStdioSession& operator=(McpStdioSession&&) noexcept;

    /// Spawns `server_exe` with `--project-root` set to `project_root_utf8`.
    [[nodiscard]] bool start(const std::string& server_exe,
                             const std::string& project_root_utf8,
                             std::string&       err_out);

    void stop() noexcept;

    [[nodiscard]] bool active() const noexcept { return impl_ != nullptr; }

    /// Writes one framed JSON-RPC body (UTF-8) and reads until a full framed response arrives.
    [[nodiscard]] bool request(const std::string& request_body_utf8,
                               std::string&       response_body_out,
                               std::string&       err_out);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace gw::editor::bld
