#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_GeldB/z_en_geldb.h"
}

// Gerudo (white-clad fighter). Body ColliderCylinder, sword ColliderQuad,
// shield-block ColliderTris.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnGeldB*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->bodyCollider);
    EnemySyncHelpers::RegisterColliderCommon(&a->swordCollider.base);
    EnemySyncHelpers::RegisterColliderCommon(&a->blockCollider.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnGeldB*>(actor);
    a->bodyCollider.base.acFlags &= ~AC_HIT;
    a->swordCollider.base.acFlags &= ~AC_HIT;
    a->blockCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_GELDB, &RegisterAC, &ClearACHits, nullptr, nullptr }));
