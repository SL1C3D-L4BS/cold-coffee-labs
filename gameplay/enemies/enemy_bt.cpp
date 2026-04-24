// gameplay/enemies/enemy_bt.cpp — Phase 23 W149–W150

#include "gameplay/enemies/enemy_bt.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace gw::gameplay::enemies {
namespace {

using gw::gameai::BBValue;
using gw::gameai::BTContext;
using gw::gameai::BTDesc;
using gw::gameai::BTNodeDesc;
using gw::gameai::BTNodeKind;
using gw::gameai::BTStatus;
using gw::gameai::Blackboard;

float bb_float(const Blackboard& bb, std::string_view k, float def) noexcept {
    if (!bb.has(k)) return def;
    const auto v = bb.get(k);
    return v.kind == gw::gameai::BBKind::Float ? v.f32 : def;
}

void bb_set_float(Blackboard& bb, std::string_view k, float f) noexcept {
    bb.set_float(k, f);
}

bool bb_bool(const Blackboard& bb, std::string_view k, bool def) noexcept {
    if (!bb.has(k)) return def;
    const auto v = bb.get(k);
    return v.kind == gw::gameai::BBKind::Bool ? v.b : def;
}

void bb_set_bool(Blackboard& bb, std::string_view k, bool b) noexcept {
    bb.set_bool(k, b);
}

gw::gameai::BTStatus act_chase(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 2.f);
    if (dist <= range) return BTStatus::Success;
    const float step = std::max(0.5f, dist * 0.15f);
    bb_set_float(*bb, bb::kDist, std::max(range, dist - step));
    return BTStatus::Running;
}

gw::gameai::BTStatus act_die(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    bb_set_bool(*bb, bb::kDead, true);
    bb_set_float(*bb, bb::kHp, 0.f);
    return BTStatus::Success;
}

gw::gameai::BTStatus bump_attacks(Blackboard& bb) noexcept {
    const float n = bb_float(bb, bb::kAttacks, 0.f);
    bb.set_float(bb::kAttacks, n + 1.f);
    return BTStatus::Success;
}

gw::gameai::BTStatus attack_generic(float& dist, float range, Blackboard& bb,
                                    void (*shape)(float& dist, float range)) noexcept {
    if (dist > range) return BTStatus::Failure;
    if (shape) shape(dist, range);
    bb_set_float(bb, bb::kDist, dist);
    return bump_attacks(bb);
}

void shape_strafe(float& dist, float range) noexcept {
    const float wobble = 0.15f * std::sin(dist * 8.371f);
    dist               = std::clamp(range + wobble, range * 0.85f, range * 1.05f);
}

void shape_teleport(float& dist, float range) noexcept {
    if (dist > range * 1.1f) {
        dist = range * 0.85f;
    }
}

gw::gameai::BTStatus act_attack_cherubim(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 2.5f);
    return attack_generic(dist, range, *bb, shape_strafe);
}

gw::gameai::BTStatus act_attack_deacon(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 2.2f);
    shape_teleport(dist, range);
    bb_set_float(*bb, bb::kDist, dist);
    return attack_generic(dist, range, *bb, nullptr);
}

gw::gameai::BTStatus act_attack_leviathan(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float        dist  = bb_float(*bb, bb::kDist, 999.f);
    const float  range = bb_float(*bb, bb::kAttackRange, 3.f);
    const auto   ph    = static_cast<std::uint8_t>(bb_float(*bb, bb::kPhase, 0.f));
    if (ph == 0 && dist > range) {
        bb_set_float(*bb, bb::kPhase, 1.f);
        dist *= 0.5f;
        bb_set_float(*bb, bb::kDist, dist);
        return BTStatus::Running;
    }
    if (dist > range) return BTStatus::Failure;
    bb_set_float(*bb, bb::kPhase, 0.f);
    bb_set_float(*bb, bb::kDist, dist);
    return bump_attacks(*bb);
}

gw::gameai::BTStatus act_attack_warden(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    const float adds = bb_float(*bb, bb::kAdds, 0.f);
    if (adds > 0.5f) return BTStatus::Running;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 2.8f);
    if (dist > range) return BTStatus::Failure;
    return bump_attacks(*bb);
}

gw::gameai::BTStatus act_attack_martyr(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 2.f);
    if (dist > range) return BTStatus::Failure;
    bb_set_bool(*bb, bb::kExplode, true);
    return bump_attacks(*bb);
}

gw::gameai::BTStatus act_attack_hell_knight(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 2.5f);
    if (dist > range * 1.05f) return BTStatus::Failure;
    return bump_attacks(*bb);
}

gw::gameai::BTStatus act_attack_painweaver(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 4.f);
    if (dist > range) return BTStatus::Failure;
    bb_set_bool(*bb, bb::kSinPaused, true);
    return bump_attacks(*bb);
}

gw::gameai::BTStatus act_attack_abyssal(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return BTStatus::Failure;
    float dist  = bb_float(*bb, bb::kDist, 999.f);
    float range = bb_float(*bb, bb::kAttackRange, 6.f);
    if (dist > range) return BTStatus::Failure;
    const float h = bb_float(*bb, bb::kHoming, 0.f);
    bb_set_float(*bb, bb::kHoming, h + 3.f);
    return bump_attacks(*bb);
}

bool cond_hp_lte_zero(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return true;
    return bb_float(*bb, bb::kHp, 0.f) <= 0.f || bb_bool(*bb, bb::kDead, false);
}

bool cond_in_range(void*, BTContext& ctx) noexcept {
    auto* bb = ctx.blackboard;
    if (!bb) return false;
    const float d = bb_float(*bb, bb::kDist, 999.f);
    const float r = bb_float(*bb, bb::kAttackRange, 2.f);
    return d <= r;
}

bool cond_not_in_range(void*, BTContext& ctx) noexcept {
    return !cond_in_range(nullptr, ctx);
}

/// Build standard combat tree: root selector — die | attack | chase.
/// `attack_name` is registry key for archetype attack leaf.
BTDesc make_tree(std::string_view tree_name, std::string_view attack_name) noexcept {
    BTDesc t{};
    t.name = std::string{tree_name};
    // Indices: 0 root Sel {1,4,7}, 1 Seq die {2,3}, 2 cond hp, 3 act die,
    //          4 Seq atk {5,6}, 5 cond range, 6 act atk, 7 chase
    BTNodeDesc root{};
    root.kind     = BTNodeKind::Selector;
    root.children = {1, 4, 7};

    BTNodeDesc seq_die{};
    seq_die.kind     = BTNodeKind::Sequence;
    seq_die.children = {2, 3};

    BTNodeDesc c_hp{};
    c_hp.kind = BTNodeKind::ConditionLeaf;
    c_hp.name = "enemy_hp_lte_zero";

    BTNodeDesc a_die{};
    a_die.kind = BTNodeKind::ActionLeaf;
    a_die.name = "enemy_die";

    BTNodeDesc seq_atk{};
    seq_atk.kind     = BTNodeKind::Sequence;
    seq_atk.children = {5, 6};

    BTNodeDesc c_rng{};
    c_rng.kind = BTNodeKind::ConditionLeaf;
    c_rng.name = "enemy_in_range";

    BTNodeDesc a_atk{};
    a_atk.kind = BTNodeKind::ActionLeaf;
    a_atk.name = std::string{attack_name};

    BTNodeDesc a_chase{};
    a_chase.kind = BTNodeKind::ActionLeaf;
    a_chase.name = "enemy_chase";

    t.nodes.push_back(root);
    t.nodes.push_back(seq_die);
    t.nodes.push_back(c_hp);
    t.nodes.push_back(a_die);
    t.nodes.push_back(seq_atk);
    t.nodes.push_back(c_rng);
    t.nodes.push_back(a_atk);
    t.nodes.push_back(a_chase);
    t.root_index = 0;
    return t;
}

} // namespace

void register_sacrilege_enemy_behaviors(gw::gameai::GameAIWorld& world) noexcept {
    auto& reg = world.action_registry();
    reg.register_action("enemy_chase", act_chase, nullptr);
    reg.register_action("enemy_die", act_die, nullptr);
    reg.register_condition("enemy_hp_lte_zero", cond_hp_lte_zero, nullptr);
    reg.register_condition("enemy_in_range", cond_in_range, nullptr);
    reg.register_condition("enemy_not_in_range", cond_not_in_range, nullptr);

    reg.register_action("attack_cherubim", act_attack_cherubim, nullptr);
    reg.register_action("attack_deacon", act_attack_deacon, nullptr);
    reg.register_action("attack_leviathan", act_attack_leviathan, nullptr);
    reg.register_action("attack_warden", act_attack_warden, nullptr);
    reg.register_action("attack_martyr", act_attack_martyr, nullptr);
    reg.register_action("attack_hell_knight", act_attack_hell_knight, nullptr);
    reg.register_action("attack_painweaver", act_attack_painweaver, nullptr);
    reg.register_action("attack_abyssal", act_attack_abyssal, nullptr);
}

gw::gameai::BTDesc make_enemy_combat_tree(ArchetypeId id) noexcept {
    switch (id) {
    case ArchetypeId::Cherubim:
        return make_tree("bt_cherubim", "attack_cherubim");
    case ArchetypeId::Deacon:
        return make_tree("bt_deacon", "attack_deacon");
    case ArchetypeId::Leviathan:
        return make_tree("bt_leviathan", "attack_leviathan");
    case ArchetypeId::Warden:
        return make_tree("bt_warden", "attack_warden");
    case ArchetypeId::Martyr:
        return make_tree("bt_martyr", "attack_martyr");
    case ArchetypeId::HellKnight:
        return make_tree("bt_hell_knight", "attack_hell_knight");
    case ArchetypeId::Painweaver:
        return make_tree("bt_painweaver", "attack_painweaver");
    case ArchetypeId::Abyssal:
        return make_tree("bt_abyssal", "attack_abyssal");
    default:
        return make_tree("bt_unknown", "attack_cherubim");
    }
}

void simulate_enemy_tick(EnemyCombatState& st, ArchetypeId id, float chase_speed,
                         float dt) noexcept {
    if (st.dead || st.hp <= 0.f) {
        st.dead = true;
        return;
    }
    const float step = chase_speed * dt;
    switch (id) {
    case ArchetypeId::Warden:
        if (st.adds_alive > 0) {
            return;
        }
        break;
    case ArchetypeId::Martyr:
        st.explode_on_death = st.hp < st.max_hp * 0.2f;
        break;
    default:
        break;
    }
    if (st.dist_to_player > st.attack_range) {
        st.dist_to_player = std::max(st.attack_range, st.dist_to_player - step);
    } else {
        ++st.attacks_landed;
        switch (id) {
        case ArchetypeId::Painweaver:
            st.sin_paused = true;
            break;
        case ArchetypeId::Abyssal:
            st.homing_volleys = static_cast<std::uint8_t>(
                std::min(255, static_cast<int>(st.homing_volleys) + 3));
            break;
        case ArchetypeId::Deacon:
            st.dist_to_player = st.attack_range * 0.9f;
            break;
        default:
            break;
        }
    }
}

} // namespace gw::gameplay::enemies
