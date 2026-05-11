#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Test/z_en_test.h"
}

// Stalfos. Body and shield ColliderCylinders, sword ColliderQuad.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnTest*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->bodyCollider);
    EnemySyncHelpers::RegisterCyl(actor, &a->shieldCollider);
    EnemySyncHelpers::RegisterColliderCommon(&a->swordCollider.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnTest*>(actor);
    a->bodyCollider.base.acFlags &= ~AC_HIT;
    a->shieldCollider.base.acFlags &= ~AC_HIT;
    a->swordCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_TEST, &RegisterAC, &ClearACHits, nullptr, nullptr }));
