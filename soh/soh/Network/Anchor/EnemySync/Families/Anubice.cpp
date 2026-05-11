#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Anubice/z_en_anubice.h"

// Anubis. Single ColliderCylinder. Note the actual En_Anubice actor is
// the body; the separate ovl_En_AnubiceTag spawner is not synced here.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnAnubice*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnAnubice*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_ANUBICE, &RegisterAC, &ClearACHits, nullptr, nullptr }));
