#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"

// Deku Baba (the small biting plant that drops a stick). JntSph
// collider on the head. AI state not yet plumbed; v1 syncs pos/rot/hp
// only and accepts a small animation-phase mismatch on non-authority.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnDekubaba*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDekubaba*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_DEKUBABA, &RegisterAC, &ClearACHits, nullptr, nullptr }));
