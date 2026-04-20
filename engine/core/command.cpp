#include "command.hpp"

namespace gw {
namespace core {

CommandStack::CommandStack(uint32_t max_commands) 
    : commands_(max_commands), current_index_(-1) {}

CommandStack::~CommandStack() = default;

void CommandStack::execute_and_push(std::unique_ptr<ICommand> command) {
    if (current_index_ >= 0) {
        // Clear redo stack when executing new command
        for (int32_t i = current_index_; i < static_cast<int32_t>(commands_.size()) - 1; ++i) {
            commands_[static_cast<size_t>(i)]->undo();
        }
    }
    
    command->execute();
    commands_[++current_index_] = std::move(command);
}

bool CommandStack::undo() {
    if (current_index_ > 0) {
        commands_[current_index_ - 1]->undo();
        --current_index_;
        return true;
    }
    return false;
}

bool CommandStack::redo() {
    if (current_index_ < static_cast<int32_t>(commands_.size()) - 1) {
        commands_[++current_index_]->execute();
        return true;
    }
    return false;
}

void CommandStack::clear() {
    commands_.clear();
    current_index_ = -1;
}

bool CommandStack::can_undo() const {
    return current_index_ > 0;
}

bool CommandStack::can_redo() const {
    return current_index_ < static_cast<int32_t>(commands_.size()) - 1;
}

uint32_t CommandStack::size() const {
    return commands_.size();
}

}  // namespace core
}  // namespace gw
