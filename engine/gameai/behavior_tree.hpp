#pragma once
// engine/gameai/behavior_tree.hpp — Phase 13 Wave 13E (ADR-0043).
//
// A BehaviorTree is stored as a tightly-packed node array in topological
// order (parent after all its children). Each node has a kind tag and
// ADR-0043 §2 semantics. The executor is a deterministic tick function
// that returns a BTStatus for the root.
//
// The "tree" as authored in the editor lowers into the same data layout
// the vscript IR writes via `bt_from_vscript_ir()` — guaranteeing
// semantic + bit-for-bit parity between editor and BLD-chat outputs.

#include "engine/gameai/blackboard.hpp"
#include "engine/gameai/gameai_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gw::gameai {

// -----------------------------------------------------------------------------
// Node kinds — complete ADR-0043 §2 vocabulary.
// -----------------------------------------------------------------------------
enum class BTNodeKind : std::uint8_t {
    // Composites
    Sequence   = 0,
    Selector   = 1,
    Parallel   = 2,
    // Decorators
    Inverter   = 10,
    Succeeder  = 11,
    RepeatN    = 12,
    UntilSuccess = 13,
    UntilFailure = 14,
    Cooldown   = 15,
    // Leaves
    ActionLeaf     = 30,
    ConditionLeaf  = 31,
    // Blackboard ops
    BBSetFloat  = 40,
    BBSetInt    = 41,
    BBSetBool   = 42,
    BBCompareGT = 43,
    BBCompareLT = 44,
    BBCompareEQ = 45,
};

// -----------------------------------------------------------------------------
// ActionRegistry — engine-side lookup of leaf callbacks by name. The BT
// executor invokes `ActionFn(instance, blackboard, dt)` for every
// ActionLeaf / ConditionLeaf node. Registering is cheap and not a hot
// path; lookups are `std::unordered_map`.
// -----------------------------------------------------------------------------
struct BTContext {
    BehaviorInstanceHandle instance{};
    EntityId               entity{kEntityNone};
    Blackboard*            blackboard{nullptr};
    float                  dt_s{0.0f};
};

using ActionFn    = BTStatus (*)(void* user, BTContext& ctx);
using ConditionFn = bool     (*)(void* user, BTContext& ctx);

class ActionRegistry {
public:
    void register_action    (std::string_view name, ActionFn fn, void* user = nullptr);
    void register_condition (std::string_view name, ConditionFn fn, void* user = nullptr);

    [[nodiscard]] BTStatus invoke_action   (std::string_view name, BTContext& ctx) const;
    [[nodiscard]] bool     invoke_condition(std::string_view name, BTContext& ctx) const;

    [[nodiscard]] bool has_action   (std::string_view name) const noexcept;
    [[nodiscard]] bool has_condition(std::string_view name) const noexcept;

private:
    struct Entry { ActionFn    fn{nullptr}; void* user{nullptr}; };
    struct EntryC{ ConditionFn fn{nullptr}; void* user{nullptr}; };
    std::unordered_map<std::string, Entry>  actions_{};
    std::unordered_map<std::string, EntryC> conditions_{};
};

// -----------------------------------------------------------------------------
// BTNodeDesc — author-time data for a single node. Children are indices
// into the same node array and must be > this node's own index
// (post-order topological sort). An empty children[] means leaf.
// -----------------------------------------------------------------------------
struct BTNodeDesc {
    BTNodeKind                 kind{BTNodeKind::Sequence};
    std::string                name{};       // for actions/conditions: registry key
    std::string                bb_key{};     // for BB* ops
    float                      f_value{0.0f};
    std::int32_t               i_value{0};
    bool                       b_value{false};
    std::uint16_t              repeat_count{0};
    float                      cooldown_s{0.0f};
    std::vector<std::uint16_t> children{};
};

struct BTDesc {
    std::string               name{};
    std::vector<BTNodeDesc>   nodes{};      // topological order (root last)
    std::uint16_t             root_index{0};
};

// Compile-time validator — returns 0 on success.
enum BTValidationError : std::uint32_t {
    kBT_V_OK                    = 0,
    kBT_V_ChildOutOfRange       = 1u << 0,
    kBT_V_ChildIndexBeforeSelf  = 1u << 1,
    kBT_V_EmptyTree             = 1u << 2,
    kBT_V_RootOutOfRange        = 1u << 3,
    kBT_V_CompositeNoChildren   = 1u << 4,
    kBT_V_DecoratorNotOneChild  = 1u << 5,
};
[[nodiscard]] std::uint32_t validate_bt(const BTDesc& desc) noexcept;

// Deterministic content hash — FNV-1a-64 over node array + fields.
[[nodiscard]] std::uint64_t bt_content_hash(const BTDesc& desc) noexcept;

} // namespace gw::gameai
