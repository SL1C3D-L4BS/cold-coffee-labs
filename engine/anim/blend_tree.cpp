// engine/anim/blend_tree.cpp — Phase 13 Wave 13C (ADR-0041).

#include "engine/anim/blend_tree.hpp"

#include "engine/physics/determinism_hash.hpp"

#include <cstring>

namespace gw::anim {

std::uint32_t validate_blend_tree(const BlendTreeDesc& desc) noexcept {
    std::uint32_t err = 0;
    const std::size_t n = desc.nodes.size();

    for (std::size_t i = 0; i < n; ++i) {
        const auto& nd = desc.nodes[i];
        if (nd.weight < 0.0f || nd.weight > 1.0f) err |= kBT_WeightOutOfRange;
        for (auto c : nd.children) {
            if (c >= i) err |= kBT_ChildAfterSelf;
        }
        if (nd.kind == BlendKind::Lerp || nd.kind == BlendKind::Additive ||
            nd.kind == BlendKind::MaskLerp) {
            if (nd.children.size() != 2) err |= kBT_ChildCountMismatch;
        }
        if (nd.kind == BlendKind::OneMinus && nd.children.size() != 1) err |= kBT_ChildCountMismatch;
        if (nd.kind == BlendKind::MaskLerp && nd.mask_index >= desc.masks.size()) {
            err |= kBT_MaskIndexOutOfRange;
        }
    }

    for (const auto& s : desc.states) {
        if (s.node_index >= n) err |= kBT_StateNodeOutOfRange;
    }
    for (const auto& t : desc.transitions) {
        if (t.from_state >= desc.states.size() ||
            t.to_state   >= desc.states.size()) err |= kBT_TransitionStateOutOfRange;
    }
    if (!desc.states.empty() && desc.default_state >= desc.states.size()) {
        err |= kBT_DefaultStateOutOfRange;
    }

    return err;
}

std::uint64_t blend_tree_content_hash(const BlendTreeDesc& desc) noexcept {
    std::uint64_t h = gw::physics::kFnvOffset64;

    auto fold_bytes = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };

    const std::uint64_t name_len = desc.name.size();
    fold_bytes(&name_len, sizeof(name_len));
    fold_bytes(desc.name.data(), desc.name.size());
    fold_bytes(&desc.target_skeleton_hash, sizeof(desc.target_skeleton_hash));
    fold_bytes(&desc.root_index, sizeof(desc.root_index));

    const std::uint64_t nnodes = desc.nodes.size();
    fold_bytes(&nnodes, sizeof(nnodes));
    for (const auto& nd : desc.nodes) {
        const std::uint8_t k = static_cast<std::uint8_t>(nd.kind);
        fold_bytes(&k, sizeof(k));
        fold_bytes(&nd.input_index, sizeof(nd.input_index));
        fold_bytes(&nd.mask_index,  sizeof(nd.mask_index));
        fold_bytes(&nd.weight,      sizeof(nd.weight));
        const std::uint64_t nc = nd.children.size();
        fold_bytes(&nc, sizeof(nc));
        if (!nd.children.empty()) {
            fold_bytes(nd.children.data(), nd.children.size() * sizeof(std::uint16_t));
        }
    }

    const std::uint64_t nmasks = desc.masks.size();
    fold_bytes(&nmasks, sizeof(nmasks));
    for (const auto& m : desc.masks) {
        const std::uint64_t sz = m.size();
        fold_bytes(&sz, sizeof(sz));
        if (!m.empty()) fold_bytes(m.data(), m.size() * sizeof(float));
    }

    const std::uint64_t nstates = desc.states.size();
    fold_bytes(&nstates, sizeof(nstates));
    for (const auto& s : desc.states) {
        const std::uint64_t nm = s.name.size();
        fold_bytes(&nm, sizeof(nm));
        fold_bytes(s.name.data(), s.name.size());
        fold_bytes(&s.node_index, sizeof(s.node_index));
    }

    const std::uint64_t ntrans = desc.transitions.size();
    fold_bytes(&ntrans, sizeof(ntrans));
    for (const auto& t : desc.transitions) {
        fold_bytes(&t.from_state, sizeof(t.from_state));
        fold_bytes(&t.to_state,   sizeof(t.to_state));
        const std::uint64_t tn = t.trigger.size();
        fold_bytes(&tn, sizeof(tn));
        fold_bytes(t.trigger.data(), t.trigger.size());
        fold_bytes(&t.duration_s, sizeof(t.duration_s));
    }

    fold_bytes(&desc.default_state, sizeof(desc.default_state));
    return h;
}

} // namespace gw::anim
