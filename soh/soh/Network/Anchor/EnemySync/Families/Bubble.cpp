#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Bubble/z_en_bubble.h"
}

// Shabom (the bouncing soap-bubble enemy). Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnBubble*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->colliderSphere);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnBubble*>(actor);
    a->colliderSphere.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_BUBBLE, &RegisterAC, &ClearACHits, nullptr, nullptr }));
