#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Dekunuts/z_en_dekunuts.h"
}

// Mad Scrub. Single ColliderCylinder; AI driven by an actionFunc
// pointer rather than an enum, so per-family AI sync would need a
// separate dispatch table to map function pointers to wire IDs. v1
// syncs pos/rot/hp only.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnDekunuts*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDekunuts*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_DEKUNUTS, &RegisterAC, &ClearACHits, nullptr, nullptr }));
