#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Ik/z_en_ik.h"

// Iron Knuckle. Body ColliderCylinder, axe ColliderQuad, shield
// ColliderTris.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnIk*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->bodyCollider);
    EnemySyncHelpers::RegisterColliderCommon(&a->axeCollider.base);
    EnemySyncHelpers::RegisterColliderCommon(&a->shieldCollider.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnIk*>(actor);
    a->bodyCollider.base.acFlags &= ~AC_HIT;
    a->axeCollider.base.acFlags &= ~AC_HIT;
    a->shieldCollider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnIk*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->bodyCollider);
    a->axeCollider.base.acFlags |= AC_HIT;
    a->shieldCollider.base.acFlags |= AC_HIT;
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnIk*>(actor);
    // The visible armor-break state on the Iron Knuckle is driven by
    // drawArmorFlag (which limb-draw sets render the armor pieces) and
    // armorStatusFlag (which limbs have been broken off and stay off).
    // drawMode + action drive the draw fn's branching. The action funcs
    // themselves are decomp-named `func_80A7...` and changing them on
    // the peer is hairy, so we sync the visible-state bytes and let the
    // peer's actor stay in its initial action until we wire those up.
    payload["ikDrawArmor"] = a->drawArmorFlag;
    payload["ikArmorStat"] = a->armorStatusFlag;
    payload["ikBreakProp"] = a->isBreakingProp;
    payload["ikDamageRxn"] = a->damageReaction;
    payload["ikAnimTimer"] = a->animationTimer;
    payload["ikAction"] = a->action;
    payload["ikDrawMode"] = a->drawMode;
    payload["ikAxeSummoned"] = a->isAxeSummoned;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnIk*>(actor);
    a->drawArmorFlag = payload.value("ikDrawArmor", (u8)0);
    a->armorStatusFlag = payload.value("ikArmorStat", (u8)0);
    a->isBreakingProp = payload.value("ikBreakProp", (u8)0);
    a->damageReaction = payload.value("ikDamageRxn", (u8)0);
    a->animationTimer = payload.value("ikAnimTimer", (u8)0);
    a->action = payload.value("ikAction", (s32)0);
    a->drawMode = payload.value("ikDrawMode", (s32)0);
    a->isAxeSummoned = payload.value("ikAxeSummoned", (s32)0);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_IK, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
