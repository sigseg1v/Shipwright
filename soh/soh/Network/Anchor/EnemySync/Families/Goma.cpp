#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Goma/z_en_goma.h"

// Gohma Larva (the small spider-like minions, both inside the boss
// fight and elsewhere). Two ColliderCylinders.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnGoma*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colCyl1);
    EnemySyncHelpers::RegisterCyl(actor, &a->colCyl2);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnGoma*>(actor);
    a->colCyl1.base.acFlags &= ~AC_HIT;
    a->colCyl2.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnGoma*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->colCyl1);
    EnemySyncHelpers::SetCylAcHit(&a->colCyl2);
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnGoma*>(actor);
    // Larva and egg variants. gomaType (ENGOMA_NORMAL / EGG /
    // HATCH_DEBRIS / BOSSLIMB) is set in Init and never reassigned, so
    // it doesn't need wire sync. hatchState + eggScale + eggSquishAngle
    // drive the visible egg jitter and burst. visualState is read by
    // the draw fn to pick which limbs are rendered. The actionFunc
    // pointer itself is decomp-named and not synced; the per-state
    // visual fields cover the user-visible drift.
    payload["gomaHatch"] = a->hatchState;
    payload["gomaVisual"] = a->visualState;
    payload["gomaEggScale"] = a->eggScale;
    payload["gomaEggPitch"] = a->eggPitch;
    payload["gomaEggSqAng"] = a->eggSquishAngle;
    payload["gomaEggSqAmt"] = a->eggSquishAmount;
    payload["gomaEggYOff"] = a->eggYOffset;
    payload["gomaHurt"] = a->hurtTimer;
    payload["gomaStun"] = a->stunTimer;
    payload["gomaInv"] = a->invincibilityTimer;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnGoma*>(actor);
    a->hatchState = payload.value("gomaHatch", (s16)0);
    a->visualState = payload.value("gomaVisual", (s16)0);
    a->eggScale = payload.value("gomaEggScale", a->eggScale);
    a->eggPitch = payload.value("gomaEggPitch", 0.0f);
    a->eggSquishAngle = payload.value("gomaEggSqAng", 0.0f);
    a->eggSquishAmount = payload.value("gomaEggSqAmt", 0.0f);
    a->eggYOffset = payload.value("gomaEggYOff", 0.0f);
    a->hurtTimer = payload.value("gomaHurt", (s16)0);
    a->stunTimer = payload.value("gomaStun", (s16)0);
    a->invincibilityTimer = payload.value("gomaInv", (s16)0);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_GOMA, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
