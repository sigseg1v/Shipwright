#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Po_Relay/z_en_po_relay.h"
}

// Dampe ghost. Single ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnPoRelay*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPoRelay*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_PO_RELAY, &RegisterAC, &ClearACHits, nullptr, nullptr }));
