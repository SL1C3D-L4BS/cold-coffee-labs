#pragma once
// editor/undo/commands.hpp
// Concrete ICommand types per ADR-0005 §2.7. Each command owns the minimum
// state it needs to round-trip do/undo; world interaction goes through
// std::function callbacks so the command layer doesn't pin down a concrete
// World pointer and stays testable in isolation.

#include "command_stack.hpp"
#include "editor/selection/selection_context.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace gw::editor::undo {

// ---------------------------------------------------------------------------
// SetComponentFieldCommand<T> — one scalar / trivially-copyable field on one
// component on one entity. Primary coalesce case.
// ---------------------------------------------------------------------------
template <typename T>
class SetComponentFieldCommand final : public ICommand {
public:
    using GetFn = std::function<T()>;
    using SetFn = std::function<void(const T&)>;

    SetComponentFieldCommand(std::string label,
                              const void* field_addr,
                              T before, T after,
                              SetFn set)
        : label_(std::move(label))
        , field_addr_(field_addr)
        , before_(std::move(before))
        , after_(std::move(after))
        , set_(std::move(set)) {}

    void do_()  override { if (set_) set_(after_); }
    void undo() override { if (set_) set_(before_); }

    [[nodiscard]] std::string_view label() const override { return label_; }
    [[nodiscard]] std::size_t heap_bytes() const noexcept override {
        return sizeof(*this) + label_.capacity();
    }

    [[nodiscard]] bool try_coalesce(
        const ICommand& other,
        std::chrono::steady_clock::time_point /*now*/) override {
        const auto* o = dynamic_cast<const SetComponentFieldCommand<T>*>(&other);
        if (!o) return false;
        if (o->field_addr_ != field_addr_) return false;
        after_ = o->after_;
        if (set_) set_(after_);
        return true;
    }

    [[nodiscard]] const T& before() const noexcept { return before_; }
    [[nodiscard]] const T& after()  const noexcept { return after_; }

private:
    std::string label_;
    const void* field_addr_;
    T           before_;
    T           after_;
    SetFn       set_;
};

// ---------------------------------------------------------------------------
// TransformSnapshot + SetEntityTransformCommand — kept for the gizmo drag path
// (gizmo_system emits these today; Phase 8 rewires directly to the ECS).
// ---------------------------------------------------------------------------
struct TransformSnapshot {
    glm::vec3 translation{0.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};
    glm::vec3 scale{1.f};
};

class SetEntityTransformCommand final : public ICommand {
public:
    using SetFn = std::function<void(EntityHandle, const TransformSnapshot&)>;

    SetEntityTransformCommand(EntityHandle h,
                               TransformSnapshot before,
                               TransformSnapshot after,
                               SetFn set)
        : handle_(h)
        , before_(before)
        , after_(after)
        , set_(std::move(set)) {}

    void do_()  override { if (set_) set_(handle_, after_); }
    void undo() override { if (set_) set_(handle_, before_); }

    [[nodiscard]] std::string_view label() const override { return "Set Transform"; }
    [[nodiscard]] std::size_t heap_bytes() const noexcept override {
        return sizeof(*this);
    }

    [[nodiscard]] bool try_coalesce(
        const ICommand& other,
        std::chrono::steady_clock::time_point /*now*/) override {
        const auto* o = dynamic_cast<const SetEntityTransformCommand*>(&other);
        if (!o) return false;
        if (o->handle_ != handle_) return false;
        after_ = o->after_;
        if (set_) set_(handle_, after_);
        return true;
    }

private:
    EntityHandle      handle_;
    TransformSnapshot before_;
    TransformSnapshot after_;
    SetFn             set_;
};

// ---------------------------------------------------------------------------
// CreateEntityCommand / DestroyEntityCommand — paired lifecycle edits.
// Snapshot format is opaque bytes so the command layer doesn't depend on the
// serialization path (ADR-0006) directly; callers inject a serialize/restore
// pair.
// ---------------------------------------------------------------------------
class CreateEntityCommand final : public ICommand {
public:
    using CreateFn  = std::function<EntityHandle(const char* name)>;
    using DestroyFn = std::function<void(EntityHandle)>;

    CreateEntityCommand(std::string name, CreateFn create, DestroyFn destroy)
        : name_(std::move(name))
        , create_(std::move(create))
        , destroy_(std::move(destroy)) {}

    void do_()  override { if (create_) handle_ = create_(name_.c_str()); }
    void undo() override { if (destroy_ && handle_ != kNullEntity) destroy_(handle_); }

    [[nodiscard]] std::string_view label()      const override { return "Create Entity"; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override {
        return sizeof(*this) + name_.capacity();
    }

    [[nodiscard]] EntityHandle created_handle() const noexcept { return handle_; }

private:
    std::string  name_;
    EntityHandle handle_ = kNullEntity;
    CreateFn     create_;
    DestroyFn    destroy_;
};

class DestroyEntityCommand final : public ICommand {
public:
    using DestroyFn  = std::function<void(EntityHandle)>;
    using RecreateFn = std::function<EntityHandle(const std::vector<std::uint8_t>&)>;

    DestroyEntityCommand(EntityHandle h,
                          std::vector<std::uint8_t> snapshot,
                          DestroyFn destroy,
                          RecreateFn recreate)
        : handle_(h)
        , snapshot_(std::move(snapshot))
        , destroy_(std::move(destroy))
        , recreate_(std::move(recreate)) {}

    void do_()  override { if (destroy_) destroy_(handle_); }
    void undo() override {
        if (recreate_) restored_ = recreate_(snapshot_);
    }

    [[nodiscard]] std::string_view label()      const override { return "Destroy Entity"; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override {
        return sizeof(*this) + snapshot_.capacity();
    }

private:
    EntityHandle                 handle_;
    std::vector<std::uint8_t>    snapshot_;
    DestroyFn                    destroy_;
    RecreateFn                   recreate_;
    EntityHandle                 restored_ = kNullEntity;
};

// ---------------------------------------------------------------------------
// ReparentEntityCommand — flips a child's parent; no coalescing.
// ---------------------------------------------------------------------------
class ReparentEntityCommand final : public ICommand {
public:
    using ReparentFn = std::function<void(EntityHandle child, EntityHandle new_parent)>;

    ReparentEntityCommand(EntityHandle child,
                           EntityHandle old_parent,
                           EntityHandle new_parent,
                           ReparentFn fn)
        : child_(child)
        , old_parent_(old_parent)
        , new_parent_(new_parent)
        , fn_(std::move(fn)) {}

    void do_()  override { if (fn_) fn_(child_, new_parent_); }
    void undo() override { if (fn_) fn_(child_, old_parent_); }

    [[nodiscard]] std::string_view label()      const override { return "Reparent Entity"; }
    [[nodiscard]] std::size_t      heap_bytes() const noexcept override {
        return sizeof(*this);
    }

private:
    EntityHandle child_;
    EntityHandle old_parent_;
    EntityHandle new_parent_;
    ReparentFn   fn_;
};

}  // namespace gw::editor::undo
