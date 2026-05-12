#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Fd2/z_boss_fd2.h"
#undef this

// Volvagia (hole phase). Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossFd2*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossFd2*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossFd2*>(actor);
    // Boss_Fd2 reads collider.elements[0].info.acHitInfo when AC_HIT is
    // set; seed acHitInfo too.
    EnemySyncHelpers::SetJntSphAcHit(&a->collider);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_FD2, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
