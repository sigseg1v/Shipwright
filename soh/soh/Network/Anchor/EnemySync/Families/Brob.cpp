#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

// z_en_brob.h declares an action-func typedef using `this` as the parameter
// name, which is a reserved word in C++. Locally rename it during include.
#define this thisx
#include "src/overlays/actors/ovl_En_Brob/z_en_brob.h"
#undef this

// Brob (Jabu-Jabu spinning shock platform). Two ColliderCylinders.
// Stored in `colliders[2]`; underlying actor is a DynaPolyActor but the
// Actor header sits at offset 0 so the reinterpret_cast is still valid.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnBrob*>(actor);
    for (int i = 0; i < 2; i++) {
        EnemySyncHelpers::RegisterCyl(actor, &a->colliders[i]);
    }
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnBrob*>(actor);
    for (int i = 0; i < 2; i++) {
        a->colliders[i].base.acFlags &= ~AC_HIT;
    }
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnBrob*>(actor);
    for (int i = 0; i < 2; i++) {
        a->colliders[i].base.acFlags |= AC_HIT;
    }
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_BROB, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
