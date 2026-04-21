// engine/core/command.cpp
#include "command.hpp"
#include "assert.hpp"

#include <algorithm>
#include <cstring>

namespace gw::core {

CommandStack::CommandStack(uint32_t max_undo) noexcept
    : max_undo_(max_undo) {
    stack_.reserve(max_undo);
}

void CommandStack::execute_and_push(std::unique_ptr<ICommand> cmd) {
    if (tx_open_) {
        // Inside a transaction — buffer until commit.
        cmd->execute();
        tx_pending_.push_back(std::move(cmd));
        return;
    }

    // Try to merge with the last executed command (same index = head_-1).
    if (head_ > 0 && !stack_.empty()) {
        auto& prev = stack_[head_ - 1];
        if (prev && prev->try_merge(cmd.get())) {
            // Absorbed — no new undo step.
            dirty_ = true;
            return;
        }
    }

    // Truncate redo branch: [head_, tail_) is no longer reachable.
    if (head_ < tail_) {
        stack_.erase(stack_.begin() + static_cast<ptrdiff_t>(head_),
                     stack_.begin() + static_cast<ptrdiff_t>(tail_));
        tail_ = head_;
    }

    cmd->execute();

    // Enforce max_undo by evicting the oldest command.
    if (head_ >= max_undo_) {
        stack_.erase(stack_.begin());
        // head_ and tail_ shift down by one.
        if (head_ > 0) --head_;
        if (tail_ > 0) --tail_;
    }

    stack_.push_back(std::move(cmd));
    head_ = tail_ = static_cast<uint32_t>(stack_.size());
    dirty_ = true;
}

bool CommandStack::undo() {
    if (head_ == 0) return false;
    --head_;
    stack_[head_]->undo();
    dirty_ = true;
    return true;
}

bool CommandStack::redo() {
    if (head_ == tail_) return false;
    stack_[head_]->execute();
    ++head_;
    dirty_ = true;
    return true;
}

bool CommandStack::can_undo() const noexcept {
    return head_ > 0;
}

bool CommandStack::can_redo() const noexcept {
    return head_ < tail_;
}

void CommandStack::begin_transaction(const char* label) {
    GW_ASSERT(!tx_open_, "CommandStack: nested transactions not supported");
    tx_open_  = true;
    tx_label_ = label ? label : "";
    tx_pending_.clear();
}

void CommandStack::commit_transaction() {
    GW_ASSERT(tx_open_, "CommandStack: commit_transaction without begin_transaction");
    tx_open_ = false;
    if (tx_pending_.empty()) return;

    // Wrap all pending commands into a single TransactionCommand and push it.
    auto tx = std::make_unique<TransactionCommand>(
        std::move(tx_label_), std::move(tx_pending_));
    tx_pending_.clear();

    // execute_and_push with tx_open_ == false pushes normally.
    // The TransactionCommand already called execute() on each sub-command,
    // so we override the inner execute() to be idempotent on first call
    // by pushing directly (no second execute).
    // Truncate redo branch.
    if (head_ < tail_) {
        stack_.erase(stack_.begin() + static_cast<ptrdiff_t>(head_),
                     stack_.begin() + static_cast<ptrdiff_t>(tail_));
        tail_ = head_;
    }
    if (head_ >= max_undo_) {
        stack_.erase(stack_.begin());
        if (head_ > 0) --head_;
        if (tail_ > 0) --tail_;
    }
    stack_.push_back(std::move(tx));
    head_ = tail_ = static_cast<uint32_t>(stack_.size());
    dirty_ = true;
}

void CommandStack::rollback_transaction() {
    GW_ASSERT(tx_open_, "CommandStack: rollback_transaction without begin_transaction");
    tx_open_ = false;
    // Undo in reverse order.
    for (auto it = tx_pending_.rbegin(); it != tx_pending_.rend(); ++it)
        (*it)->undo();
    tx_pending_.clear();
    tx_label_.clear();
}

void CommandStack::clear() noexcept {
    stack_.clear();
    head_  = 0;
    tail_  = 0;
    dirty_ = false;
    if (tx_open_) {
        tx_open_ = false;
        tx_pending_.clear();
        tx_label_.clear();
    }
}

uint32_t CommandStack::undo_depth() const noexcept {
    return head_;
}

uint32_t CommandStack::redo_depth() const noexcept {
    return tail_ - head_;
}

void CommandStack::summary(char* buf, uint32_t buf_size,
                           uint32_t max_entries) const noexcept {
    if (!buf || buf_size == 0) return;
    buf[0] = '\0';

    uint32_t written = 0;
    uint32_t pos     = 0;

    // Print undo branch in reverse (most recent first).
    for (int32_t i = static_cast<int32_t>(head_) - 1;
         i >= 0 && written < max_entries; --i, ++written) {
        const char* n  = stack_[static_cast<uint32_t>(i)]->name();
        uint32_t    nl = static_cast<uint32_t>(std::strlen(n));
        if (pos + nl + 1 >= buf_size) break;
        std::memcpy(buf + pos, n, nl);
        pos += nl;
        buf[pos++] = '|';
    }
    if (pos > 0 && buf[pos - 1] == '|') --pos;
    buf[pos] = '\0';
}

}  // namespace gw::core
