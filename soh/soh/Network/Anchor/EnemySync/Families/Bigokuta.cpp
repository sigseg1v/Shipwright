#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Bigokuta/z_en_bigokuta.h"

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

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnBigokuta*>(actor);
    // En_Bigokuta reads collider.elements[0].info.acHitInfo->toucher.dmgFlags
    // when AC_HIT is observed; seed acHitInfo too.
    EnemySyncHelpers::SetJntSphAcHit(&a->collider);
    for (int i = 0; i < 2; i++) {
        EnemySyncHelpers::SetCylAcHit(&a->cylinder[i]);
    }
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_BIGOKUTA, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
