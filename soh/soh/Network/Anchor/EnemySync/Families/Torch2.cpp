#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

// Dark Link. Reuses the Player struct (sizeof(Player) actor), so the
// colliders live on Player: body ColliderCylinder, two melee
// ColliderQuads, one shield ColliderQuad. Player is already pulled in
// transitively via z64.h.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<Player*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->cylinder);
    EnemySyncHelpers::RegisterColliderCommon(&a->meleeWeaponQuads[0].base);
    EnemySyncHelpers::RegisterColliderCommon(&a->meleeWeaponQuads[1].base);
    EnemySyncHelpers::RegisterColliderCommon(&a->shieldQuad.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<Player*>(actor);
    a->cylinder.base.acFlags &= ~AC_HIT;
    a->meleeWeaponQuads[0].base.acFlags &= ~AC_HIT;
    a->meleeWeaponQuads[1].base.acFlags &= ~AC_HIT;
    a->shieldQuad.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_TORCH2, &RegisterAC, &ClearACHits, nullptr, nullptr }));
