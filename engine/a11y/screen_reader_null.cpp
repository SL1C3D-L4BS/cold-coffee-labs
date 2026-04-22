// engine/a11y/screen_reader_null.cpp — ADR-0072 null backend.

#include "engine/a11y/screen_reader.hpp"

#include <string>
#include <vector>

namespace gw::a11y {

namespace {

class NullScreenReader final : public IScreenReader {
public:
    void update_tree(std::span<const AccessibleNode> nodes) override {
        tree_.assign(nodes.begin(), nodes.end());
    }
    void announce_focus(std::uint64_t node_id) override {
        for (const auto& n : tree_) {
            if (n.node_id == node_id) {
                last_ = n.name_utf8;
                last_pol_ = Politeness::Polite;
                ++count_;
                return;
            }
        }
    }
    void announce_text(std::string_view utf8, Politeness p) override {
        last_.assign(utf8.data(), utf8.size());
        last_pol_ = p;
        ++count_;
    }
    [[nodiscard]] std::string_view backend_name() const noexcept override { return "null"; }
    [[nodiscard]] std::size_t tree_size() const noexcept override { return tree_.size(); }
    [[nodiscard]] std::size_t announce_count() const noexcept override { return count_; }
    [[nodiscard]] std::string_view last_announcement() const noexcept override { return last_; }

private:
    std::vector<AccessibleNode> tree_{};
    std::string                 last_{};
    Politeness                  last_pol_{Politeness::Off};
    std::size_t                 count_{0};
};

} // namespace

std::unique_ptr<IScreenReader> make_null_screen_reader() {
    return std::make_unique<NullScreenReader>();
}

std::unique_ptr<IScreenReader> make_screen_reader_aggregated(bool prefer_accesskit) {
#if GW_ENABLE_ACCESSKIT
    if (prefer_accesskit) {
        if (auto p = make_accesskit_screen_reader()) return p;
    }
#else
    (void)prefer_accesskit;
#endif
    return make_null_screen_reader();
}

#if !GW_ENABLE_ACCESSKIT
std::unique_ptr<IScreenReader> make_accesskit_screen_reader() { return nullptr; }
#endif

} // namespace gw::a11y
