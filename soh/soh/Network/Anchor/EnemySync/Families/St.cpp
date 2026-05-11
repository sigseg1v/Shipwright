#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

// z_en_st.h declares an action-func typedef using `this` as the parameter
// name, which is a reserved word in C++. Locally rename it during include
// only -- the redefinition is identifier-name only, so the struct layout
// and any `this` callers in .c files are unaffected.
#define this thisx
#include "src/overlays/actors/ovl_En_St/z_en_st.h"
#undef this

// Skulltula (the large web-hanging variety). One JntSph (body) plus six
// ColliderCylinders (legs).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnSt*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->colSph);
    for (int i = 0; i < 6; i++) {
        EnemySyncHelpers::RegisterCyl(actor, &a->colCylinder[i]);
    }
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnSt*>(actor);
    a->colSph.base.acFlags &= ~AC_HIT;
    for (int i = 0; i < 6; i++) {
        a->colCylinder[i].base.acFlags &= ~AC_HIT;
    }
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnSt*>(actor);
    a->colSph.base.acFlags |= AC_HIT;
    for (int i = 0; i < 6; i++) {
        a->colCylinder[i].base.acFlags |= AC_HIT;
    }
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_ST, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
