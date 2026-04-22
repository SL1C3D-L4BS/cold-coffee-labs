#pragma once

#include "engine/persist/cloud_save.hpp"

namespace gw::persist {

enum class CloudResolveAction : std::uint8_t {
    Prompt,
    TakeLocal,
    TakeCloud,
};

[[nodiscard]] CloudResolveAction resolve_conflict(const SlotStamp& local_stamp,
                                                  const SlotStamp& cloud_stamp,
                                                  CloudConflictPolicy policy) noexcept;

} // namespace gw::persist
