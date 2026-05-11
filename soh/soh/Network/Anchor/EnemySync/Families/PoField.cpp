#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Po_Field/z_en_po_field.h"

// Field poe. Body ColliderCylinder plus a flame ColliderCylinder.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnPoField*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
    EnemySyncHelpers::RegisterCyl(actor, &a->flameCollider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPoField*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
    a->flameCollider.base.acFlags &= ~AC_HIT;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_PO_FIELD, &RegisterAC, &ClearACHits, nullptr, nullptr }));
