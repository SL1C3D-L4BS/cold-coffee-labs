// engine/narrative/voice_director.cpp — Phase 22 scaffold.

#include "engine/narrative/voice_director.hpp"

#include <cstdint>

namespace gw::narrative {

namespace {

// SplitMix64 — cheap, deterministic, seeded hash for line selection.
// Pure function, no global state. docs/05 §14 rule 3 compliance.
std::uint64_t splitmix64(std::uint64_t x) noexcept {
    x += 0x9E3779B97F4A7C15ULL;
    x  = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x  = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

bool circle_gate_allows(std::uint32_t mask, std::uint32_t circle) noexcept {
    return mask == 0 || ((mask >> (circle & 31u)) & 1u) != 0;
}

bool sig_in_bounds(const SinSignature& sig, const DialogueLine& ln) noexcept {
    const auto cr = static_cast<std::uint8_t>(sig.cruelty_ratio   * 255.f);
    const auto pr = static_cast<std::uint8_t>(sig.precision_ratio * 255.f);
    return cr >= ln.cruelty_min && cr <= ln.cruelty_max
        && pr >= ln.precision_min && pr <= ln.precision_max;
}

} // namespace

VoicePick pick_voice_line(const VoiceDirectorContext& ctx) noexcept {
    VoicePick out{};
    if (ctx.graph == nullptr) {
        return out;
    }

    // Hard overrides.
    if (ctx.grace_window) {
        for (const DialogueLine& ln : ctx.graph->lines()) {
            if (ln.speaker == Speaker::None /* combined 'forgive' beat */) {
                out.line = ln.id;
                return out;
            }
        }
    }

    // P22 W146 — mirror-step is Malakor-only, Act-II-gated, and ignores
    // Sin-signature range so the line always fires on cast.
    const Speaker forced = ctx.mirror_step   ? Speaker::Malakor
                         : ctx.rapture_active ? Speaker::Malakor
                         : ctx.ruin_active    ? Speaker::Niccolo
                                              : Speaker::None;

    // Weighted roll: walk all candidates, hash each candidate with seed, pick
    // the lowest hash matching all gates. Pure function of (seed, gates).
    std::uint64_t best_hash = ~std::uint64_t{0};
    for (const DialogueLine& ln : ctx.graph->lines()) {
        if (ln.act_gate != Act::None && ln.act_gate != ctx.act) continue;
        if (!circle_gate_allows(ln.circle_gate_mask, ctx.circle_index)) continue;
        if (forced != Speaker::None && ln.speaker != forced) continue;
        // Mirror-step bypasses Sin-signature bounds so the line always fires.
        if (!ctx.mirror_step && !sig_in_bounds(ctx.signature, ln)) continue;

        // Sin-signature bias: lines matching dominant channel get a hash bonus.
        std::uint64_t h = splitmix64(ctx.seed ^ static_cast<std::uint64_t>(ln.id.value));
        if (ctx.signature.cruelty_ratio > 0.6f && ln.speaker == Speaker::Malakor) h >>= 1;
        if (ctx.signature.precision_ratio > 0.6f && ln.speaker == Speaker::Niccolo) h >>= 1;

        if (h < best_hash) {
            best_hash    = h;
            out.line     = ln.id;
            out.speaker  = ln.speaker;
        }
    }
    return out;
}

} // namespace gw::narrative
