#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Fd/z_boss_fd.h"
#undef this

// Volvagia (flying phase). Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossFd*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossFd*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossFd*>(actor);
    // Boss_Fd reads collider.elements[head].info.acHitInfo when AC_HIT
    // is set; seed acHitInfo too.
    EnemySyncHelpers::SetJntSphAcHit(&a->collider);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_FD, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
