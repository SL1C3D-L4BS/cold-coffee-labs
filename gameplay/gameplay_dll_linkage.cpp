// gameplay/gameplay_dll_linkage.cpp — explicit TU for gameplay subsystem linkage
// (includes retained so gameplay.cpp stays ABI-thin).

#include "gameplay/gameplay_dll_linkage.hpp"

#include "gameplay/a11y/gameplay_a11y.hpp"
#include "gameplay/acts/act_state_component.hpp"
#include "gameplay/boss/god_machine/god_machine.hpp"
#include "gameplay/boss/logos/phase4_selector.hpp"
#include "gameplay/characters/malakor_niccolo/character_state.hpp"
#include "gameplay/combat/encounter_director_bridge.hpp"
#include "gameplay/combat/gore_system.hpp"
#include "gameplay/enemies/abyssal/abyssal.hpp"
#include "gameplay/enemies/cherubim/cherubim.hpp"
#include "gameplay/enemies/deacon/deacon.hpp"
#include "gameplay/enemies/hell_knight/hell_knight.hpp"
#include "gameplay/enemies/leviathan/leviathan.hpp"
#include "gameplay/enemies/martyr/martyr.hpp"
#include "gameplay/enemies/painweaver/painweaver.hpp"
#include "gameplay/enemies/warden/warden.hpp"
#include "gameplay/martyrdom/blasphemy_state.hpp"
#include "gameplay/martyrdom/god_mode.hpp"
#include "gameplay/martyrdom/grace_meter.hpp"
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/stats.hpp"
#include "gameplay/interaction/trigger_zone.hpp"
#include "gameplay/weapons/weapon_system.hpp"

#include "engine/narrative/grace_meter.hpp"
#include "engine/narrative/sin_signature.hpp"
#include "engine/services/director/schema/director.hpp"
#include "engine/services/gore/schema/gore.hpp"

void gw_gameplay_touch_dll_linkage() noexcept {
    static gw::gameplay::a11y::GameplayA11yConfig a11y_cfg{};
    gw::gameplay::a11y::initialize_defaults(a11y_cfg);
    gw::gameplay::a11y::apply(a11y_cfg);

    static gw::gameplay::acts::ActStateComponent       act_state{};
    static gw::gameplay::martyrdom::GodModePresentation god_mode{};
    static gw::gameplay::martyrdom::BlasphemyState      blasphemy{};
    static gw::gameplay::martyrdom::GraceBookkeeping    grace_book{};
    static gw::narrative::GraceComponent                grace{};
    (void)act_state;
    (void)god_mode;
    (void)blasphemy.active();
    gw::narrative::GraceTransaction tx{};
    gw::gameplay::martyrdom::try_apply_grace(grace, grace_book, tx);

    static gw::gameplay::martyrdom::SinComponent          sin_c{};
    static gw::gameplay::martyrdom::MantraComponent       mantra_c{};
    static gw::gameplay::martyrdom::RaptureState         rapture_c{};
    static gw::gameplay::martyrdom::RuinState            ruin_c{};
    static gw::gameplay::martyrdom::ResolvedStats       resolved_c{};
    static gw::gameplay::martyrdom::ActiveBlasphemies   active_b{};
    static gw::gameplay::martyrdom::RaptureTriggerCounter trigger_counter{};
    (void)sin_c;
    (void)mantra_c;
    (void)rapture_c;
    (void)ruin_c;
    (void)resolved_c;
    (void)active_b;
    (void)trigger_counter;

    (void)gw::gameplay::boss::logos::select_phase4_branch(grace);
    gw::narrative::SinSignature sin_fingerprint{};
    sin_fingerprint.precision_ratio = 0.42f;
    (void)gw::gameplay::boss::logos::select_phase4(grace, sin_fingerprint);
    (void)gw::narrative::grace_mechanic_unlocked_for_act(gw::narrative::Act::III);

    gw::gameplay::boss::god_machine::GodMachineState gm{};
    gw::gameplay::boss::god_machine::tick_god_machine(gm, 500.f, 0.05f);
    (void)gw::gameplay::boss::god_machine::is_defeated(gm);

    (void)gw::gameplay::enemies::abyssal::behavior_tree();
    (void)gw::gameplay::enemies::cherubim::behavior_tree();
    (void)gw::gameplay::enemies::deacon::behavior_tree();
    (void)gw::gameplay::enemies::hell_knight::behavior_tree();
    (void)gw::gameplay::enemies::leviathan::behavior_tree();
    (void)gw::gameplay::enemies::martyr::behavior_tree();
    (void)gw::gameplay::enemies::painweaver::behavior_tree();
    (void)gw::gameplay::enemies::warden::behavior_tree();

    gw::services::gore::GoreHitRequest gh{};
    gh.seed      = 0xC0FFEEu;
    gh.damage    = 80.f;
    gh.archetype = 3;
    (void)gw::gameplay::combat::apply_gore_hit(gh);
    gw::services::director::DirectorRequest dr{};
    dr.seed           = 0xD00Du;
    dr.normalized_sin = 0.7f;
    dr.current_state  = gw::services::director::IntensityState::BuildUp;
    (void)gw::gameplay::combat::evaluate_encounter_director(dr);

    for (std::uint8_t wi = 0; wi < static_cast<std::uint8_t>(gw::gameplay::weapons::WeaponId::Count);
         ++wi) {
        gw::gameplay::weapons::WeaponRuntimeState wrt{};
        gw::gameplay::weapons::init_default_runtime(static_cast<gw::gameplay::weapons::WeaponId>(wi),
                                                    wrt);
        gw::gameplay::martyrdom::ResolvedStats wrs{};
        (void)gw::gameplay::weapons::fire_primary(static_cast<gw::gameplay::weapons::WeaponId>(wi),
                                                  wrs, wrt, 0.016f);
    }

    static gw::gameplay::characters::malakor_niccolo::CharacterState malakor_state{};
    (void)malakor_state;

    using gw::gameplay::interaction::advance_door_towards_open;
    using gw::gameplay::interaction::AxisAlignedZone;
    using gw::gameplay::interaction::DoorSlideState;
    using gw::gameplay::interaction::pickup_is_available;
    using gw::gameplay::interaction::PickupRespawnState;
    using gw::gameplay::interaction::tick_pickup_respawn;
    using gw::gameplay::interaction::zones_overlap;
    static AxisAlignedZone trig_a{
        .min_x = 0.f, .min_y = 0.f, .min_z = 0.f, .max_x = 1.f, .max_y = 1.f, .max_z = 1.f};
    static AxisAlignedZone trig_b{
        .min_x = 0.5f, .min_y = 0.f, .min_z = 0.f, .max_x = 1.5f, .max_y = 1.f, .max_z = 1.f};
    (void)zones_overlap(trig_a, trig_b);
    static DoorSlideState door{};
    (void)advance_door_towards_open(door, 1.f / 60.f, 2.f);
    static PickupRespawnState pk{.frames_until_spawn = 2u};
    tick_pickup_respawn(pk);
    (void)pickup_is_available(pk);
}
