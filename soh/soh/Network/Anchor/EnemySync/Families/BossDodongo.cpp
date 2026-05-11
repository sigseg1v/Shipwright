#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Dodongo/z_boss_dodongo.h"
#undef this

// King Dodongo. Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossDodongo*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossDodongo*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossDodongo*>(actor);
    a->collider.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_DODONGO, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
