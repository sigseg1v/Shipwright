#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Sst/z_boss_sst.h"
#undef this

// Bongo Bongo. ColliderJntSph colliderJntSph (hands/head spheres) and
// ColliderCylinder colliderCyl.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossSst*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->colliderJntSph);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderCyl);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossSst*>(actor);
    a->colliderJntSph.base.acFlags &= ~AC_HIT;
    a->colliderCyl.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossSst*>(actor);
    a->colliderJntSph.base.acFlags |= AC_HIT;
    a->colliderCyl.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_SST, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
