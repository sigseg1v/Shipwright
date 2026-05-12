#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Poh/z_en_poh.h"

// Poe. One ColliderCylinder (lantern/body) plus one ColliderJntSph
// (soul).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnPoh*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderCyl);
    EnemySyncHelpers::RegisterJntSph(&a->colliderSph);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPoh*>(actor);
    a->colliderCyl.base.acFlags &= ~AC_HIT;
    a->colliderSph.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPoh*>(actor);
    // En_Poh reads colliderCyl.info.acHitInfo->toucher.dmgFlags when
    // AC_HIT is set; seed acHitInfo too.
    EnemySyncHelpers::SetCylAcHit(&a->colliderCyl);
    EnemySyncHelpers::SetJntSphAcHit(&a->colliderSph);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_POH, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
