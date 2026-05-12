#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Wallmas/z_en_wallmas.h"
#include "objects/object_wallmaster/object_wallmaster.h"

// Wallmaster. Single ColliderCylinder driven by an actionFunc state
// machine that runs from invisible-on-ceiling -> WaitToDrop -> Drop ->
// Land -> Stand -> Walk -> JumpToCeiling -> ReturnToCeiling. Without AI
// sync the peer's wallmaster never enters Drop and just hovers
// invisibly while the authority's drops on the player.

extern "C" {
    void EnWallmas_Draw(Actor*, PlayState*);
    void EnWallmas_WaitToDrop(EnWallmas*, PlayState*);
    void EnWallmas_Drop(EnWallmas*, PlayState*);
    void EnWallmas_Land(EnWallmas*, PlayState*);
    void EnWallmas_Stand(EnWallmas*, PlayState*);
    void EnWallmas_JumpToCeiling(EnWallmas*, PlayState*);
    void EnWallmas_ReturnToCeiling(EnWallmas*, PlayState*);
    void EnWallmas_TakeDamage(EnWallmas*, PlayState*);
    void EnWallmas_Cooldown(EnWallmas*, PlayState*);
    void EnWallmas_Die(EnWallmas*, PlayState*);
    void EnWallmas_TakePlayer(EnWallmas*, PlayState*);
    void EnWallmas_WaitForProximity(EnWallmas*, PlayState*);
    void EnWallmas_WaitForSwitchFlag(EnWallmas*, PlayState*);
    void EnWallmas_Stun(EnWallmas*, PlayState*);
    void EnWallmas_Walk(EnWallmas*, PlayState*);
}

namespace {

enum WallmasAction : u8 {
    WM_WAIT_PROX = 0,
    WM_WAIT_FLAG = 1,
    WM_WAIT_DROP = 2,
    WM_DROP = 3,
    WM_LAND = 4,
    WM_STAND = 5,
    WM_WALK = 6,
    WM_JUMP_TO_CEILING = 7,
    WM_RETURN_TO_CEILING = 8,
    WM_TAKE_DAMAGE = 9,
    WM_COOLDOWN = 10,
    WM_DIE = 11,
    WM_TAKE_PLAYER = 12,
    WM_STUN = 13,
    WM_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(EnWallmasActionFunc f) {
    if (f == EnWallmas_WaitForProximity) return WM_WAIT_PROX;
    if (f == EnWallmas_WaitForSwitchFlag) return WM_WAIT_FLAG;
    if (f == EnWallmas_WaitToDrop) return WM_WAIT_DROP;
    if (f == EnWallmas_Drop) return WM_DROP;
    if (f == EnWallmas_Land) return WM_LAND;
    if (f == EnWallmas_Stand) return WM_STAND;
    if (f == EnWallmas_Walk) return WM_WALK;
    if (f == EnWallmas_JumpToCeiling) return WM_JUMP_TO_CEILING;
    if (f == EnWallmas_ReturnToCeiling) return WM_RETURN_TO_CEILING;
    if (f == EnWallmas_TakeDamage) return WM_TAKE_DAMAGE;
    if (f == EnWallmas_Cooldown) return WM_COOLDOWN;
    if (f == EnWallmas_Die) return WM_DIE;
    if (f == EnWallmas_TakePlayer) return WM_TAKE_PLAYER;
    if (f == EnWallmas_Stun) return WM_STUN;
    return WM_UNKNOWN;
}

EnWallmasActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case WM_WAIT_PROX: return EnWallmas_WaitForProximity;
        case WM_WAIT_FLAG: return EnWallmas_WaitForSwitchFlag;
        case WM_WAIT_DROP: return EnWallmas_WaitToDrop;
        case WM_DROP: return EnWallmas_Drop;
        case WM_LAND: return EnWallmas_Land;
        case WM_STAND: return EnWallmas_Stand;
        case WM_WALK: return EnWallmas_Walk;
        case WM_JUMP_TO_CEILING: return EnWallmas_JumpToCeiling;
        case WM_RETURN_TO_CEILING: return EnWallmas_ReturnToCeiling;
        case WM_TAKE_DAMAGE: return EnWallmas_TakeDamage;
        case WM_COOLDOWN: return EnWallmas_Cooldown;
        case WM_DIE: return EnWallmas_Die;
        case WM_TAKE_PLAYER: return EnWallmas_TakePlayer;
        case WM_STUN: return EnWallmas_Stun;
        default: return nullptr;
    }
}

// Mirror the Animation_* call in each SetupX in z_en_wallmas.c. Skip
// the side effects (sounds, dust ring spawn in SetupLand, item drop in
// SetupDie, OnePointCutscene_Init in SetupTakePlayer) which the
// authority already executes.
void PlayAnimationFor(EnWallmas* a, u8 actionId) {
    switch (actionId) {
        case WM_WAIT_DROP:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterLungeAnim, 0.0f, 20.0f,
                             Animation_GetLastFrame((void*)gWallmasterLungeAnim), ANIMMODE_ONCE, 0.0f);
            break;
        case WM_DROP:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 1.0f, 41.0f,
                             Animation_GetLastFrame((void*)gWallmasterJumpAnim), ANIMMODE_ONCE, -3.0f);
            break;
        case WM_LAND:
            // Land setup uses same gWallmasterJumpAnim animation continuation;
            // morph into it without re-changing.
            break;
        case WM_STAND:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterStandUpAnim);
            break;
        case WM_WALK:
            Animation_PlayOnceSetSpeed(&a->skelAnime, (AnimationHeader*)gWallmasterWalkAnim, 3.0f);
            break;
        case WM_JUMP_TO_CEILING:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterStopWalkAnim);
            break;
        case WM_RETURN_TO_CEILING:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 3.0f, 0.0f,
                             Animation_GetLastFrame((void*)gWallmasterJumpAnim), ANIMMODE_ONCE, -3.0f);
            break;
        case WM_TAKE_DAMAGE:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterDamageAnim, -3.0f);
            break;
        case WM_COOLDOWN:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterRecoverFromDamageAnim);
            break;
        case WM_TAKE_PLAYER:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)gWallmasterHoverAnim, -5.0f);
            break;
        case WM_STUN:
            Animation_Change(&a->skelAnime, (AnimationHeader*)gWallmasterJumpAnim, 1.5f, 0, 20.0f,
                             ANIMMODE_ONCE, -3.0f);
            break;
        default:
            break;
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnWallmas*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnWallmas*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnWallmas*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->collider);
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnWallmas*>(actor);
    payload["wmAction"] = ActionFuncToId(a->actionFunc);
    payload["wmTimer"] = a->timer;
    payload["wmYTarget"] = a->yTarget;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnWallmas*>(actor);

    u8 wireId = payload.value("wmAction", (u8)WM_UNKNOWN);
    EnWallmasActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionFunc != desired) {
        PlayAnimationFor(a, wireId);
        a->actionFunc = desired;
        // ProximityOrSwitchInit set actor.draw = NULL so the unspawned
        // wallmaster is invisible while waiting. TimerInit re-sets it
        // when transitioning to WaitToDrop. Mirror that for the peer
        // any time it leaves a waiting state.
        if (wireId != WM_WAIT_PROX && wireId != WM_WAIT_FLAG) {
            a->actor.draw = EnWallmas_Draw;
        }
        if (wireId == WM_DROP) {
            a->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
            a->actor.flags &= ~ACTOR_FLAG_DRAW_CULLING_DISABLED;
        }
    }

    a->timer = payload.value("wmTimer", (s16)0);
    a->yTarget = payload.value("wmYTarget", 0.0f);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_WALLMAS, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
