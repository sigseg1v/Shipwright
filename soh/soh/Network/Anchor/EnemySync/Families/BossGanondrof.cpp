#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#define this thisx
#include "src/overlays/actors/ovl_Boss_Ganondrof/z_boss_ganondrof.h"
#undef this
}

// Phantom Ganon (rider). Two ColliderCylinders: colliderBody, colliderSpear.
// The horse is a separate paired actor without its own collider fields.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossGanondrof*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderBody);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderSpear);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGanondrof*>(actor);
    a->colliderBody.base.acFlags &= ~AC_HIT;
    a->colliderSpear.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_GANONDROF, &RegisterAC, &ClearACHits, nullptr, nullptr }));
