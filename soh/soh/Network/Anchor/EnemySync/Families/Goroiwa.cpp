#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Goroiwa/z_en_goroiwa.h"

// Rolling boulder. Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnGoroiwa*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnGoroiwa*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnGoroiwa*>(actor);
    a->collider.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_GOROIWA, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
