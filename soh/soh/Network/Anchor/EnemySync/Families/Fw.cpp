#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

// z_en_fw.h declares an action-func typedef using `this` as the parameter
// name, which is a reserved word in C++. Locally rename it during include.
#define this thisx
#include "src/overlays/actors/ovl_En_Fw/z_en_fw.h"
#undef this

// Flare Dancer. Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnFw*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFw*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_FW, &RegisterAC, &ClearACHits, nullptr, nullptr }));
