#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Heishi3/z_en_heishi3.h"
}

// Gerudo Fortress guard. Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnHeishi3*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnHeishi3*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_HEISHI3, &RegisterAC, &ClearACHits, nullptr, nullptr }));
