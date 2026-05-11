#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#define this thisx
#include "src/overlays/actors/ovl_Boss_Va/z_boss_va.h"
#undef this
}

// Barinade. ColliderCylinder colliderBody, ColliderJntSph colliderSph,
// ColliderQuad colliderLightning.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossVa*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderBody);
    EnemySyncHelpers::RegisterJntSph(&a->colliderSph);
    EnemySyncHelpers::RegisterColliderCommon(&a->colliderLightning.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossVa*>(actor);
    a->colliderBody.base.acFlags &= ~AC_HIT;
    a->colliderSph.base.acFlags &= ~AC_HIT;
    a->colliderLightning.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_VA, &RegisterAC, &ClearACHits, nullptr, nullptr }));
