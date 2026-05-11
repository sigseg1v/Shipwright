#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Dha/z_en_dha.h"

// Dead Hand's hand (the disembodied grabbers). Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnDha*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDha*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDha*>(actor);
    a->collider.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_DHA, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
