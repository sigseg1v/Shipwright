#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

// z_en_niw.h declares helper functions using `this` as the parameter
// name. The header has its own __cplusplus guard but we still wrap to be
// safe.
extern "C" {
#define this thisx
#include "src/overlays/actors/ovl_En_Niw/z_en_niw.h"
#undef this
}

// Cucco. Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnNiw*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnNiw*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_NIW, &RegisterAC, &ClearACHits, nullptr, nullptr }));
