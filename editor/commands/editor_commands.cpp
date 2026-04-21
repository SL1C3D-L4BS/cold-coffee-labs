// editor/commands/editor_commands.cpp
#include "editor_commands.hpp"

namespace gw::editor::commands {

// ---------------------------------------------------------------------------
// SetEntityTransformCommand
// ---------------------------------------------------------------------------
SetEntityTransformCommand::SetEntityTransformCommand(
    EntityHandle h, TransformSnapshot before, TransformSnapshot after,
    SetTransformFn set_fn)
    : handle_(h)
    , before_(std::move(before))
    , after_(std::move(after))
    , set_fn_(std::move(set_fn)) {}

void SetEntityTransformCommand::execute() { set_fn_(handle_, after_); }
void SetEntityTransformCommand::undo()    { set_fn_(handle_, before_); }

bool SetEntityTransformCommand::try_merge(gw::core::ICommand* newer) noexcept {
    auto* n = dynamic_cast<SetEntityTransformCommand*>(newer);
    if (!n || n->handle_ != handle_) return false;
    after_ = n->after_;
    set_fn_(handle_, after_);
    return true;
}

// ---------------------------------------------------------------------------
// CreateEntityCommand
// ---------------------------------------------------------------------------
CreateEntityCommand::CreateEntityCommand(std::string name,
                                          CreateFn    create,
                                          DestroyFn   destroy)
    : entity_name_(std::move(name))
    , create_fn_(std::move(create))
    , destroy_fn_(std::move(destroy)) {}

void CreateEntityCommand::execute() {
    handle_ = create_fn_(entity_name_.c_str());
}
void CreateEntityCommand::undo() {
    if (handle_ != kNullEntity) {
        destroy_fn_(handle_);
        handle_ = kNullEntity;
    }
}

// ---------------------------------------------------------------------------
// DestroyEntityCommand
// ---------------------------------------------------------------------------
DestroyEntityCommand::DestroyEntityCommand(
    EntityHandle h, std::vector<uint8_t> snapshot,
    DestroyFn destroy, RecreateFromSnapshotFn recreate)
    : handle_(h)
    , snapshot_(std::move(snapshot))
    , destroy_fn_(std::move(destroy))
    , recreate_fn_(std::move(recreate)) {}

void DestroyEntityCommand::execute() {
    destroy_fn_(handle_);
}
void DestroyEntityCommand::undo() {
    restored_handle_ = recreate_fn_(snapshot_);
}

// ---------------------------------------------------------------------------
// ReparentEntityCommand
// ---------------------------------------------------------------------------
ReparentEntityCommand::ReparentEntityCommand(
    EntityHandle child, EntityHandle old_parent, EntityHandle new_parent,
    ReparentFn fn)
    : child_(child)
    , old_parent_(old_parent)
    , new_parent_(new_parent)
    , fn_(std::move(fn)) {}

void ReparentEntityCommand::execute() { fn_(child_, new_parent_); }
void ReparentEntityCommand::undo()    { fn_(child_, old_parent_); }

}  // namespace gw::editor::commands
