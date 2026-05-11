#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Fz/z_en_fz.h"

// Freezard. Three stacked ColliderCylinders.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnFz*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider1);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider2);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider3);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFz*>(actor);
    a->collider1.base.acFlags &= ~AC_HIT;
    a->collider2.base.acFlags &= ~AC_HIT;
    a->collider3.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFz*>(actor);
    a->collider1.base.acFlags |= AC_HIT;
    a->collider2.base.acFlags |= AC_HIT;
    a->collider3.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_FZ, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
