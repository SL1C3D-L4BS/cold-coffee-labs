#pragma once
// engine/a11y/screen_reader.hpp — ADR-0072.

#include "engine/a11y/a11y_types.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::a11y {

class IScreenReader {
public:
    virtual ~IScreenReader() = default;

    virtual void update_tree(std::span<const AccessibleNode> nodes) = 0;
    virtual void announce_focus(std::uint64_t node_id) = 0;
    virtual void announce_text(std::string_view utf8, Politeness p) = 0;

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;

    // Introspection hooks (test-only).
    [[nodiscard]] virtual std::size_t tree_size() const noexcept = 0;
    [[nodiscard]] virtual std::size_t announce_count() const noexcept = 0;
    [[nodiscard]] virtual std::string_view last_announcement() const noexcept = 0;
};

[[nodiscard]] std::unique_ptr<IScreenReader> make_null_screen_reader();
[[nodiscard]] std::unique_ptr<IScreenReader> make_accesskit_screen_reader(); // GW_ENABLE_ACCESSKIT only
[[nodiscard]] std::unique_ptr<IScreenReader> make_screen_reader_aggregated(bool prefer_accesskit);

} // namespace gw::a11y
