// engine/ai_runtime/ai_cvars.cpp — Phase 26 scaffold.

#include "engine/ai_runtime/ai_cvars.hpp"

namespace gw::ai_runtime {

namespace {
AiCVars& storage() noexcept {
    static AiCVars s{};
    return s;
}
} // namespace

const AiCVars& cvars() noexcept        { return storage(); }
AiCVars&       cvars_mutable() noexcept { return storage(); }

} // namespace gw::ai_runtime
