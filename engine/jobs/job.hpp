#pragma once

#include "scheduler.hpp"
#include <cstdint>
#include <cstddef>
#include <functional>
#include <tuple>

namespace gw {
namespace jobs {

// Individual job with its function and captured arguments
template<typename... Args>
class Job {
public:
    using function_type = JobFunction<Args...>;
    
    Job(JobPriority priority, function_type&& func, Args&&... args)
        : priority_(priority), func_(std::move(func)), args_(std::make_tuple(std::forward<Args>(args)...)) {}
    
    [[nodiscard]] JobPriority priority() const noexcept { return priority_; }
    
    // Execute the job
    void execute() {
        std::apply(func_, args_);
    }
    
private:
    // Helper function to execute with unpacked arguments
    template<std::size_t... Indices>
    void execute_with_args_impl(std::tuple<Args...>& args, std::index_sequence<Indices...>);
    
    JobPriority priority_;
    function_type func_;
    std::tuple<Args...> args_;
};

}  // namespace jobs
}  // namespace gw
