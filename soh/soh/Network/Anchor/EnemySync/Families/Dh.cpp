#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Dh/z_en_dh.h"
#include "objects/object_dh/object_dh.h"

// Dead Hand. ColliderCylinder (body) + ColliderJntSph (head/hand).
//
// Without AI sync the peer sees a Dead Hand frozen at its initial
// underground pose: SetupWait drops actor.shape.yOffset to -15000 and
// the body rises only when EnDh_Wait advances actionState. The dirt
// wave plume and the surfacing animation are also Update-driven, so we
// sync the action func, the visible timers, dirt wave fields, yOffset,
// alpha, and params (the params field doubles as a state byte after
// Init: ENDH_START_ATTACK_*, ENDH_HANDS_KILLED_*, ENDH_DEATH).

extern "C" {
    void EnDh_Wait(EnDh*, PlayState*);
    void EnDh_Walk(EnDh*, PlayState*);
    void EnDh_Retreat(EnDh*, PlayState*);
    void EnDh_Attack(EnDh*, PlayState*);
    void EnDh_Burrow(EnDh*, PlayState*);
    void EnDh_Damage(EnDh*, PlayState*);
    void EnDh_Death(EnDh*, PlayState*);
}

namespace {

enum DhAction : u8 {
    DH_WAIT = 0,
    DH_WALK = 1,
    DH_RETREAT = 2,
    DH_ATTACK = 3,
    DH_BURROW = 4,
    DH_DAMAGE = 5,
    DH_DEATH = 6,
    DH_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(EnDhActionFunc f) {
    if (f == EnDh_Wait) return DH_WAIT;
    if (f == EnDh_Walk) return DH_WALK;
    if (f == EnDh_Retreat) return DH_RETREAT;
    if (f == EnDh_Attack) return DH_ATTACK;
    if (f == EnDh_Burrow) return DH_BURROW;
    if (f == EnDh_Damage) return DH_DAMAGE;
    if (f == EnDh_Death) return DH_DEATH;
    return DH_UNKNOWN;
}

EnDhActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case DH_WAIT: return EnDh_Wait;
        case DH_WALK: return EnDh_Walk;
        case DH_RETREAT: return EnDh_Retreat;
        case DH_ATTACK: return EnDh_Attack;
        case DH_BURROW: return EnDh_Burrow;
        case DH_DAMAGE: return EnDh_Damage;
        case DH_DEATH: return EnDh_Death;
        default: return nullptr;
    }
}

void PlayAnimationFor(EnDh* a, u8 actionId) {
    switch (actionId) {
        case DH_WAIT:
            Animation_PlayLoop(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_003A8C);
            break;
        case DH_WALK:
            Animation_Change(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_003A8C, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)&object_dh_Anim_003A8C) - 3.0f,
                             ANIMMODE_LOOP, -6.0f);
            break;
        case DH_RETREAT:
            Animation_MorphToLoop(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_005880, -4.0f);
            break;
        case DH_ATTACK:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_004658, -6.0f);
            break;
        case DH_BURROW:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_002148, -6.0f);
            break;
        case DH_DAMAGE:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_003D6C, -6.0f);
            break;
        case DH_DEATH:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)&object_dh_Anim_0032BC, -1.0f);
            break;
        default:
            break;
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnDh*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider1);
    EnemySyncHelpers::RegisterJntSph(&a->collider2);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDh*>(actor);
    a->collider1.base.acFlags &= ~AC_HIT;
    a->collider2.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnDh*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->collider1);
    EnemySyncHelpers::SetJntSphAcHit(&a->collider2);
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnDh*>(actor);
    payload["dhAction"] = ActionFuncToId(a->actionFunc);
    payload["dhActionState"] = a->actionState;
    payload["dhCurAction"] = a->curAction;
    payload["dhRetreat"] = a->retreat;
    payload["dhAlpha"] = a->alpha;
    payload["dhDrawDirt"] = a->drawDirtWave;
    payload["dhDirtPhase"] = a->dirtWavePhase;
    payload["dhTimer"] = a->timer;
    payload["dhDirtSpread"] = a->dirtWaveSpread;
    payload["dhDirtHeight"] = a->dirtWaveHeight;
    payload["dhDirtAlpha"] = a->dirtWaveAlpha;
    payload["dhYOffset"] = a->actor.shape.yOffset;
    payload["dhParams"] = (s16)a->actor.params;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnDh*>(actor);

    u8 wireId = payload.value("dhAction", (u8)DH_UNKNOWN);
    EnDhActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionFunc != desired) {
        PlayAnimationFor(a, wireId);
        a->actionFunc = desired;
    }

    a->actionState = payload.value("dhActionState", (u8)0);
    a->curAction = payload.value("dhCurAction", a->curAction);
    a->retreat = payload.value("dhRetreat", (u8)0);
    a->alpha = payload.value("dhAlpha", (u8)255);
    a->drawDirtWave = payload.value("dhDrawDirt", (u8)0);
    a->dirtWavePhase = payload.value("dhDirtPhase", (s16)0);
    a->timer = payload.value("dhTimer", (s16)0);
    a->dirtWaveSpread = payload.value("dhDirtSpread", 0.0f);
    a->dirtWaveHeight = payload.value("dhDirtHeight", 0.0f);
    a->dirtWaveAlpha = payload.value("dhDirtAlpha", 0.0f);
    a->actor.shape.yOffset = payload.value("dhYOffset", a->actor.shape.yOffset);
    a->actor.params = payload.value("dhParams", (s16)a->actor.params);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_DH, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
