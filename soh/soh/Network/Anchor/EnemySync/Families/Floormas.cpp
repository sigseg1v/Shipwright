#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

// z_en_floormas.h declares an action-func typedef using `this` as the
// parameter name, which is a reserved word in C++. Locally rename it
// during include only -- the redefinition is identifier-name only, so
// the struct layout and any `this` callers in .c files are unaffected.
#define this thisx
#include "src/overlays/actors/ovl_En_Floormas/z_en_floormas.h"
#undef this
#include "objects/object_wallmaster/object_wallmaster.h"

// Floormaster. Single ColliderCylinder; big AI state machine with two
// modes (big body, then three small bodies after Split). Without AI
// sync the peer sees the big body freeze in Stand while the authority
// hovers / charges / splits into three small bodies that visibly
// shrink. The small bodies' actor scale also needs syncing.

extern "C" {
    void EnFloormas_BigDecideAction(EnFloormas*, PlayState*);
    void EnFloormas_Stand(EnFloormas*, PlayState*);
    void EnFloormas_BigWalk(EnFloormas*, PlayState*);
    void EnFloormas_BigStopWalk(EnFloormas*, PlayState*);
    void EnFloormas_Run(EnFloormas*, PlayState*);
    void EnFloormas_Turn(EnFloormas*, PlayState*);
    void EnFloormas_Hover(EnFloormas*, PlayState*);
    void EnFloormas_Charge(EnFloormas*, PlayState*);
    void EnFloormas_Land(EnFloormas*, PlayState*);
    void EnFloormas_Split(EnFloormas*, PlayState*);
    void EnFloormas_SmWalk(EnFloormas*, PlayState*);
    void EnFloormas_SmDecideAction(EnFloormas*, PlayState*);
    void EnFloormas_SmShrink(EnFloormas*, PlayState*);
    void EnFloormas_SmSlaveJumpAtMaster(EnFloormas*, PlayState*);
    void EnFloormas_JumpAtLink(EnFloormas*, PlayState*);
    void EnFloormas_GrabLink(EnFloormas*, PlayState*);
    void EnFloormas_Merge(EnFloormas*, PlayState*);
    void EnFloormas_SmWait(EnFloormas*, PlayState*);
    void EnFloormas_TakeDamage(EnFloormas*, PlayState*);
    void EnFloormas_Recover(EnFloormas*, PlayState*);
    void EnFloormas_Freeze(EnFloormas*, PlayState*);
}

namespace {

enum FloormasAction : u8 {
    FM_BIG_DECIDE = 0,
    FM_STAND = 1,
    FM_BIG_WALK = 2,
    FM_BIG_STOP_WALK = 3,
    FM_RUN = 4,
    FM_TURN = 5,
    FM_HOVER = 6,
    FM_CHARGE = 7,
    FM_LAND = 8,
    FM_SPLIT = 9,
    FM_SM_WALK = 10,
    FM_SM_DECIDE = 11,
    FM_SM_SHRINK = 12,
    FM_SM_SLAVE_JUMP = 13,
    FM_JUMP_AT_LINK = 14,
    FM_GRAB_LINK = 15,
    FM_MERGE = 16,
    FM_SM_WAIT = 17,
    FM_TAKE_DAMAGE = 18,
    FM_RECOVER = 19,
    FM_FREEZE = 20,
    FM_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(EnFloormasActionFunc f) {
    if (f == EnFloormas_BigDecideAction) return FM_BIG_DECIDE;
    if (f == EnFloormas_Stand) return FM_STAND;
    if (f == EnFloormas_BigWalk) return FM_BIG_WALK;
    if (f == EnFloormas_BigStopWalk) return FM_BIG_STOP_WALK;
    if (f == EnFloormas_Run) return FM_RUN;
    if (f == EnFloormas_Turn) return FM_TURN;
    if (f == EnFloormas_Hover) return FM_HOVER;
    if (f == EnFloormas_Charge) return FM_CHARGE;
    if (f == EnFloormas_Land) return FM_LAND;
    if (f == EnFloormas_Split) return FM_SPLIT;
    if (f == EnFloormas_SmWalk) return FM_SM_WALK;
    if (f == EnFloormas_SmDecideAction) return FM_SM_DECIDE;
    if (f == EnFloormas_SmShrink) return FM_SM_SHRINK;
    if (f == EnFloormas_SmSlaveJumpAtMaster) return FM_SM_SLAVE_JUMP;
    if (f == EnFloormas_JumpAtLink) return FM_JUMP_AT_LINK;
    if (f == EnFloormas_GrabLink) return FM_GRAB_LINK;
    if (f == EnFloormas_Merge) return FM_MERGE;
    if (f == EnFloormas_SmWait) return FM_SM_WAIT;
    if (f == EnFloormas_TakeDamage) return FM_TAKE_DAMAGE;
    if (f == EnFloormas_Recover) return FM_RECOVER;
    if (f == EnFloormas_Freeze) return FM_FREEZE;
    return FM_UNKNOWN;
}

EnFloormasActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case FM_BIG_DECIDE: return EnFloormas_BigDecideAction;
        case FM_STAND: return EnFloormas_Stand;
        case FM_BIG_WALK: return EnFloormas_BigWalk;
        case FM_BIG_STOP_WALK: return EnFloormas_BigStopWalk;
        case FM_RUN: return EnFloormas_Run;
        case FM_TURN: return EnFloormas_Turn;
        case FM_HOVER: return EnFloormas_Hover;
        case FM_CHARGE: return EnFloormas_Charge;
        case FM_LAND: return EnFloormas_Land;
        case FM_SPLIT: return EnFloormas_Split;
        case FM_SM_WALK: return EnFloormas_SmWalk;
        case FM_SM_DECIDE: return EnFloormas_SmDecideAction;
        case FM_SM_SHRINK: return EnFloormas_SmShrink;
        case FM_SM_SLAVE_JUMP: return EnFloormas_SmSlaveJumpAtMaster;
        case FM_JUMP_AT_LINK: return EnFloormas_JumpAtLink;
        case FM_GRAB_LINK: return EnFloormas_GrabLink;
        case FM_MERGE: return EnFloormas_Merge;
        case FM_SM_WAIT: return EnFloormas_SmWait;
        case FM_TAKE_DAMAGE: return EnFloormas_TakeDamage;
        case FM_RECOVER: return EnFloormas_Recover;
        case FM_FREEZE: return EnFloormas_Freeze;
        default: return nullptr;
    }
}

void PlayAnimationFor(EnFloormas* a, u8 actionId) {
    switch (actionId) {
        case FM_BIG_DECIDE:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterWaitAnim);
            break;
        case FM_STAND:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterStandUpAnim, -3.0f);
            break;
        case FM_BIG_WALK:
        case FM_SM_WALK:
        case FM_SM_DECIDE:
            Animation_PlayLoopSetSpeed(&a->skelAnime, (AnimationHeader*)gWallmasterWalkAnim,
                                       actionId == FM_BIG_WALK ? 1.5f : 4.5f);
            break;
        case FM_BIG_STOP_WALK:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterStopWalkAnim);
            break;
        case FM_TURN:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)gFloormasterTurnAnim, -3.0f);
            break;
        case FM_HOVER:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterHoverAnim, 3.0f, 0,
                             Animation_GetLastFrame((void*)gWallmasterHoverAnim), ANIMMODE_LOOP, 0.0f);
            break;
        case FM_LAND:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 1.0f, 41.0f, 42.0f,
                             ANIMMODE_ONCE, 5.0f);
            break;
        case FM_SPLIT:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 1.0f, 41.0f,
                             Animation_GetLastFrame((void*)gWallmasterJumpAnim), ANIMMODE_ONCE, 0.0f);
            break;
        case FM_SM_SLAVE_JUMP:
        case FM_JUMP_AT_LINK:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 2.0f, 0.0f, 41.0f,
                             ANIMMODE_ONCE, 0.0f);
            break;
        case FM_GRAB_LINK:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 1.0f, 36.0f, 45.0f,
                             ANIMMODE_ONCE, -3.0f);
            break;
        case FM_MERGE:
        case FM_SM_WAIT:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterWaitAnim);
            break;
        case FM_TAKE_DAMAGE:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterDamageAnim, -3.0f);
            break;
        case FM_RECOVER:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterRecoverFromDamageAnim);
            break;
        case FM_FREEZE:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 1.5f, 0, 20.0f,
                             ANIMMODE_ONCE, -3.0f);
            break;
        default:
            break;
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnFloormas*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFloormas*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnFloormas*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->collider);
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnFloormas*>(actor);
    payload["fmAction"] = ActionFuncToId(a->actionFunc);
    payload["fmActionTimer"] = a->actionTimer;
    payload["fmActionTarget"] = a->actionTarget;
    payload["fmZOffset"] = a->zOffset;
    payload["fmSmTimer"] = a->smActionTimer;
    payload["fmScaleX"] = a->actor.scale.x;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnFloormas*>(actor);

    u8 wireId = payload.value("fmAction", (u8)FM_UNKNOWN);
    EnFloormasActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionFunc != desired) {
        PlayAnimationFor(a, wireId);
        a->actionFunc = desired;
    }

    a->actionTimer = payload.value("fmActionTimer", (s16)0);
    a->actionTarget = payload.value("fmActionTarget", (s16)0);
    a->zOffset = payload.value("fmZOffset", (s16)0);
    a->smActionTimer = payload.value("fmSmTimer", (s16)0);

    // After Split the three small bodies are visibly smaller. Without
    // scale sync they render full-size on the peer.
    f32 scale = payload.value("fmScaleX", a->actor.scale.x);
    a->actor.scale.x = scale;
    a->actor.scale.y = scale;
    a->actor.scale.z = scale;
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_FLOORMAS, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
