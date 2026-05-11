#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"
}

// Big/Withered Deku Baba. Two ColliderCylinders (head + body stem).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnKarebaba*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->headCollider);
    EnemySyncHelpers::RegisterCyl(actor, &a->bodyCollider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnKarebaba*>(actor);
    a->headCollider.base.acFlags &= ~AC_HIT;
    a->bodyCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_KAREBABA, &RegisterAC, &ClearACHits, nullptr, nullptr }));
