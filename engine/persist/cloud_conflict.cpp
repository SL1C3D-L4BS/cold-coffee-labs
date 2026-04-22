// engine/persist/cloud_conflict.cpp

#include "engine/persist/cloud_conflict.hpp"

namespace gw::persist {

CloudResolveAction resolve_conflict(const SlotStamp& local_stamp,
                                    const SlotStamp& cloud_stamp,
                                    CloudConflictPolicy policy) noexcept {
    if (policy == CloudConflictPolicy::PreferLocal) return CloudResolveAction::TakeLocal;
    if (policy == CloudConflictPolicy::PreferCloud) return CloudResolveAction::TakeCloud;

    if (policy == CloudConflictPolicy::PreferNewer) {
        const bool ms_ok = local_stamp.unix_ms <= cloud_stamp.unix_ms;
        const bool clk_ok = local_stamp.vector_clock <= cloud_stamp.vector_clock;
        if (ms_ok && clk_ok) return CloudResolveAction::TakeCloud;
        if (!ms_ok && !clk_ok) return CloudResolveAction::TakeLocal;
        return CloudResolveAction::Prompt;
    }

    if (policy == CloudConflictPolicy::PreserveBoth) return CloudResolveAction::Prompt;

    return CloudResolveAction::Prompt;
}

} // namespace gw::persist
