#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Zf/z_en_zf.h"

// Lizalfos / Dinolfos. Body ColliderCylinder + sword ColliderQuad.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnZf*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->bodyCollider);
    EnemySyncHelpers::RegisterColliderCommon(&a->swordCollider.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnZf*>(actor);
    a->bodyCollider.base.acFlags &= ~AC_HIT;
    a->swordCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_ZF, &RegisterAC, &ClearACHits, nullptr, nullptr }));
