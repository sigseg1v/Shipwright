#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Tubo_Trap/z_en_tubo_trap.h"
}

// Flying jar trap. Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnTuboTrap*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnTuboTrap*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_TUBO_TRAP, &RegisterAC, &ClearACHits, nullptr, nullptr }));
