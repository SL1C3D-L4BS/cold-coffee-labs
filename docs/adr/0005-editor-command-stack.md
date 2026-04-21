# ADR 0005 — Editor CommandStack (undo / redo, transactions, coalescing)

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the never-delivered Phase-3 week-011 `engine/core/command/` `CommandStack` (see `docs/05 §Phase-3 amendment`).
- **Superseded by:** —
- **Related:** `docs/AUDIT_MAP_2026-04-20.md §P1-11`; ADR-0004 (ECS-World — the primary target of command write-through); `CLAUDE.md` non-negotiables #5 (no raw `new`/`delete` — use `std::unique_ptr`), #20 (doctrine first).

## 1. Context

Phase-3 week 011 promised `engine/core/command/{ICommand,CommandStack,transaction brackets}`. The directory landed with only a `.gitkeep`; no code. `editor/panels/inspector_panel.cpp` imports a `SetComponentFieldCommand<T>` and a `ReparentEntityCommand`, suggesting there's *some* command infrastructure already — but the implementation of those commands captures the post-edit value on both `before` and `after` sides (`inspector_panel.cpp:55-60`), meaning undo restores the post-edit state, which is no undo at all.

*Editor v0.1* needs real transactional edits:

- Every Inspector field edit should be undoable (typed values, gizmo drags, component add/remove).
- Rapid drags (gizmo, drag-float fields) should coalesce into a single undoable transaction.
- Multi-field edits ("set position and rotation at once") should group.
- Ctrl+Z / Ctrl+Y bindings; the Edit menu shows the next undo/redo labels.
- The stack has a memory cap; old entries drop when it's exceeded.

The Phase-3 location (`engine/core/command/`) turned out to be wrong in hindsight: the only consumers are the editor and, prospectively, BLD's transaction hooks (which are themselves editor-scoped). Putting this in `engine/core/` would drag gameplay code into a dependency it doesn't need. Editor-scoped is the right home.

## 2. Decision

### 2.1 Location

`editor/undo/command_stack.{hpp,cpp}` (+ `editor/undo/commands/` for concrete command types as they accrete). The `engine/core/command/` directory stays empty / deleted; if an engine-side command surface ever becomes necessary (serialized BLD actions, replay, deterministic-sim rewind), it gets its own ADR.

### 2.2 `ICommand` interface

```cpp
namespace gw::editor::undo {

class ICommand {
public:
    virtual ~ICommand() = default;

    // Must be symmetric: do() followed by undo() returns state to the pre-do snapshot.
    virtual void do_()   = 0;
    virtual void undo()  = 0;

    // Human-readable label for the Edit menu and the undo-stack panel.
    [[nodiscard]] virtual std::string_view label() const = 0;

    // Approximate heap footprint in bytes for memory-cap bookkeeping.
    // Must return a conservative upper bound; used to decide when to drop
    // oldest entries when the stack exceeds its size budget.
    [[nodiscard]] virtual std::size_t heap_bytes() const noexcept = 0;

    // Coalescing: if this command would logically merge with `other` (same
    // target, compatible operation, recent enough by `time`), absorb other's
    // terminal state and return true. Called by CommandStack::push when
    // coalescing is on and the previous command is of a compatible type.
    //
    // Default: no coalescing.
    [[nodiscard]] virtual bool try_coalesce(const ICommand& /*other*/,
                                            std::chrono::steady_clock::time_point /*now*/) {
        return false;
    }
};

} // namespace gw::editor::undo
```

`do_` has an underscore because `do` is a C++ keyword.

### 2.3 `CommandStack`

```cpp
namespace gw::editor::undo {

class CommandStack {
public:
    struct Config {
        std::size_t memory_budget_bytes = 16 * 1024 * 1024;  // 16 MiB default
        std::chrono::milliseconds coalesce_window{120};      // ≈8 frames @ 60fps
    };

    explicit CommandStack(Config cfg = {});
    ~CommandStack();

    // Push a new command. Executes do_() synchronously. Clears the redo stack.
    // Attempts coalescing with the top of the undo stack when enabled.
    void push(std::unique_ptr<ICommand> cmd);

    // Undo / redo. No-op if respective stack is empty.
    void undo();
    void redo();

    [[nodiscard]] bool can_undo() const noexcept;
    [[nodiscard]] bool can_redo() const noexcept;

    // Labels for menu display; empty string if stack is empty.
    [[nodiscard]] std::string_view next_undo_label() const noexcept;
    [[nodiscard]] std::string_view next_redo_label() const noexcept;

    // Group a sequence of pushes into a single undoable unit.
    // Nesting allowed; outermost begin/end pair is the boundary.
    void begin_group(std::string_view label);
    void end_group();

    // Explicit coalescing control (default: on).
    void set_coalescing_enabled(bool) noexcept;

    // Stats for debug overlay.
    [[nodiscard]] std::size_t undo_depth()  const noexcept;
    [[nodiscard]] std::size_t redo_depth()  const noexcept;
    [[nodiscard]] std::size_t memory_used() const noexcept;

    void clear();

private:
    // Implementation: two std::vector<std::unique_ptr<ICommand>> (undo, redo),
    // a GroupCommand aggregator during begin/end, running memory-bytes counter,
    // time-of-last-push for coalesce-window gating.
};

} // namespace gw::editor::undo
```

### 2.4 Transaction semantics

- **`push(cmd)`**
  1. If a group is open (`group_depth_ > 0`), the command is buffered into the current `GroupCommand`, not the stack directly. `do_` is still called.
  2. Else: call `cmd->do_()`; attempt coalescing with the top of the undo stack (if the top is of the same concrete type *and* `top->try_coalesce(*cmd, now)` returns true *and* `now - last_push_time_ ≤ coalesce_window`); if coalesced, replace; else push onto undo stack.
  3. Clear the redo stack.
  4. Trim from the front of the undo stack while `memory_used > memory_budget_bytes`.
  5. Record `last_push_time_ = now`.

- **`undo`**
  1. If undo stack empty, no-op.
  2. Pop top; call `.undo()`; push onto redo stack.
  3. Reset `last_push_time_` to a far-past sentinel so the next `push` cannot coalesce with the popped command.

- **`redo`**
  1. If redo stack empty, no-op.
  2. Pop top; call `.do_()`; push onto undo stack.
  3. Reset `last_push_time_` likewise.

- **`begin_group(label)` / `end_group()`**
  - Maintain `group_depth_`; create a fresh `GroupCommand` on depth transition 0 → 1.
  - Buffered commands are owned by the group.
  - `end_group()` on depth transition 1 → 0 pushes the group onto the undo stack as a single command. Empty groups are discarded.
  - `group_depth_ > 0` disables coalescing with prior stack entries (within a group we coalesce *within the group only*, which is the `GroupCommand`'s job to handle).

### 2.5 `GroupCommand`

A built-in `ICommand` implementation that owns a `std::vector<std::unique_ptr<ICommand>>` and forwards `do_`/`undo` (undo in reverse order). `heap_bytes()` returns the sum of children + container overhead. `label()` is the caller-supplied group label. Does not support coalescing with other groups (returns false).

### 2.6 Coalescing — the drag case

The canonical coalesce case is **drag on a drag-float / gizmo**: each mouse-move emits a `SetComponentFieldCommand<float>` or `TransformEntityCommand`. Without coalescing the undo stack would collect hundreds of micro-changes, one per mouse-move frame. With coalescing they merge into a single undoable edit from *pre-drag value* to *post-release value*.

**Coalescing rule in concrete commands:**

- `try_coalesce(other, now)` returns true iff:
  1. `other` is same concrete type (same `T` for `SetComponentFieldCommand<T>`, same entity for `TransformEntityCommand`).
  2. Same target (same field address / same entity + same component).
  3. `now - self.timestamp <= window` (the stack gates this too; belt-and-braces).
  4. `self.before` value is preserved; `self.after` is replaced with `other.after`. The command's `do_()` should be idempotent — re-applying the updated `after` from the coalesced state is equivalent to re-playing every step.

- Commands that are not idempotent under value-replacement (add/remove component, reparent, create/destroy entity) **return false** unconditionally from `try_coalesce`.

### 2.7 Concrete commands landed in this PR block

- `SetComponentFieldCommand<T>` — one field on one component on one entity. Stores `{Entity, ComponentTypeId, field_offset, before: T, after: T}`. Coalesces on same `{entity, component, field_offset}`.
- `AddComponentCommand<T>` — add a default-constructed or caller-provided `T` to an entity. Not coalescing.
- `RemoveComponentCommand<T>` — snapshot the component's current state via `ComponentRegistry::info(id).reflection` (or raw `memcpy` for trivial types) before removal; undo re-adds from the snapshot. Not coalescing.
- `CreateEntityCommand` / `DestroyEntityCommand` — paired destroy/create; `DestroyEntityCommand` snapshots every component via the serialization path (ADR-0007). Not coalescing.
- `ReparentEntityCommand` — stores `{child, old_parent, new_parent}`; the existing inspector call site gets wired up (its callback was a no-op per the audit). Not coalescing.

Additional commands (multi-select edits, asset-field drops, etc.) accrete later; they implement `ICommand` with the same shape.

### 2.8 Input bindings (Ctrl+Z / Ctrl+Y)

Handled in the editor's global shortcut layer (`editor/app/editor_app.cpp` or a dedicated `editor/input/shortcuts.cpp` if this layer grows). `CommandStack` itself is keyboard-unaware.

### 2.9 Memory policy

- Memory is accounted via `ICommand::heap_bytes()`. Trim policy is FIFO (drop oldest from front of undo stack) until `memory_used <= memory_budget`. The redo stack is not trimmed explicitly — it's always emptied on a fresh push.
- `std::unique_ptr<ICommand>` is the ownership primitive. No raw `new`/`delete` in consumer code (NN #5): a helper `make_command<Cmd>(args...)` wraps `std::make_unique`. Consumers call `stack.push(make_set_field_command<float>(...))`.

### 2.10 Out of scope (explicitly)

- **Persistent undo across editor sessions.** Not useful for Editor v0.1; Phase-8+.
- **Replay / determinism.** `CommandStack` is for UI actions, not for sim state. BLD's transaction hooks (Phase 9) will probably reuse `ICommand` but will have their own Phase-9 ADR; today we only need editor undo.
- **Multi-user editing / OT / CRDT.** Out of scope for the entire project unless Greywater grows a multiplayer-editor story.
- **Asynchronous commands.** Every command runs synchronously on the editor thread; `do_()` / `undo()` may not suspend. If something must happen async (a long asset import), it completes first, then a synchronous command records the result.

## 3. Consequences

### Immediate (this commit block)

- `editor/undo/command_stack.{hpp,cpp}`, `editor/undo/commands/*.{hpp,cpp}` land.
- `editor/panels/inspector_panel.cpp`'s existing `SetComponentFieldCommand` inline usage is replaced with the ADR-shaped command; its `before` capture is corrected to snapshot *before* the widget overwrites (the current sequence reads post-value-both-sides, which is why undo is effectively broken).
- `editor/app/editor_app.cpp` owns one `CommandStack` in `EditorContext`; Ctrl+Z / Ctrl+Y bound in the global shortcut layer.
- Unit tests: `tests/unit/editor_undo/` — push/undo/redo round-trip, coalesce gating (same target, time window), group semantics (begin/end, nesting), memory-budget trim, redo clear on fresh push.

### Short term (Phase 7 remainder)

- Every inspector write goes through a command. Gizmo drags emit coalesced `TransformEntityCommand`s. Outliner reparent/delete use `ReparentEntityCommand` / `DestroyEntityCommand` with real callbacks (not the no-op the audit found).
- Viewport-initiated transform gizmo → coalesced transform command stream — one undo per release.

### Long term

- When BLD (Phase 9) needs to record authored actions, it layers on top of `ICommand`. Whether BLD commands live on the same stack or a dedicated one is deferred to the BLD ADR.
- If we ever need sim-state rewind (replay, determinism testing), a separate `engine/core/replay/` subsystem can pattern-match the API but is explicitly not the same code — that ADR is not this ADR.

### Alternatives considered

1. **Snapshot-diff undo (store full pre/post world snapshots, no command objects).** Rejected as too memory-heavy and too coarse — a single field edit would snapshot the whole world.

2. **Event-sourced model (everything is a command; render state is a pure function of the command log).** Rejected as far too much machinery for a UI-edit undo stack. Worth reconsidering *only* if BLD Phase 9 demands it, and then with a fresh ADR.

3. **Land this in `engine/core/command/` as Phase-3 originally planned.** Rejected. The only consumers are editor + prospective BLD (editor-scoped). Engine core should not know about undo stacks; it would cause spurious deps.

4. **Use QUndoStack / a third-party library.** We're not on Qt; no third-party C++ undo library is worth the dep for the shape we need. Rolled our own.

5. **Auto-coalesce on type alone (Unity-style).** Rejected. Without the time-window gate, coalescing fires across unrelated edits of the same type (two separate transform edits on two entities over several seconds would merge into one undoable). Time-window + same-target is the minimum correct gate.

## 4. References

- `docs/AUDIT_MAP_2026-04-20.md §P1-11`
- `docs/05 §Phase-3 amendment`
- ADR-0004 (ECS-World)
- `editor/panels/inspector_panel.cpp:42-88` — current broken command flow
- `CLAUDE.md` non-negotiables #5, #20

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*
