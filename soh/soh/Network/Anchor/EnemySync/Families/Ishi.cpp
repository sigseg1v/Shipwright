#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Ishi/z_en_ishi.h"

// Throwable rock. Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnIshi*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnIshi*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnIshi*>(actor);
    // En_Ishi reads collider.info.acHitInfo->toucher.dmgFlags when AC_HIT
    // is set; seed acHitInfo too.
    EnemySyncHelpers::SetCylAcHit(&a->collider);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_ISHI, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
