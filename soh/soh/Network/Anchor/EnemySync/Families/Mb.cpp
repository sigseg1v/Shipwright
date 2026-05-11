#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Mb/z_en_mb.h"

// Moblin (spear and club variants). Body ColliderCylinder, attack
// ColliderQuad, plus a ColliderTris standing in for a front shield.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnMb*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->hitbox);
    EnemySyncHelpers::RegisterColliderCommon(&a->attackCollider.base);
    EnemySyncHelpers::RegisterColliderCommon(&a->frontShielding.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnMb*>(actor);
    a->hitbox.base.acFlags &= ~AC_HIT;
    a->attackCollider.base.acFlags &= ~AC_HIT;
    a->frontShielding.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnMb*>(actor);
    a->hitbox.base.acFlags |= AC_HIT;
    a->attackCollider.base.acFlags |= AC_HIT;
    a->frontShielding.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_MB, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
