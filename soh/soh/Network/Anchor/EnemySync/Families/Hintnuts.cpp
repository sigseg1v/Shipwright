#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Hintnuts/z_en_hintnuts.h"
#include "objects/object_hintnuts/object_hintnuts.h"

// Puzzle/hint Deku Scrub (the locked-room reflect-the-deku-nut variant).
// Like EnDekunuts, AI is driven by an actionFunc pointer. Without sync
// the peer's scrub stays stuck in Wait (hidden under the flower, height
// 5, AC off) while the authority's transitions through Burrow ->
// BeginRun -> Run after the player reflects a nut back at it. Map the
// actionFunc to a small enum and replay it on the peer, including
// animation + collider height + AC_ON so the visual matches.

extern "C" {
    void EnHintnuts_Wait(EnHintnuts*, PlayState*);
    void EnHintnuts_LookAround(EnHintnuts*, PlayState*);
    void EnHintnuts_Stand(EnHintnuts*, PlayState*);
    void EnHintnuts_ThrowNut(EnHintnuts*, PlayState*);
    void EnHintnuts_Burrow(EnHintnuts*, PlayState*);
    void EnHintnuts_BeginRun(EnHintnuts*, PlayState*);
    void EnHintnuts_BeginFreeze(EnHintnuts*, PlayState*);
    void EnHintnuts_Run(EnHintnuts*, PlayState*);
    void EnHintnuts_Talk(EnHintnuts*, PlayState*);
    void EnHintnuts_Leave(EnHintnuts*, PlayState*);
    void EnHintnuts_Freeze(EnHintnuts*, PlayState*);
}

namespace {

enum HintnutsAction : u8 {
    HN_WAIT = 0,
    HN_LOOK_AROUND = 1,
    HN_STAND = 2,
    HN_THROW_NUT = 3,
    HN_BURROW = 4,
    HN_BEGIN_RUN = 5,
    HN_BEGIN_FREEZE = 6,
    HN_RUN = 7,
    HN_TALK = 8,
    HN_LEAVE = 9,
    HN_FREEZE = 10,
    HN_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(EnHintnutsActionFunc f) {
    if (f == EnHintnuts_Wait) return HN_WAIT;
    if (f == EnHintnuts_LookAround) return HN_LOOK_AROUND;
    if (f == EnHintnuts_Stand) return HN_STAND;
    if (f == EnHintnuts_ThrowNut) return HN_THROW_NUT;
    if (f == EnHintnuts_Burrow) return HN_BURROW;
    if (f == EnHintnuts_BeginRun) return HN_BEGIN_RUN;
    if (f == EnHintnuts_BeginFreeze) return HN_BEGIN_FREEZE;
    if (f == EnHintnuts_Run) return HN_RUN;
    if (f == EnHintnuts_Talk) return HN_TALK;
    if (f == EnHintnuts_Leave) return HN_LEAVE;
    if (f == EnHintnuts_Freeze) return HN_FREEZE;
    return HN_UNKNOWN;
}

EnHintnutsActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case HN_WAIT: return EnHintnuts_Wait;
        case HN_LOOK_AROUND: return EnHintnuts_LookAround;
        case HN_STAND: return EnHintnuts_Stand;
        case HN_THROW_NUT: return EnHintnuts_ThrowNut;
        case HN_BURROW: return EnHintnuts_Burrow;
        case HN_BEGIN_RUN: return EnHintnuts_BeginRun;
        case HN_BEGIN_FREEZE: return EnHintnuts_BeginFreeze;
        case HN_RUN: return EnHintnuts_Run;
        case HN_TALK: return EnHintnuts_Talk;
        case HN_LEAVE: return EnHintnuts_Leave;
        case HN_FREEZE: return EnHintnuts_Freeze;
        default: return nullptr;
    }
}

// On peer-side action transition, set the matching animation so the
// scrub visibly unburrows / runs / freezes rather than staying frozen
// on the prior animation. Mirrors the Animation_* call inside each
// SetupX() in z_en_hintnuts.c, minus the side effects (sounds, item
// spawns, sPuzzleCounter mutations) which we don't want to duplicate
// on the peer.
void PlayAnimationFor(EnHintnuts* a, u8 actionId) {
    switch (actionId) {
        case HN_WAIT:
            Animation_PlayOnceSetSpeed(&a->skelAnime, (AnimationHeader*)gHintNutsUpAnim, 0.0f);
            break;
        case HN_LOOK_AROUND:
            Animation_PlayLoop(&a->skelAnime, (AnimationHeader*)gHintNutsLookAroundAnim);
            break;
        case HN_STAND:
            Animation_MorphToLoop(&a->skelAnime, (AnimationHeader*)gHintNutsStandAnim, -3.0f);
            break;
        case HN_THROW_NUT:
            Animation_PlayOnce(&a->skelAnime, (AnimationHeader*)gHintNutsSpitAnim);
            break;
        case HN_BURROW:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)gHintNutsBurrowAnim, -5.0f);
            break;
        case HN_BEGIN_RUN:
        case HN_BEGIN_FREEZE:
            Animation_MorphToPlayOnce(&a->skelAnime, (AnimationHeader*)gHintNutsUnburrowAnim, -3.0f);
            break;
        case HN_RUN:
            Animation_PlayLoop(&a->skelAnime, (AnimationHeader*)gHintNutsRunAnim);
            break;
        case HN_TALK:
            Animation_MorphToLoop(&a->skelAnime, (AnimationHeader*)gHintNutsTalkAnim, -5.0f);
            break;
        case HN_LEAVE:
            Animation_MorphToLoop(&a->skelAnime, (AnimationHeader*)gHintNutsRunAnim, -5.0f);
            break;
        case HN_FREEZE:
            Animation_PlayLoop(&a->skelAnime, (AnimationHeader*)gHintNutsFreezeAnim);
            break;
        default:
            break;
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnHintnuts*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnHintnuts*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnHintnuts*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->collider);
}

// EnHintnuts_ColliderCheck branches on `collider.base.ac->id`: the
// reflected-nutsball case takes the BeginRun / BeginFreeze path (the
// scrub pops up and runs), anything else takes the burrow path. Forward
// the attacker id so the authority's synthesized AC_HIT preserves that
// distinction. Returns 0 if the engine never landed a real local hit
// this frame (base.ac unset) -- HandleNonAuthorityHit only forwards on
// damage > 0 anyway, and a nonzero damage implies CollisionCheck_AC
// just populated base.ac.
u16 GetAttackerActorId(Actor* actor) {
    auto* a = reinterpret_cast<EnHintnuts*>(actor);
    if (a->collider.base.ac == nullptr) {
        return 0;
    }
    return (u16)a->collider.base.ac->id;
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnHintnuts*>(actor);
    payload["hnAction"] = ActionFuncToId(a->actionFunc);
    payload["hnTimer"] = a->animFlagAndTimer;
    payload["hnUnk196"] = a->unk_196;
    payload["hnColH"] = a->collider.dim.height;
    payload["hnAcOn"] = (a->collider.base.acFlags & AC_ON) != 0;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnHintnuts*>(actor);

    u8 wireId = payload.value("hnAction", (u8)HN_UNKNOWN);
    EnHintnutsActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionFunc != desired) {
        SPDLOG_INFO("[Anchor:diag] Hintnuts ApplyAI action {} -> {}",
                    (int)ActionFuncToId(a->actionFunc), (int)wireId);
        PlayAnimationFor(a, wireId);
        a->actionFunc = desired;
    }

    a->animFlagAndTimer = payload.value("hnTimer", (s16)0);
    a->unk_196 = payload.value("hnUnk196", (s16)0);
    a->collider.dim.height = payload.value("hnColH", (s16)5);

    bool acOn = payload.value("hnAcOn", false);
    if (acOn) {
        a->collider.base.acFlags |= AC_ON;
    } else {
        a->collider.base.acFlags &= ~AC_ON;
    }
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_HINTNUTS, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits, &GetAttackerActorId }));
