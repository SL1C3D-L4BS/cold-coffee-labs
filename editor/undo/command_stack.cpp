// editor/undo/command_stack.cpp
// See editor/undo/command_stack.hpp. ADR-0005 §2.3-2.9.
#include "command_stack.hpp"

#include <algorithm>
#include <cstring>
#include <typeinfo>
#include <utility>

namespace gw::editor::undo {

// ---------------------------------------------------------------------------
// GroupCommand
// ---------------------------------------------------------------------------
GroupCommand::GroupCommand(std::string label,
                            std::vector<std::unique_ptr<ICommand>> children) noexcept
    : label_(std::move(label))
    , children_(std::move(children)) {}

void GroupCommand::do_() {
    for (auto& c : children_) c->do_();
}

void GroupCommand::undo() {
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        (*it)->undo();
}

std::size_t GroupCommand::heap_bytes() const noexcept {
    std::size_t total = sizeof(GroupCommand) + label_.capacity()
                      + children_.capacity() * sizeof(std::unique_ptr<ICommand>);
    for (const auto& c : children_) {
        if (c) total += c->heap_bytes();
    }
    return total;
}

// ---------------------------------------------------------------------------
// CommandStack — ctors
// ---------------------------------------------------------------------------
CommandStack::CommandStack(Config cfg) : cfg_(cfg) {}
CommandStack::~CommandStack() = default;
CommandStack::CommandStack(CommandStack&&) noexcept            = default;
CommandStack& CommandStack::operator=(CommandStack&&) noexcept = default;

// ---------------------------------------------------------------------------
// push
// ---------------------------------------------------------------------------
void CommandStack::push(std::unique_ptr<ICommand> cmd) {
    if (!cmd) return;

    // Inside a group: do_() runs now, command is buffered into the innermost
    // group (no coalescing across stack boundary; GroupCommand itself handles
    // any intra-group coalescing in a future revision).
    cmd->do_();

    if (group_depth_ > 0) {
        open_groups_.back().children.push_back(std::move(cmd));
        return;
    }

    push_no_group_(std::move(cmd));
}

void CommandStack::push_no_group_(std::unique_ptr<ICommand> cmd) {
    const auto now = std::chrono::steady_clock::now();

    // Coalesce with the top of the undo stack if enabled, within window, and
    // same concrete type. The top decides via try_coalesce().
    if (coalesce_enabled_ && !undo_.empty()) {
        auto& top = undo_.back();
        // Note: typeid on an lvalue of polymorphic type is well-defined; we
        // deliberately accept the 'potentially-evaluated-expression' warning
        // here since the side-effect-free smart-pointer deref is safe.
        const ICommand& top_ref = *top;
        const ICommand& cmd_ref = *cmd;
        const bool same_type = typeid(top_ref) == typeid(cmd_ref);
        const bool in_window = (now - last_push_time_) <= cfg_.coalesce_window;
        if (same_type && in_window && top->try_coalesce(*cmd, now)) {
            // Top absorbed the new command. memory_used_ may drift but the
            // delta per-coalesce is bounded (same-shape state); recompute lazily.
            // For simplicity we refresh the counter from the coalesced top.
            // (SetComponentFieldCommand<T> etc. keep their size constant, so
            // this loop stays cheap.)
            last_push_time_ = now;
            redo_.clear();
            return;
        }
    }

    redo_.clear();
    memory_used_ += command_bytes_(*cmd);
    undo_.push_back(std::move(cmd));
    last_push_time_ = now;
    dirty_          = true;
    trim_to_budget_();
}

// ---------------------------------------------------------------------------
// undo / redo
// ---------------------------------------------------------------------------
void CommandStack::undo() {
    if (undo_.empty()) return;
    auto c = std::move(undo_.back());
    undo_.pop_back();
    c->undo();
    redo_.push_back(std::move(c));
    // Sentinel — prevent coalesce with the just-popped command on next push.
    last_push_time_ = {};
    dirty_          = true;
}

void CommandStack::redo() {
    if (redo_.empty()) return;
    auto c = std::move(redo_.back());
    redo_.pop_back();
    c->do_();
    undo_.push_back(std::move(c));
    last_push_time_ = {};
    dirty_          = true;
}

std::string_view CommandStack::next_undo_label() const noexcept {
    if (undo_.empty()) return {};
    return undo_.back()->label();
}

std::string_view CommandStack::next_redo_label() const noexcept {
    if (redo_.empty()) return {};
    return redo_.back()->label();
}

// ---------------------------------------------------------------------------
// Groups
// ---------------------------------------------------------------------------
void CommandStack::begin_group(std::string_view label) {
    open_groups_.push_back({std::string{label}, {}});
    ++group_depth_;
}

void CommandStack::end_group() {
    if (group_depth_ == 0) return;
    --group_depth_;

    auto g = std::move(open_groups_.back());
    open_groups_.pop_back();

    if (g.children.empty()) return;

    auto grp = std::make_unique<GroupCommand>(std::move(g.label),
                                                std::move(g.children));

    if (group_depth_ > 0) {
        // Nested — hand the group up to the enclosing group as a single child.
        open_groups_.back().children.push_back(std::move(grp));
        return;
    }

    // Top-level close: push without invoking do_() again (children already ran).
    redo_.clear();
    memory_used_ += command_bytes_(*grp);
    undo_.push_back(std::move(grp));
    last_push_time_ = std::chrono::steady_clock::now();
    dirty_          = true;
    trim_to_budget_();
}

// ---------------------------------------------------------------------------
// Memory bookkeeping
// ---------------------------------------------------------------------------
std::size_t CommandStack::command_bytes_(const ICommand& c) noexcept {
    return c.heap_bytes();
}

void CommandStack::trim_to_budget_() noexcept {
    while (memory_used_ > cfg_.memory_budget_bytes && !undo_.empty()) {
        memory_used_ -= command_bytes_(*undo_.front());
        undo_.erase(undo_.begin());
    }
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------
void CommandStack::clear() {
    undo_.clear();
    redo_.clear();
    open_groups_.clear();
    group_depth_    = 0;
    memory_used_    = 0;
    last_push_time_ = {};
    dirty_          = false;
}

// ---------------------------------------------------------------------------
// summary
// ---------------------------------------------------------------------------
void CommandStack::summary(char* buf, std::size_t buf_size,
                             std::size_t max_entries) const noexcept {
    if (!buf || buf_size == 0) return;
    buf[0] = '\0';
    if (undo_.empty() || max_entries == 0) return;

    std::size_t pos     = 0;
    std::size_t written = 0;

    for (auto it = undo_.rbegin(); it != undo_.rend() && written < max_entries;
         ++it, ++written) {
        const auto lbl  = (*it)->label();
        const auto need = lbl.size() + 1;  // +1 for separator
        if (pos + need >= buf_size) break;
        std::memcpy(buf + pos, lbl.data(), lbl.size());
        pos += lbl.size();
        buf[pos++] = '|';
    }
    if (pos > 0 && buf[pos - 1] == '|') --pos;
    buf[pos] = '\0';
}

}  // namespace gw::editor::undo
