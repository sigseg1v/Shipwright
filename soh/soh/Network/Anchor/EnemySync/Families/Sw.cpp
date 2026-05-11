#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Sw/z_en_sw.h"

// Skullwalltula (the small wall-crawling spider). Single JntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnSw*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnSw*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_SW, &RegisterAC, &ClearACHits, nullptr, nullptr }));
