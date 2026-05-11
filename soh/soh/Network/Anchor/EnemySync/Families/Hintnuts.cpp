#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Hintnuts/z_en_hintnuts.h"

// Puzzle/hint Deku Scrub (the locked-room reflect-the-deku-nut variant).
// Same shape as EnDekunuts: actionFunc-driven AI, single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnHintnuts*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnHintnuts*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_HINTNUTS, &RegisterAC, &ClearACHits, nullptr, nullptr }));
