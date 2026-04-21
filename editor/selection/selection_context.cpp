// editor/selection/selection_context.cpp
#include "selection_context.hpp"

#include <algorithm>

namespace gw::editor {

void SelectionContext::set(EntityHandle h) {
    if (h == kNullEntity) { clear(); return; }
    selected_.clear();
    selected_.push_back(h);
    notify();
}

void SelectionContext::toggle(EntityHandle h) {
    auto it = std::find(selected_.begin(), selected_.end(), h);
    if (it != selected_.end()) {
        selected_.erase(it);
    } else {
        selected_.push_back(h);
    }
    notify();
}

void SelectionContext::clear() {
    if (selected_.empty()) return;
    selected_.clear();
    notify();
}

bool SelectionContext::is_selected(EntityHandle h) const noexcept {
    return std::find(selected_.begin(), selected_.end(), h) != selected_.end();
}

void SelectionContext::notify() const {
    for (const auto& cb : listeners_)
        cb(*this);
}

}  // namespace gw::editor
