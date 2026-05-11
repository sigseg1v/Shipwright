#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Skj/z_en_skj.h"

// Skull Kid. Single ColliderCylinder (used for both the friendly forest
// variant and the hostile needle-shooting variants).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnSkj*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnSkj*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnSkj*>(actor);
    a->collider.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_SKJ, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
