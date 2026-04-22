// engine/vfx/ribbons/ribbon_state.cpp — ADR-0078 Wave 17E.

#include "engine/vfx/ribbons/ribbon_state.hpp"

namespace gw::vfx::ribbons {

void RibbonState::append(const std::array<float, 3>& position, float dt_seconds) {
    acc_ += dt_seconds;
    if (acc_ < desc_.node_period_s) return;
    acc_ = 0.0f;

    RibbonVertex v{};
    v.position = position;
    v.age      = 0.0f;
    const auto i = static_cast<float>(verts_.size());
    v.uv       = {i / static_cast<float>(desc_.max_length), 0.0f};
    v.width    = desc_.width;
    verts_.push_back(v);

    if (verts_.size() > desc_.max_length) {
        verts_.erase(verts_.begin());
    }
}

void RibbonState::advance(float dt_seconds) {
    for (auto& v : verts_) v.age += dt_seconds;
    while (!verts_.empty() && verts_.front().age > desc_.lifetime_s) {
        verts_.erase(verts_.begin());
    }
}

} // namespace gw::vfx::ribbons
