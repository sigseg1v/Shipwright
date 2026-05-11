#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

// z_en_floormas.h declares an action-func typedef using `this` as the
// parameter name, which is a reserved word in C++. Locally rename it
// during include only -- the redefinition is identifier-name only, so
// the struct layout and any `this` callers in .c files are unaffected.
#define this thisx
#include "src/overlays/actors/ovl_En_Floormas/z_en_floormas.h"
#undef this

// Floormaster. Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnFloormas*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFloormas*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFloormas*>(actor);
    a->collider.base.acFlags |= AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_FLOORMAS, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
