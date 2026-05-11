#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Vm/z_en_vm.h"

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnVm*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colliderCylinder);
    EnemySyncHelpers::RegisterColliderCommon(&a->colliderQuad1.base);
    EnemySyncHelpers::RegisterColliderCommon(&a->colliderQuad2.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnVm*>(actor);
    a->colliderCylinder.base.acFlags &= ~AC_HIT;
    a->colliderQuad1.base.acFlags &= ~AC_HIT;
    a->colliderQuad2.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_VM, &RegisterAC, &ClearACHits, nullptr, nullptr }));
