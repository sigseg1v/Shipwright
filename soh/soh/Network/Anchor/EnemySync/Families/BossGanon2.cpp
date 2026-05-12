#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.h"
#undef this

// Ganon (final form, pig). Two ColliderJntSph fields (unk_424 and
// unk_444) for body and tail respectively.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossGanon2*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->unk_424);
    EnemySyncHelpers::RegisterJntSph(&a->unk_444);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGanon2*>(actor);
    a->unk_424.base.acFlags &= ~AC_HIT;
    a->unk_444.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGanon2*>(actor);
    // Boss_Ganon2 reads element[0].info.acHitInfo when AC_HIT is set;
    // seed acHitInfo for both spheres.
    EnemySyncHelpers::SetJntSphAcHit(&a->unk_424);
    EnemySyncHelpers::SetJntSphAcHit(&a->unk_444);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_GANON2, &RegisterAC, &ClearACHits, nullptr, nullptr, &SetACHits }));
