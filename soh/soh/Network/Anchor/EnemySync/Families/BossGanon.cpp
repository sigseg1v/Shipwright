#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Ganon/z_boss_ganon.h"
#undef this

// Ganondorf (tower fight). Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossGanon*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGanon*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGanon*>(actor);
    // Boss_Ganon reads collider.info.acHitInfo->toucher.dmgFlags when
    // AC_HIT is set; seed acHitInfo too.
    EnemySyncHelpers::SetCylAcHit(&a->collider);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_GANON, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
