#pragma once
// engine/core/command.hpp
// ICommand + CommandStack with correct undo/redo semantics, dirty tracking,
// transaction brackets, and try_merge for consecutive-drag coalescing.

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace gw::core {

// ---------------------------------------------------------------------------
// ICommand
// ---------------------------------------------------------------------------
class ICommand {
public:
    virtual ~ICommand() = default;

    virtual void execute()                         = 0;
    virtual void undo()                            = 0;
    [[nodiscard]] virtual bool can_undo()    const = 0;
    [[nodiscard]] virtual const char* name() const = 0;

    // Optional merge: let `this` absorb `newer` (same target, same drag).
    // Return true → newer is discarded, this is updated in-place.
    [[nodiscard]] virtual bool try_merge(ICommand* /*newer*/) noexcept { return false; }
};

// ---------------------------------------------------------------------------
// TransactionCommand — groups N commands into one atomic undo step.
// Created by CommandStack::commit_transaction(); never used directly.
// ---------------------------------------------------------------------------
class TransactionCommand final : public ICommand {
public:
    explicit TransactionCommand(std::string label,
                                std::vector<std::unique_ptr<ICommand>> cmds) noexcept
        : label_(std::move(label))
        , cmds_(std::move(cmds)) {}

    void execute() override {
        for (auto& c : cmds_) c->execute();
    }
    void undo() override {
        for (auto it = cmds_.rbegin(); it != cmds_.rend(); ++it)
            (*it)->undo();
    }
    [[nodiscard]] bool can_undo() const override { return !cmds_.empty(); }
    [[nodiscard]] const char* name() const override { return label_.c_str(); }

private:
    std::string                            label_;
    std::vector<std::unique_ptr<ICommand>> cmds_;
};

// ---------------------------------------------------------------------------
// CommandStack
// ---------------------------------------------------------------------------
class CommandStack {
public:
    // max_undo: maximum number of reversible steps retained.
    explicit CommandStack(uint32_t max_undo = 256) noexcept;

    CommandStack(const CommandStack&)            = delete;
    CommandStack& operator=(const CommandStack&) = delete;
    CommandStack(CommandStack&&)                 = default;
    CommandStack& operator=(CommandStack&&)      = default;

    // Execute cmd, optionally merge with previous command of same type/target,
    // then push onto the undo stack.  Clears the redo branch.
    void execute_and_push(std::unique_ptr<ICommand> cmd);

    // Returns false when there is nothing to undo/redo.
    [[nodiscard]] bool undo();
    [[nodiscard]] bool redo();

    [[nodiscard]] bool can_undo() const noexcept;
    [[nodiscard]] bool can_redo() const noexcept;

    // Transaction bracketing — everything pushed between begin/commit becomes
    // one reversible step.  Nested calls are not supported (assertion fires).
    void begin_transaction(const char* label);
    void commit_transaction();
    void rollback_transaction();   // abort, undo partial work, discard
    [[nodiscard]] bool in_transaction() const noexcept { return tx_open_; }

    // Dirty flag: true after any mutation that hasn't been mark_saved().
    [[nodiscard]] bool is_dirty() const noexcept { return dirty_; }
    void mark_saved() noexcept { dirty_ = false; }

    void clear() noexcept;

    [[nodiscard]] uint32_t undo_depth() const noexcept;
    [[nodiscard]] uint32_t redo_depth() const noexcept;
    [[nodiscard]] uint32_t max_undo()   const noexcept { return max_undo_; }

    // Fill buf with "|"-separated command names for BLD API; buf is null-terminated.
    void summary(char* buf, uint32_t buf_size, uint32_t max_entries) const noexcept;

private:
    // Conceptual layout: [0 .. head_) = undo branch; [head_ .. tail_) = redo branch.
    // tail_ is always the logical end of valid entries; head_ is the write cursor.
    // On execute_and_push: truncate to head_, then append.
    // On undo: head_ -= 1.
    // On redo: head_ += 1.
    std::vector<std::unique_ptr<ICommand>> stack_;
    uint32_t head_     = 0;
    uint32_t tail_     = 0;
    uint32_t max_undo_ = 256;
    bool     dirty_    = false;
    bool     tx_open_  = false;

    std::vector<std::unique_ptr<ICommand>> tx_pending_;
    std::string                            tx_label_;
};

}  // namespace gw::core
