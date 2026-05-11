#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Tw/z_boss_tw.h"
#undef this

// Twinrova (Koume / Kotake, also merged form). Single ColliderCylinder.
// Each sister actor instance owns its own collider; the paired sub-actor
// case does not apply here as both Koume and Kotake have a collider field.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossTw*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossTw*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossTw*>(actor);
    a->collider.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_TW, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
