#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Am/z_en_am.h"

// Armos. Two ColliderCylinders (hurt + block) plus one ColliderQuad
// (hit). Quad has no per-frame position helper; register its base
// directly via RegisterColliderCommon.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnAm*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->hurtCollider);
    EnemySyncHelpers::RegisterCyl(actor, &a->blockCollider);
    EnemySyncHelpers::RegisterColliderCommon(&a->hitCollider.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnAm*>(actor);
    a->hurtCollider.base.acFlags &= ~AC_HIT;
    a->blockCollider.base.acFlags &= ~AC_HIT;
    a->hitCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_AM, &RegisterAC, &ClearACHits, nullptr, nullptr }));
