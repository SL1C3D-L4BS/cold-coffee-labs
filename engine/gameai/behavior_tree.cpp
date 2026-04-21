// engine/gameai/behavior_tree.cpp — Phase 13 Wave 13E (ADR-0043).

#include "engine/gameai/behavior_tree.hpp"

#include "engine/physics/determinism_hash.hpp"

#include <cstring>
#include <string>

namespace gw::gameai {

void ActionRegistry::register_action(std::string_view name, ActionFn fn, void* user) {
    actions_[std::string{name}] = Entry{fn, user};
}
void ActionRegistry::register_condition(std::string_view name, ConditionFn fn, void* user) {
    conditions_[std::string{name}] = EntryC{fn, user};
}

BTStatus ActionRegistry::invoke_action(std::string_view name, BTContext& ctx) const {
    auto it = actions_.find(std::string{name});
    if (it == actions_.end() || it->second.fn == nullptr) return BTStatus::Failure;
    return it->second.fn(it->second.user, ctx);
}

bool ActionRegistry::invoke_condition(std::string_view name, BTContext& ctx) const {
    auto it = conditions_.find(std::string{name});
    if (it == conditions_.end() || it->second.fn == nullptr) return false;
    return it->second.fn(it->second.user, ctx);
}

bool ActionRegistry::has_action(std::string_view name) const noexcept {
    return actions_.find(std::string{name}) != actions_.end();
}
bool ActionRegistry::has_condition(std::string_view name) const noexcept {
    return conditions_.find(std::string{name}) != conditions_.end();
}

std::uint32_t validate_bt(const BTDesc& desc) noexcept {
    std::uint32_t err = 0;
    if (desc.nodes.empty()) { err |= kBT_V_EmptyTree; return err; }
    if (desc.root_index >= desc.nodes.size()) err |= kBT_V_RootOutOfRange;

    const std::size_t n = desc.nodes.size();
    for (std::size_t i = 0; i < n; ++i) {
        const auto& nd = desc.nodes[i];
        for (auto c : nd.children) {
            if (c >= n) err |= kBT_V_ChildOutOfRange;
            else if (c <= static_cast<std::uint16_t>(i)) err |= kBT_V_ChildIndexBeforeSelf;
        }
        switch (nd.kind) {
            case BTNodeKind::Sequence:
            case BTNodeKind::Selector:
            case BTNodeKind::Parallel:
                if (nd.children.empty()) err |= kBT_V_CompositeNoChildren;
                break;
            case BTNodeKind::Inverter:
            case BTNodeKind::Succeeder:
            case BTNodeKind::RepeatN:
            case BTNodeKind::UntilSuccess:
            case BTNodeKind::UntilFailure:
            case BTNodeKind::Cooldown:
                if (nd.children.size() != 1) err |= kBT_V_DecoratorNotOneChild;
                break;
            default: break;
        }
    }
    return err;
}

std::uint64_t bt_content_hash(const BTDesc& desc) noexcept {
    std::uint64_t h = gw::physics::kFnvOffset64;
    auto fold = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };

    const std::uint64_t name_len = desc.name.size();
    fold(&name_len, sizeof(name_len));
    fold(desc.name.data(), desc.name.size());
    fold(&desc.root_index, sizeof(desc.root_index));

    const std::uint64_t nn = desc.nodes.size();
    fold(&nn, sizeof(nn));
    for (const auto& nd : desc.nodes) {
        const std::uint8_t k = static_cast<std::uint8_t>(nd.kind);
        fold(&k, sizeof(k));
        const std::uint64_t nl = nd.name.size();
        fold(&nl, sizeof(nl));
        fold(nd.name.data(), nd.name.size());
        const std::uint64_t bk = nd.bb_key.size();
        fold(&bk, sizeof(bk));
        fold(nd.bb_key.data(), nd.bb_key.size());
        fold(&nd.f_value, sizeof(nd.f_value));
        fold(&nd.i_value, sizeof(nd.i_value));
        const std::uint8_t b = nd.b_value ? 1u : 0u;
        fold(&b, sizeof(b));
        fold(&nd.repeat_count, sizeof(nd.repeat_count));
        fold(&nd.cooldown_s,   sizeof(nd.cooldown_s));
        const std::uint64_t nc = nd.children.size();
        fold(&nc, sizeof(nc));
        if (!nd.children.empty()) fold(nd.children.data(), nd.children.size() * sizeof(std::uint16_t));
    }
    return h;
}

} // namespace gw::gameai
