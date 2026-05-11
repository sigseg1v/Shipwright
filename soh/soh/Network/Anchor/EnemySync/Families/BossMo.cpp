#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#define this thisx
#include "src/overlays/actors/ovl_Boss_Mo/z_boss_mo.h"
#undef this
}

// Morpha. ColliderJntSph tentCollider and ColliderCylinder coreCollider.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossMo*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->tentCollider);
    EnemySyncHelpers::RegisterCyl(actor, &a->coreCollider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossMo*>(actor);
    a->tentCollider.base.acFlags &= ~AC_HIT;
    a->coreCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_MO, &RegisterAC, &ClearACHits, nullptr, nullptr }));
