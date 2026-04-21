#pragma once
// editor/commands/editor_commands.hpp
// Phase 7 editor-specific ICommand implementations.
// Spec ref: Phase 7 §5 — SetEntityTransform, SetComponentField, CreateEntity,
//            DestroyEntity, ReparentEntity.

#include "engine/core/command.hpp"
#include "editor/selection/selection_context.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gw::editor::commands {

// ---------------------------------------------------------------------------
// Transform snapshot used by SetEntityTransformCommand.
// Phase 8 will replace this with the ECS TransformComponent directly.
// ---------------------------------------------------------------------------
struct TransformSnapshot {
    glm::vec3 translation{0.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};
    glm::vec3 scale{1.f};
};

// Callback type for applying/querying transforms (bridged to ECS by the
// ViewportPanel; Phase 8 queries the real ECS world).
using SetTransformFn  = std::function<void(EntityHandle, const TransformSnapshot&)>;
using GetTransformFn  = std::function<TransformSnapshot(EntityHandle)>;

// ---------------------------------------------------------------------------
// SetEntityTransformCommand
// ---------------------------------------------------------------------------
class SetEntityTransformCommand final : public gw::core::ICommand {
public:
    SetEntityTransformCommand(EntityHandle          h,
                               TransformSnapshot      before,
                               TransformSnapshot      after,
                               SetTransformFn         set_fn);

    void execute() override;
    void undo()    override;
    [[nodiscard]] bool can_undo()     const override { return true; }
    [[nodiscard]] const char* name()  const override { return "Set Transform"; }
    [[nodiscard]] bool try_merge(gw::core::ICommand* newer) noexcept override;

private:
    EntityHandle      handle_;
    TransformSnapshot before_;
    TransformSnapshot after_;
    SetTransformFn    set_fn_;
};

// ---------------------------------------------------------------------------
// SetComponentFieldCommand<T> — typed field mutation.
// Spec ref: Phase 7 §5.
// ---------------------------------------------------------------------------
template<typename T>
class SetComponentFieldCommand final : public gw::core::ICommand {
public:
    using GetFn = std::function<T()>;
    using SetFn = std::function<void(const T&)>;

    SetComponentFieldCommand(const char* label, T before, T after,
                              GetFn get_fn, SetFn set_fn)
        : label_(label)
        , before_(std::move(before))
        , after_(std::move(after))
        , get_fn_(std::move(get_fn))
        , set_fn_(std::move(set_fn)) {}

    void execute() override               { set_fn_(after_); }
    void undo()    override               { set_fn_(before_); }
    [[nodiscard]] bool can_undo()  const override { return true; }
    [[nodiscard]] const char* name() const override { return label_.c_str(); }

    [[nodiscard]] bool try_merge(gw::core::ICommand* newer) noexcept override {
        auto* n = dynamic_cast<SetComponentFieldCommand<T>*>(newer);
        if (!n || n->label_ != label_) return false;
        after_ = n->after_;
        set_fn_(after_);
        return true;
    }

private:
    std::string label_;
    T           before_;
    T           after_;
    GetFn       get_fn_;
    SetFn       set_fn_;
};

// ---------------------------------------------------------------------------
// CreateEntityCommand
// ---------------------------------------------------------------------------
class CreateEntityCommand final : public gw::core::ICommand {
public:
    using CreateFn  = std::function<EntityHandle(const char* name)>;
    using DestroyFn = std::function<void(EntityHandle)>;

    CreateEntityCommand(std::string name, CreateFn create, DestroyFn destroy);

    void execute() override;
    void undo()    override;
    [[nodiscard]] bool can_undo()    const override { return true; }
    [[nodiscard]] const char* name() const override { return "Create Entity"; }

    [[nodiscard]] EntityHandle created_handle() const noexcept { return handle_; }

private:
    std::string  entity_name_;
    EntityHandle handle_ = kNullEntity;
    CreateFn     create_fn_;
    DestroyFn    destroy_fn_;
};

// ---------------------------------------------------------------------------
// DestroyEntityCommand
// ---------------------------------------------------------------------------
class DestroyEntityCommand final : public gw::core::ICommand {
public:
    using DestroyFn  = std::function<void(EntityHandle)>;
    using RecreateFromSnapshotFn = std::function<EntityHandle(const std::vector<uint8_t>&)>;

    DestroyEntityCommand(EntityHandle h, std::vector<uint8_t> snapshot,
                          DestroyFn destroy, RecreateFromSnapshotFn recreate);

    void execute() override;
    void undo()    override;
    [[nodiscard]] bool can_undo()    const override { return true; }
    [[nodiscard]] const char* name() const override { return "Destroy Entity"; }

private:
    EntityHandle             handle_;
    std::vector<uint8_t>     snapshot_;
    DestroyFn                destroy_fn_;
    RecreateFromSnapshotFn   recreate_fn_;
    EntityHandle             restored_handle_ = kNullEntity;
};

// ---------------------------------------------------------------------------
// ReparentEntityCommand
// ---------------------------------------------------------------------------
class ReparentEntityCommand final : public gw::core::ICommand {
public:
    using ReparentFn = std::function<void(EntityHandle child, EntityHandle new_parent)>;

    ReparentEntityCommand(EntityHandle child,
                           EntityHandle old_parent,
                           EntityHandle new_parent,
                           ReparentFn   fn);

    void execute() override;
    void undo()    override;
    [[nodiscard]] bool can_undo()    const override { return true; }
    [[nodiscard]] const char* name() const override { return "Reparent Entity"; }

private:
    EntityHandle child_;
    EntityHandle old_parent_;
    EntityHandle new_parent_;
    ReparentFn   fn_;
};

}  // namespace gw::editor::commands
