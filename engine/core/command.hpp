#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace gw {
namespace core {

// Command interface for undo/redo system
class ICommand {
public:
    virtual ~ICommand() = default;
    
    // Execute the command
    virtual void execute() = 0;
    
    // Undo the command
    virtual void undo() = 0;
    
    // Check if command can be undone
    [[nodiscard]] virtual bool can_undo() const = 0;
    
    // Get command description
    [[nodiscard]] virtual const char* name() const = 0;
};

// Command stack for managing undo/redo history
class CommandStack {
public:
    CommandStack(uint32_t max_commands = 100);
    ~CommandStack();
    
    // Execute and push command
    void execute_and_push(std::unique_ptr<ICommand> command);
    
    // Undo last command
    bool undo();
    
    // Redo undone command
    bool redo();
    
    // Clear all commands
    void clear();
    
    // Check if can undo/redo
    [[nodiscard]] bool can_undo() const;
    [[nodiscard]] bool can_redo() const;
    
    [[nodiscard]] uint32_t size() const;
    
private:
    std::vector<std::unique_ptr<ICommand>> commands_;
    int32_t current_index_{-1};
};

}  // namespace core
}  // namespace gw
