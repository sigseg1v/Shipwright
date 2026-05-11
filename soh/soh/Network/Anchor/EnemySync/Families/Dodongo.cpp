#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Dodongo/z_en_dodongo.h"

// Dodongo (the big lizard miniboss spawned in dungeons / KDR). AT ColliderQuad,
// hard-skin ColliderTris, body ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnDodongo*>(actor);
    EnemySyncHelpers::RegisterColliderCommon(&a->colliderAT.base);
    EnemySyncHelpers::RegisterColliderCommon(&a->colliderHard.base);
    EnemySyncHelpers::RegisterJntSph(&a->colliderBody);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDodongo*>(actor);
    a->colliderAT.base.acFlags &= ~AC_HIT;
    a->colliderHard.base.acFlags &= ~AC_HIT;
    a->colliderBody.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_DODONGO, &RegisterAC, &ClearACHits, nullptr, nullptr }));
