#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Vali/z_en_vali.h"

// Bari (large jellyfish). Body ColliderCylinder plus left/right arm
// ColliderQuads.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnVali*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->bodyCollider);
    EnemySyncHelpers::RegisterColliderCommon(&a->leftArmCollider.base);
    EnemySyncHelpers::RegisterColliderCommon(&a->rightArmCollider.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnVali*>(actor);
    a->bodyCollider.base.acFlags &= ~AC_HIT;
    a->leftArmCollider.base.acFlags &= ~AC_HIT;
    a->rightArmCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_VALI, &RegisterAC, &ClearACHits, nullptr, nullptr }));
