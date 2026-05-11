#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Ik/z_en_ik.h"

// Iron Knuckle. Body ColliderCylinder, axe ColliderQuad, shield
// ColliderTris.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnIk*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->bodyCollider);
    EnemySyncHelpers::RegisterColliderCommon(&a->axeCollider.base);
    EnemySyncHelpers::RegisterColliderCommon(&a->shieldCollider.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnIk*>(actor);
    a->bodyCollider.base.acFlags &= ~AC_HIT;
    a->axeCollider.base.acFlags &= ~AC_HIT;
    a->shieldCollider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnIk*>(actor);
    a->bodyCollider.base.acFlags |= AC_HIT;
    a->axeCollider.base.acFlags |= AC_HIT;
    a->shieldCollider.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_IK, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
