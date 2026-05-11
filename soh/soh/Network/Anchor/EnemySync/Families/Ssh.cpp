#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Ssh/z_en_ssh.h"

// Big Skulltula (the large web-hanging variety). One ColliderJntSph
// body plus six ColliderCylinders for the legs. Mirrors En_St.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnSsh*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->colSph);
    for (int i = 0; i < 6; i++) {
        EnemySyncHelpers::RegisterCyl(actor, &a->colCylinder[i]);
    }
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnSsh*>(actor);
    a->colSph.base.acFlags &= ~AC_HIT;
    for (int i = 0; i < 6; i++) {
        a->colCylinder[i].base.acFlags &= ~AC_HIT;
    }
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_SSH, &RegisterAC, &ClearACHits, nullptr, nullptr }));
