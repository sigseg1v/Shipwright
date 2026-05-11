#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Peehat/z_en_peehat.h"
}

// Peahat. One ColliderCylinder (body) plus one ColliderJntSph (blade
// hub) plus one ColliderQuad (spinning blade). Quad is registered via
// RegisterColliderCommon since it has no per-frame position helper.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnPeehat*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colCylinder);
    EnemySyncHelpers::RegisterJntSph(&a->colJntSph);
    EnemySyncHelpers::RegisterColliderCommon(&a->colQuad.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPeehat*>(actor);
    a->colCylinder.base.acFlags &= ~AC_HIT;
    a->colJntSph.base.acFlags &= ~AC_HIT;
    a->colQuad.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_PEEHAT, &RegisterAC, &ClearACHits, nullptr, nullptr }));
