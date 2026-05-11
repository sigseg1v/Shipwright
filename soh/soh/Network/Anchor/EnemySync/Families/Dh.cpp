#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Dh/z_en_dh.h"

// Dead Hand. ColliderCylinder (body) + ColliderJntSph (head/hand).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnDh*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider1);
    EnemySyncHelpers::RegisterJntSph(&a->collider2);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDh*>(actor);
    a->collider1.base.acFlags &= ~AC_HIT;
    a->collider2.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDh*>(actor);
    a->collider1.base.acFlags |= AC_HIT;
    a->collider2.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_DH, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
