#pragma once

#include <cstddef>
#include <string_view>

namespace gw::editor::shell {

[[nodiscard]] bool env_skip_private_gate() noexcept;

/// `%APPDATA%/GreywaterEditor/private_gate.toml` (or XDG equivalent).
[[nodiscard]] bool private_gate_file_exists() noexcept;

/// Returns true when authentication succeeds. On failure, writes a short
/// message into `err` (NUL-terminated when `err_cap > 0`).
[[nodiscard]] bool try_private_gate(std::string_view username,
                                    std::string_view password, char* err,
                                    std::size_t err_cap) noexcept;

} // namespace gw::editor::shell
