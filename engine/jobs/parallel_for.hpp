#pragma once
// Free-function thin wrappers around Scheduler::parallel_for.
// Prefer using Scheduler::parallel_for directly; these exist for backwards
// compatibility with callers that do not own a Scheduler instance.

#include "scheduler.hpp"

namespace gw {
namespace jobs {

// parallel_for(scheduler, count, func)
// func: void(uint32_t index)
template<typename Func>
void parallel_for(Scheduler& scheduler, uint32_t count, Func&& func) {
    scheduler.parallel_for(count, std::forward<Func>(func));
}

} // namespace jobs
} // namespace gw
