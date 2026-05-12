#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#define this thisx
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#undef this
#include "objects/object_goma/object_goma.h"

// Queen Gohma. Single ColliderJntSph.
//
// Boss_Goma's update is suppressed on the peer, so without AI sync the
// peer sees the boss frozen in its initial ceiling pose: no walk
// cycle, no descent, no defeat decay. Sync the action func + every
// visible field that the various action funcs mutate: animation
// (replayed via PlayAnimationFor), eye lids/iris angles, decay
// progress, invincibility timers, env colors. Cutscene-only state
// (sub-camera, disableGameplayLogic) stays authority-only.

extern "C" {
    void BossGoma_SetupEncounter(BossGoma*, PlayState*);
    void BossGoma_Encounter(BossGoma*, PlayState*);
    void BossGoma_Defeated(BossGoma*, PlayState*);
    void BossGoma_FloorAttackPosture(BossGoma*, PlayState*);
    void BossGoma_FloorPrepareAttack(BossGoma*, PlayState*);
    void BossGoma_FloorAttack(BossGoma*, PlayState*);
    void BossGoma_FloorDamaged(BossGoma*, PlayState*);
    void BossGoma_FloorLandStruckDown(BossGoma*, PlayState*);
    void BossGoma_FloorLand(BossGoma*, PlayState*);
    void BossGoma_FloorStunned(BossGoma*, PlayState*);
    void BossGoma_FallJump(BossGoma*, PlayState*);
    void BossGoma_FallStruckDown(BossGoma*, PlayState*);
    void BossGoma_CeilingSpawnGohmas(BossGoma*, PlayState*);
    void BossGoma_CeilingPrepareSpawnGohmas(BossGoma*, PlayState*);
    void BossGoma_FloorIdle(BossGoma*, PlayState*);
    void BossGoma_CeilingIdle(BossGoma*, PlayState*);
    void BossGoma_FloorMain(BossGoma*, PlayState*);
    void BossGoma_WallClimb(BossGoma*, PlayState*);
    void BossGoma_CeilingMoveToCenter(BossGoma*, PlayState*);
}

namespace {

enum BossGomaActionId : u8 {
    BG_ENCOUNTER = 0,
    BG_DEFEATED = 1,
    BG_FLOOR_ATTACK_POSTURE = 2,
    BG_FLOOR_PREPARE_ATTACK = 3,
    BG_FLOOR_ATTACK = 4,
    BG_FLOOR_DAMAGED = 5,
    BG_FLOOR_LAND_STRUCK = 6,
    BG_FLOOR_LAND = 7,
    BG_FLOOR_STUNNED = 8,
    BG_FALL_JUMP = 9,
    BG_FALL_STRUCK = 10,
    BG_CEIL_SPAWN = 11,
    BG_CEIL_PREP_SPAWN = 12,
    BG_FLOOR_IDLE = 13,
    BG_CEIL_IDLE = 14,
    BG_FLOOR_MAIN = 15,
    BG_WALL_CLIMB = 16,
    BG_CEIL_MOVE = 17,
    BG_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(BossGomaActionFunc f) {
    if (f == BossGoma_Encounter) return BG_ENCOUNTER;
    if (f == BossGoma_Defeated) return BG_DEFEATED;
    if (f == BossGoma_FloorAttackPosture) return BG_FLOOR_ATTACK_POSTURE;
    if (f == BossGoma_FloorPrepareAttack) return BG_FLOOR_PREPARE_ATTACK;
    if (f == BossGoma_FloorAttack) return BG_FLOOR_ATTACK;
    if (f == BossGoma_FloorDamaged) return BG_FLOOR_DAMAGED;
    if (f == BossGoma_FloorLandStruckDown) return BG_FLOOR_LAND_STRUCK;
    if (f == BossGoma_FloorLand) return BG_FLOOR_LAND;
    if (f == BossGoma_FloorStunned) return BG_FLOOR_STUNNED;
    if (f == BossGoma_FallJump) return BG_FALL_JUMP;
    if (f == BossGoma_FallStruckDown) return BG_FALL_STRUCK;
    if (f == BossGoma_CeilingSpawnGohmas) return BG_CEIL_SPAWN;
    if (f == BossGoma_CeilingPrepareSpawnGohmas) return BG_CEIL_PREP_SPAWN;
    if (f == BossGoma_FloorIdle) return BG_FLOOR_IDLE;
    if (f == BossGoma_CeilingIdle) return BG_CEIL_IDLE;
    if (f == BossGoma_FloorMain) return BG_FLOOR_MAIN;
    if (f == BossGoma_WallClimb) return BG_WALL_CLIMB;
    if (f == BossGoma_CeilingMoveToCenter) return BG_CEIL_MOVE;
    return BG_UNKNOWN;
}

BossGomaActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case BG_ENCOUNTER: return BossGoma_Encounter;
        case BG_DEFEATED: return BossGoma_Defeated;
        case BG_FLOOR_ATTACK_POSTURE: return BossGoma_FloorAttackPosture;
        case BG_FLOOR_PREPARE_ATTACK: return BossGoma_FloorPrepareAttack;
        case BG_FLOOR_ATTACK: return BossGoma_FloorAttack;
        case BG_FLOOR_DAMAGED: return BossGoma_FloorDamaged;
        case BG_FLOOR_LAND_STRUCK: return BossGoma_FloorLandStruckDown;
        case BG_FLOOR_LAND: return BossGoma_FloorLand;
        case BG_FLOOR_STUNNED: return BossGoma_FloorStunned;
        case BG_FALL_JUMP: return BossGoma_FallJump;
        case BG_FALL_STRUCK: return BossGoma_FallStruckDown;
        case BG_CEIL_SPAWN: return BossGoma_CeilingSpawnGohmas;
        case BG_CEIL_PREP_SPAWN: return BossGoma_CeilingPrepareSpawnGohmas;
        case BG_FLOOR_IDLE: return BossGoma_FloorIdle;
        case BG_CEIL_IDLE: return BossGoma_CeilingIdle;
        case BG_FLOOR_MAIN: return BossGoma_FloorMain;
        case BG_WALL_CLIMB: return BossGoma_WallClimb;
        case BG_CEIL_MOVE: return BossGoma_CeilingMoveToCenter;
        default: return nullptr;
    }
}

void PlayAnimationFor(BossGoma* a, u8 actionId) {
    switch (actionId) {
        case BG_ENCOUNTER:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaWalkAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaWalkAnim), ANIMMODE_LOOP, -15.0f);
            break;
        case BG_DEFEATED:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaDeathAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaDeathAnim), ANIMMODE_ONCE, -2.0f);
            break;
        case BG_FLOOR_ATTACK_POSTURE:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaPrepareAttackAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaPrepareAttackAnim), ANIMMODE_ONCE, -10.0f);
            break;
        case BG_FLOOR_PREPARE_ATTACK:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaStandAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaStandAnim), ANIMMODE_LOOP, -10.0f);
            break;
        case BG_FLOOR_ATTACK:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaAttackAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaAttackAnim), ANIMMODE_ONCE, -10.0f);
            break;
        case BG_FLOOR_DAMAGED:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaDamageAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaDamageAnim), ANIMMODE_ONCE, -2.0f);
            break;
        case BG_FLOOR_LAND_STRUCK:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaCrashAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaCrashAnim), ANIMMODE_ONCE, -2.0f);
            break;
        case BG_FLOOR_LAND:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaLandAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaLandAnim), ANIMMODE_ONCE, -2.0f);
            break;
        case BG_FLOOR_STUNNED:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaStunnedAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaStunnedAnim), ANIMMODE_LOOP, -2.0f);
            break;
        case BG_FALL_JUMP:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaLandAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, -5.0f);
            break;
        case BG_FALL_STRUCK:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaCrashAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, -5.0f);
            break;
        case BG_CEIL_SPAWN:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaLayEggsAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaLayEggsAnim), ANIMMODE_LOOP, -15.0f);
            break;
        case BG_CEIL_PREP_SPAWN:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaPrepareEggsAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaPrepareEggsAnim), ANIMMODE_LOOP, -10.0f);
            break;
        case BG_FLOOR_IDLE:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaIdleCrouchedAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaIdleCrouchedAnim), ANIMMODE_LOOP, -5.0f);
            break;
        case BG_CEIL_IDLE:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaHangAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaHangAnim), ANIMMODE_LOOP, -5.0f);
            break;
        case BG_FLOOR_MAIN:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaWalkCrouchedAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaWalkCrouchedAnim), ANIMMODE_LOOP, -5.0f);
            break;
        case BG_WALL_CLIMB:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaClimbAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaClimbAnim), ANIMMODE_LOOP, -10.0f);
            break;
        case BG_CEIL_MOVE:
            Animation_Change(&a->skelanime, (AnimationHeader*)gGohmaWalkAnim, 1.0f, 0.0f,
                             Animation_GetLastFrame((void*)gGohmaWalkAnim), ANIMMODE_LOOP, -5.0f);
            break;
        default:
            break;
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<BossGoma*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGoma*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<BossGoma*>(actor);
    EnemySyncHelpers::SetJntSphAcHit(&a->collider);
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const BossGoma*>(actor);
    payload["bgAction"] = ActionFuncToId(a->actionFunc);
    payload["bgFrameCount"] = a->frameCount;
    payload["bgPatience"] = a->patienceTimer;
    payload["bgEyeLidBot"] = a->eyeLidBottomRotX;
    payload["bgEyeLidTop"] = a->eyeLidTopRotX;
    payload["bgEyeClosed"] = a->eyeClosedTimer;
    payload["bgIrisX"] = a->eyeIrisRotX;
    payload["bgIrisY"] = a->eyeIrisRotY;
    payload["bgEyeState"] = a->eyeState;
    payload["bgActionState"] = a->actionState;
    payload["bgFramesNext"] = a->framesUntilNextAction;
    payload["bgTimer"] = a->timer;
    payload["bgVisualState"] = a->visualState;
    payload["bgInvFrames"] = a->invincibilityFrames;
    payload["bgDecay"] = a->decayingProgress;
    payload["bgNoBack"] = a->noBackfaceCulling;
    payload["bgBlinkT"] = a->blinkTimer;
    payload["bgIrisSX"] = a->eyeIrisScaleX;
    payload["bgIrisSY"] = a->eyeIrisScaleY;
    payload["bgTail0"] = a->tailLimbsScale[0];
    payload["bgTail1"] = a->tailLimbsScale[1];
    payload["bgTail2"] = a->tailLimbsScale[2];
    payload["bgTail3"] = a->tailLimbsScale[3];
    payload["bgMainCR"] = a->mainEnvColor[0];
    payload["bgMainCG"] = a->mainEnvColor[1];
    payload["bgMainCB"] = a->mainEnvColor[2];
    payload["bgEyeCR"] = a->eyeEnvColor[0];
    payload["bgEyeCG"] = a->eyeEnvColor[1];
    payload["bgEyeCB"] = a->eyeEnvColor[2];
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<BossGoma*>(actor);

    u8 wireId = payload.value("bgAction", (u8)BG_UNKNOWN);
    BossGomaActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionFunc != desired) {
        PlayAnimationFor(a, wireId);
        a->actionFunc = desired;
    }

    a->frameCount = payload.value("bgFrameCount", (s16)0);
    a->patienceTimer = payload.value("bgPatience", (s16)0);
    a->eyeLidBottomRotX = payload.value("bgEyeLidBot", (s16)0);
    a->eyeLidTopRotX = payload.value("bgEyeLidTop", (s16)0);
    a->eyeClosedTimer = payload.value("bgEyeClosed", (s16)0);
    a->eyeIrisRotX = payload.value("bgIrisX", (s16)0);
    a->eyeIrisRotY = payload.value("bgIrisY", (s16)0);
    a->eyeState = payload.value("bgEyeState", (s16)0);
    a->actionState = payload.value("bgActionState", (s16)0);
    a->framesUntilNextAction = payload.value("bgFramesNext", (s16)0);
    a->timer = payload.value("bgTimer", (s16)0);
    a->visualState = payload.value("bgVisualState", (s16)0);
    a->invincibilityFrames = payload.value("bgInvFrames", (s16)0);
    a->decayingProgress = payload.value("bgDecay", (s16)0);
    a->noBackfaceCulling = payload.value("bgNoBack", (s16)0);
    a->blinkTimer = payload.value("bgBlinkT", (s16)0);
    a->eyeIrisScaleX = payload.value("bgIrisSX", 1.0f);
    a->eyeIrisScaleY = payload.value("bgIrisSY", 1.0f);
    a->tailLimbsScale[0] = payload.value("bgTail0", 1.0f);
    a->tailLimbsScale[1] = payload.value("bgTail1", 1.0f);
    a->tailLimbsScale[2] = payload.value("bgTail2", 1.0f);
    a->tailLimbsScale[3] = payload.value("bgTail3", 1.0f);
    a->mainEnvColor[0] = payload.value("bgMainCR", a->mainEnvColor[0]);
    a->mainEnvColor[1] = payload.value("bgMainCG", a->mainEnvColor[1]);
    a->mainEnvColor[2] = payload.value("bgMainCB", a->mainEnvColor[2]);
    a->eyeEnvColor[0] = payload.value("bgEyeCR", a->eyeEnvColor[0]);
    a->eyeEnvColor[1] = payload.value("bgEyeCG", a->eyeEnvColor[1]);
    a->eyeEnvColor[2] = payload.value("bgEyeCB", a->eyeEnvColor[2]);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_BOSS_GOMA, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
