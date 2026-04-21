#pragma once
// editor/undo/command_stack.hpp
// Editor undo / redo stack per ADR-0005.
//
// Responsibilities:
//   * ICommand — symmetric do_() / undo() with label(), heap_bytes(), and a
//     time-gated try_coalesce() hook for drag-merging.
//   * CommandStack — execute → push → trim-to-budget, with coalesce window,
//     group brackets (begin_group/end_group), and a FIFO memory cap.
//
// Non-goals (ADR-0005 §2.10):
//   * Persistent undo across sessions.
//   * Replay / determinism.
//   * Multi-user OT / CRDT.
//   * Asynchronous commands.
//
// All state is single-threaded; every call must happen on the editor thread.

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gw::editor::undo {

// ---------------------------------------------------------------------------
// ICommand
// ---------------------------------------------------------------------------
class ICommand {
public:
    virtual ~ICommand() = default;

    virtual void do_()   = 0;
    virtual void undo()  = 0;

    [[nodiscard]] virtual std::string_view label() const = 0;

    // Approximate heap footprint (conservative upper bound).
    [[nodiscard]] virtual std::size_t heap_bytes() const noexcept = 0;

    // Default: no coalescing.
    [[nodiscard]] virtual bool try_coalesce(
        const ICommand& /*other*/,
        std::chrono::steady_clock::time_point /*now*/) {
        return false;
    }
};

// ---------------------------------------------------------------------------
// GroupCommand — aggregates multiple ICommands into one atomic undo step.
// Created internally by CommandStack::end_group; also exposed for tests.
// ---------------------------------------------------------------------------
class GroupCommand final : public ICommand {
public:
    GroupCommand(std::string label,
                  std::vector<std::unique_ptr<ICommand>> children) noexcept;

    void do_()  override;
    void undo() override;

    [[nodiscard]] std::string_view label()      const override { return label_; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override;

    [[nodiscard]] std::size_t size() const noexcept { return children_.size(); }

private:
    std::string                            label_;
    std::vector<std::unique_ptr<ICommand>> children_;
};

// ---------------------------------------------------------------------------
// CommandStackConfig — lives at namespace scope (clang-cl refuses in-class
// default member initializers on a nested struct when the enclosing class has
// a field of that nested type default-constructed).
// ---------------------------------------------------------------------------
struct CommandStackConfig {
    std::size_t               memory_budget_bytes = 16 * 1024 * 1024;
    std::chrono::milliseconds coalesce_window{120};
};

// ---------------------------------------------------------------------------
// CommandStack
// ---------------------------------------------------------------------------
class CommandStack {
public:
    using Config = CommandStackConfig;

    explicit CommandStack(Config cfg = {});
    ~CommandStack();

    CommandStack(const CommandStack&)            = delete;
    CommandStack& operator=(const CommandStack&) = delete;
    CommandStack(CommandStack&&) noexcept;
    CommandStack& operator=(CommandStack&&) noexcept;

    // Execute do_() synchronously; attempt coalesce with top; push, trim, clear redo.
    void push(std::unique_ptr<ICommand> cmd);

    void undo();
    void redo();

    [[nodiscard]] bool can_undo() const noexcept { return !undo_.empty(); }
    [[nodiscard]] bool can_redo() const noexcept { return !redo_.empty(); }

    [[nodiscard]] std::string_view next_undo_label() const noexcept;
    [[nodiscard]] std::string_view next_redo_label() const noexcept;

    void begin_group(std::string_view label);
    void end_group();
    [[nodiscard]] bool in_group() const noexcept { return group_depth_ > 0; }

    void set_coalescing_enabled(bool enabled) noexcept { coalesce_enabled_ = enabled; }
    [[nodiscard]] bool coalescing_enabled() const noexcept { return coalesce_enabled_; }

    [[nodiscard]] std::size_t undo_depth()  const noexcept { return undo_.size(); }
    [[nodiscard]] std::size_t redo_depth()  const noexcept { return redo_.size(); }
    [[nodiscard]] std::size_t memory_used() const noexcept { return memory_used_; }

    void clear();

    // Dirty tracking — "any mutation since last mark_saved()".
    // Not in ADR-0005 but cheap and useful for the editor's title-bar "*".
    [[nodiscard]] bool is_dirty() const noexcept { return dirty_; }
    void mark_saved() noexcept { dirty_ = false; }

    // Debug summary: "|"-separated undo-branch labels, most recent first.
    // Null-terminates within buf_size. For BLD introspection.
    void summary(char* buf, std::size_t buf_size,
                  std::size_t max_entries) const noexcept;

private:
    void       push_no_group_(std::unique_ptr<ICommand> cmd);
    void       trim_to_budget_() noexcept;
    [[nodiscard]] static std::size_t command_bytes_(const ICommand& c) noexcept;

    Config cfg_{};

    std::vector<std::unique_ptr<ICommand>> undo_;
    std::vector<std::unique_ptr<ICommand>> redo_;

    // Group aggregator — std::vector of open groups to support nesting.
    struct OpenGroup {
        std::string                            label;
        std::vector<std::unique_ptr<ICommand>> children;
    };
    std::vector<OpenGroup> open_groups_;
    int                    group_depth_ = 0;

    std::size_t memory_used_ = 0;

    std::chrono::steady_clock::time_point last_push_time_{};
    bool coalesce_enabled_ = true;
    bool dirty_            = false;
};

// ---------------------------------------------------------------------------
// Ergonomic factory (NN #5 — no raw new/delete in consumer code).
// ---------------------------------------------------------------------------
template <typename Cmd, typename... Args>
[[nodiscard]] std::unique_ptr<ICommand> make_command(Args&&... args) {
    return std::make_unique<Cmd>(std::forward<Args>(args)...);
}

}  // namespace gw::editor::undo
