#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Wf/z_en_wf.h"

// Wolfos. One ColliderJntSph (body spheres) plus two ColliderCylinders
// (body and tail).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnWf*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->colliderSpheres);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderCylinderBody);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderCylinderTail);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnWf*>(actor);
    a->colliderSpheres.base.acFlags &= ~AC_HIT;
    a->colliderCylinderBody.base.acFlags &= ~AC_HIT;
    a->colliderCylinderTail.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_WF, &RegisterAC, &ClearACHits, nullptr, nullptr }));
