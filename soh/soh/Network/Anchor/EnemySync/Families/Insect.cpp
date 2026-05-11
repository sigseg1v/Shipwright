#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Insect/z_en_insect.h"
}

// Insect (bug). Single ColliderJntSph.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnInsect*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnInsect*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_INSECT, &RegisterAC, &ClearACHits, nullptr, nullptr }));
