#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Bx/z_en_bx.h"
}

// Tailpasaran (the worm/stinger swarm). ColliderCylinder body plus a
// ColliderQuad tail attack.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnBx*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
    EnemySyncHelpers::RegisterColliderCommon(&a->colliderQuad.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnBx*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
    a->colliderQuad.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_BX, &RegisterAC, &ClearACHits, nullptr, nullptr }));
