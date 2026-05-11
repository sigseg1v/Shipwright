#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Bigokuta/z_en_bigokuta.h"
}

// Big Octo. One ColliderJntSph (body) plus two ColliderCylinders (sub hitboxes).

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnBigokuta*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
    for (int i = 0; i < 2; i++) {
        EnemySyncHelpers::RegisterCyl(actor, &a->cylinder[i]);
    }
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnBigokuta*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
    for (int i = 0; i < 2; i++) {
        a->cylinder[i].base.acFlags &= ~AC_HIT;
    }
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_BIGOKUTA, &RegisterAC, &ClearACHits, nullptr, nullptr }));
