#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Poh/z_en_poh.h"
}

// Poe. One ColliderCylinder (lantern/body) plus one ColliderJntSph
// (soul).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnPoh*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderCyl);
    EnemySyncHelpers::RegisterJntSph(&a->colliderSph);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPoh*>(actor);
    a->colliderCyl.base.acFlags &= ~AC_HIT;
    a->colliderSph.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_POH, &RegisterAC, &ClearACHits, nullptr, nullptr }));
