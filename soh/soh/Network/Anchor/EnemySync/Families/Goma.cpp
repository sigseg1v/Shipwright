#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Goma/z_en_goma.h"

// Gohma Larva (the small spider-like minions, both inside the boss
// fight and elsewhere). Two ColliderCylinders.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnGoma*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colCyl1);
    EnemySyncHelpers::RegisterCyl(actor, &a->colCyl2);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnGoma*>(actor);
    a->colCyl1.base.acFlags &= ~AC_HIT;
    a->colCyl2.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_GOMA, &RegisterAC, &ClearACHits, nullptr, nullptr }));
