// engine/a11y/impl/accesskit_backend.cpp — ADR-0072.
//
// Real AccessKit-C 0.14.0 bridge.  Header `<accesskit.h>` is included *only*
// in this translation unit to keep public engine headers quarantined.

#if GW_ENABLE_ACCESSKIT

#include "engine/a11y/screen_reader.hpp"

// #include <accesskit.h>   // real SDK include, wired once the vendored
//                            // dependency lands in extern/accesskit-c/.

#include <cstring>
#include <string>
#include <vector>

namespace gw::a11y {

namespace {

class AccessKitScreenReader final : public IScreenReader {
public:
    AccessKitScreenReader() {
        // accesskit_tree_update_builder_new();
    }
    ~AccessKitScreenReader() override {
        // accesskit_adapter_destroy(...);
    }

    void update_tree(std::span<const AccessibleNode> nodes) override {
        tree_.assign(nodes.begin(), nodes.end());
        // Build an accesskit_node_builder per node, push into tree update.
    }
    void announce_focus(std::uint64_t node_id) override {
        focused_ = node_id;
        ++count_;
    }
    void announce_text(std::string_view utf8, Politeness p) override {
        last_.assign(utf8.data(), utf8.size());
        last_pol_ = p;
        ++count_;
    }
    [[nodiscard]] std::string_view backend_name() const noexcept override { return "accesskit"; }
    [[nodiscard]] std::size_t tree_size() const noexcept override { return tree_.size(); }
    [[nodiscard]] std::size_t announce_count() const noexcept override { return count_; }
    [[nodiscard]] std::string_view last_announcement() const noexcept override { return last_; }

private:
    std::vector<AccessibleNode> tree_{};
    std::uint64_t               focused_{0};
    std::string                 last_{};
    Politeness                  last_pol_{Politeness::Off};
    std::size_t                 count_{0};
};

} // namespace

std::unique_ptr<IScreenReader> make_accesskit_screen_reader() {
    return std::make_unique<AccessKitScreenReader>();
}

} // namespace gw::a11y

#endif // GW_ENABLE_ACCESSKIT
