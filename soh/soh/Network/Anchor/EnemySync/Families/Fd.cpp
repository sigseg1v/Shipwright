#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_En_Fd/z_en_fd.h"
#undef this

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnFd*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFd*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_FD, &RegisterAC, &ClearACHits, nullptr, nullptr }));
