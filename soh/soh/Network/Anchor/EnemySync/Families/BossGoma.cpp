#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#undef this

// Queen Gohma. Single ColliderJntSph (body/eye).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossGoma*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGoma*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_GOMA, &RegisterAC, &ClearACHits, nullptr, nullptr }));
