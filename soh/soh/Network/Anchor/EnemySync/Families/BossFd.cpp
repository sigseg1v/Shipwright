#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#define this thisx
#include "src/overlays/actors/ovl_Boss_Fd/z_boss_fd.h"
#undef this
}

// Volvagia (flying phase). Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossFd*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossFd*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_FD, &RegisterAC, &ClearACHits, nullptr, nullptr }));
