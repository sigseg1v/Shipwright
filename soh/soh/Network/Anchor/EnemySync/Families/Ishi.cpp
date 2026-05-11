#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Ishi/z_en_ishi.h"
}

// Throwable rock. Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnIshi*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnIshi*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_ISHI, &RegisterAC, &ClearACHits, nullptr, nullptr }));
