#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Bw/z_en_bw.h"

// Torch Slug. Two ColliderCylinders (body + flame aura).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnBw*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider1);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider2);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnBw*>(actor);
    a->collider1.base.acFlags &= ~AC_HIT;
    a->collider2.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_BW, &RegisterAC, &ClearACHits, nullptr, nullptr }));
