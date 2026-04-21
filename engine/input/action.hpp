#pragma once
// engine/input/action.hpp — typed Actions + ActionSet + context stack.
//
// ADR-0021 is authoritative. Actions produce typed output (bool/float/Vec2/Vec3/u32)
// from a chain of Bindings, passed through a Processor, with optional
// accessibility modifiers (hold-to-toggle, auto-repeat, assist window,
// can_be_invoked_while_paused).

#include "engine/input/device.hpp"
#include "engine/input/input_types.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace gw::input {

enum class BindingSource : uint8_t {
    None = 0,
    Keyboard,
    Mouse,
    MouseAxis,
    GamepadButton,
    GamepadAxis,
    GamepadGyro,
    CompositeWASD,  // synthesised 2D from four keys
};

// A single binding. For CompositeWASD, `code` indexes the four keys as
// fva/fvb/fvc/fvd (up/down/left/right) packed into fval[0..3] of the Action
// at load time (see action_map_toml).
struct Binding {
    BindingSource source{BindingSource::None};
    uint32_t      code{0};
    ModifierMask  modifiers{0};
    float         threshold{0.0f};   // for analog-as-button
    bool          inverted{false};
    // For CompositeWASD we need four key codes; packed below:
    uint16_t      composite_keys[4]{0, 0, 0, 0};  // up/down/left/right
};

// Accessibility flags per Action (ADR-0021 §2.1, §2.7, §2.8).
struct ActionAccessibility {
    bool  hold_to_toggle{false};
    float auto_repeat_ms{0.0f};
    float assist_window_ms{0.0f};
    float repeat_cooldown_ms{0.0f};
    bool  can_be_invoked_while_paused{false};
    int32_t scan_index{-1};          // used by single-switch scanner; -1 = exclude
};

enum class ActionOutputKind : uint8_t {
    Bool = 0, Float, Vec2, Vec3, U32,
};

// Output value variant.
struct ActionValue {
    ActionOutputKind kind{ActionOutputKind::Bool};
    bool             b{false};
    float            f{0.0f};
    math::Vec2f      v2{0.0f, 0.0f};
    math::Vec3f      v3{0.0f, 0.0f, 0.0f};
    uint32_t         u{0};
};

// Hold-to-toggle / auto-repeat state per Action instance.
struct ActionRuntimeState {
    bool  last_pressed{false};
    bool  latched{false};
    // Set when a double-tap un-latches on rising edge; the next falling edge
    // suppresses its re-latch so the un-latched state sticks (ADR-0021 §2.7).
    bool  suppress_next_latch{false};
    float last_press_time_ms{-1000.0f};
    float last_release_time_ms{-1000.0f};
    float last_repeat_time_ms{-1000.0f};
    float last_activation_time_ms{-1000.0f};  // for cooldown
    // Smoothing state used by the Processor.
    float        scalar_state{0.0f};
    math::Vec2f  vec2_state{0.0f, 0.0f};
};

struct Action {
    std::string          name;
    ActionOutputKind     output{ActionOutputKind::Bool};
    std::vector<Binding> bindings;
    Processor            processor{};
    ActionAccessibility  accessibility{};
    ActionRuntimeState   state{};
    ActionValue          value{};
};

class ActionSet {
public:
    explicit ActionSet(std::string name) : name_(std::move(name)) {}
    ~ActionSet() = default;

    // Add or overwrite an action (by name).
    Action& add(Action a);
    [[nodiscard]] Action* find(std::string_view name) noexcept;
    [[nodiscard]] const Action* find(std::string_view name) const noexcept;
    [[nodiscard]] std::vector<Action>& actions() noexcept { return actions_; }
    [[nodiscard]] const std::vector<Action>& actions() const noexcept { return actions_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

private:
    std::string         name_;
    std::vector<Action> actions_;
};

// Context stack: the topmost set's bindings are evaluated first. Lower
// sets can fall through if the topmost does not define (or does not consume)
// the relevant verb.
class ContextStack {
public:
    void push(ActionSet* set) { stack_.push_back(set); }
    void pop() { if (!stack_.empty()) stack_.pop_back(); }

    [[nodiscard]] ActionSet* top() const noexcept {
        return stack_.empty() ? nullptr : stack_.back();
    }
    [[nodiscard]] std::size_t size() const noexcept { return stack_.size(); }
    [[nodiscard]] const std::vector<ActionSet*>& stack() const noexcept { return stack_; }

private:
    std::vector<ActionSet*> stack_;
};

// Evaluate an action given the snapshot and the delta-time since last
// evaluation (for auto-repeat / hold-to-toggle timing). Populates
// `action.value` and updates `action.state`.
void evaluate_action(Action& action, const RawSnapshot& snap, float now_ms) noexcept;

// Collapse all actions in a set for this frame.
void evaluate_set(ActionSet& set, const RawSnapshot& snap, float now_ms) noexcept;

// ---------------------------------------------------------------------------
// Raw-binding evaluator (shared by tests and the action runtime). Returns a
// scalar in [0, 1] representing the "activation strength" of this binding
// against the snapshot.
// ---------------------------------------------------------------------------
[[nodiscard]] float evaluate_binding(const Binding& b, const RawSnapshot& snap) noexcept;

// Returns the 2D composite for a CompositeWASD binding (or zero).
[[nodiscard]] math::Vec2f evaluate_composite(const Binding& b, const RawSnapshot& snap) noexcept;

} // namespace gw::input
