#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Dha/z_en_dha.h"
#include "objects/object_dh/object_dh.h"

// Dead Hand's hand (the disembodied grabbers). Single ColliderJntSph.
//
// The hand reaches with limb angles + arm/hand positions that are all
// computed in EnDha_Wait. Without sync the peer sees a hand stuck in
// its initial pose. unk_1C0 doubles as a state byte (>= 8 means dying).

extern "C" {
    void EnDha_Wait(EnDha*, PlayState*);
    void EnDha_TakeDamage(EnDha*, PlayState*);
    void EnDha_Die(EnDha*, PlayState*);
}

namespace {

enum DhaAction : u8 {
    DHA_WAIT = 0,
    DHA_TAKE_DAMAGE = 1,
    DHA_DIE = 2,
    DHA_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(EnDhaActionFunc f) {
    if (f == EnDha_Wait) return DHA_WAIT;
    if (f == EnDha_TakeDamage) return DHA_TAKE_DAMAGE;
    if (f == EnDha_Die) return DHA_DIE;
    return DHA_UNKNOWN;
}

EnDhaActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case DHA_WAIT: return EnDha_Wait;
        case DHA_TAKE_DAMAGE: return EnDha_TakeDamage;
        case DHA_DIE: return EnDha_Die;
        default: return nullptr;
    }
}

void PlayAnimationFor(EnDha* a, u8 actionId) {
    if (actionId == DHA_WAIT) {
        Animation_PlayLoop(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_0015B0);
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnDha*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDha*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDha*>(actor);
    EnemySyncHelpers::SetJntSphAcHit(&a->collider);
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnDha*>(actor);
    payload["dhaAction"] = ActionFuncToId(a->actionFunc);
    payload["dhaUnk1C0"] = a->unk_1C0;
    payload["dhaUnk1CC"] = a->unk_1CC;
    payload["dhaActionTimer"] = a->actionTimer;
    payload["dhaTimer"] = a->timer;
    payload["dhaLimbX0"] = a->limbAngleX[0];
    payload["dhaLimbX1"] = a->limbAngleX[1];
    payload["dhaLimbY"] = a->limbAngleY;
    payload["dhaHandAngleX"] = a->handAngle.x;
    payload["dhaHandAngleY"] = a->handAngle.y;
    payload["dhaHandAngleZ"] = a->handAngle.z;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnDha*>(actor);

    u8 wireId = payload.value("dhaAction", (u8)DHA_UNKNOWN);
    EnDhaActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionFunc != desired) {
        PlayAnimationFor(a, wireId);
        a->actionFunc = desired;
    }

    a->unk_1C0 = payload.value("dhaUnk1C0", (u8)0);
    a->unk_1CC = payload.value("dhaUnk1CC", (u8)0);
    a->actionTimer = payload.value("dhaActionTimer", (s16)0);
    a->timer = payload.value("dhaTimer", (s16)0);
    a->limbAngleX[0] = payload.value("dhaLimbX0", (s16)0);
    a->limbAngleX[1] = payload.value("dhaLimbX1", (s16)0);
    a->limbAngleY = payload.value("dhaLimbY", (s16)0);
    a->handAngle.x = payload.value("dhaHandAngleX", (s16)0);
    a->handAngle.y = payload.value("dhaHandAngleY", (s16)0);
    a->handAngle.z = payload.value("dhaHandAngleZ", (s16)0);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_DHA, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
