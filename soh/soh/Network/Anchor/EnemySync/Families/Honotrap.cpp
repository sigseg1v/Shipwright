#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Honotrap/z_en_honotrap.h"

// Flame trap. Collider is a union: HONOTRAP_EYE uses ColliderTris,
// HONOTRAP_FLAME_MOVE / HONOTRAP_FLAME_DROP use ColliderCylinder.
// Dispatch on actor->params so we touch the live variant only.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnHonotrap*>(actor);
    if (actor->params == HONOTRAP_EYE) {
        EnemySyncHelpers::RegisterColliderCommon(&a->collider.tris.base);
    } else {
        EnemySyncHelpers::RegisterCyl(actor, &a->collider.cyl);
    }
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnHonotrap*>(actor);
    if (actor->params == HONOTRAP_EYE) {
        a->collider.tris.base.acFlags &= ~AC_HIT;
    } else {
        a->collider.cyl.base.acFlags &= ~AC_HIT;
    }
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnHonotrap*>(actor);
    if (actor->params == HONOTRAP_EYE) {
        a->collider.tris.base.acFlags |= AC_HIT;
    } else {
        a->collider.cyl.base.acFlags |= AC_HIT;
    }
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_HONOTRAP, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
