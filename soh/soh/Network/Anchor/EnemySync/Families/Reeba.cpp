#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Reeba/z_en_reeba.h"

// Leever. Single ColliderCylinder. The key visible state is the
// actor.shape.yOffset (depth below the ground while burrowed) and
// shadowScale (the rising/sinking shadow). Both are mutated inside
// Update which is suppressed on the peer, so without sync the peer
// sees a frozen leever at its initial yOffset (-2000.0 from init,
// i.e. deeply buried) while the authority's surfaces and chases.

extern "C" {
    void EnReeba_SetupSurface(EnReeba*, PlayState*);
    void EnReeba_Surface(EnReeba*, PlayState*);
    void EnReeba_Move(EnReeba*, PlayState*);
    void EnReeba_SetupSink(EnReeba*, PlayState*);
    void EnReeba_Sink(EnReeba*, PlayState*);
    void EnReeba_SetupMoveBig(EnReeba*, PlayState*);
    void EnReeba_MoveBig(EnReeba*, PlayState*);
    void EnReeba_Recoiled(EnReeba*, PlayState*);
    void EnReeba_SetupDamaged(EnReeba*, PlayState*);
    void EnReeba_Damaged(EnReeba*, PlayState*);
    void EnReeba_Die(EnReeba*, PlayState*);
    void EnReeba_Stunned(EnReeba*, PlayState*);
    void EnReeba_StunDie(EnReeba*, PlayState*);
    void EnReeba_StunRecover(EnReeba*, PlayState*);
}

namespace {

enum ReebaAction : u8 {
    RE_SETUP_SURFACE = 0,
    RE_SURFACE = 1,
    RE_MOVE = 2,
    RE_SETUP_SINK = 3,
    RE_SINK = 4,
    RE_SETUP_MOVE_BIG = 5,
    RE_MOVE_BIG = 6,
    RE_RECOILED = 7,
    RE_SETUP_DAMAGED = 8,
    RE_DAMAGED = 9,
    RE_DIE = 10,
    RE_STUNNED = 11,
    RE_STUN_DIE = 12,
    RE_STUN_RECOVER = 13,
    RE_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(EnReebaActionFunc f) {
    if (f == EnReeba_SetupSurface) return RE_SETUP_SURFACE;
    if (f == EnReeba_Surface) return RE_SURFACE;
    if (f == EnReeba_Move) return RE_MOVE;
    if (f == EnReeba_SetupSink) return RE_SETUP_SINK;
    if (f == EnReeba_Sink) return RE_SINK;
    if (f == EnReeba_SetupMoveBig) return RE_SETUP_MOVE_BIG;
    if (f == EnReeba_MoveBig) return RE_MOVE_BIG;
    if (f == EnReeba_Recoiled) return RE_RECOILED;
    if (f == EnReeba_SetupDamaged) return RE_SETUP_DAMAGED;
    if (f == EnReeba_Damaged) return RE_DAMAGED;
    if (f == EnReeba_Die) return RE_DIE;
    if (f == EnReeba_Stunned) return RE_STUNNED;
    if (f == EnReeba_StunDie) return RE_STUN_DIE;
    if (f == EnReeba_StunRecover) return RE_STUN_RECOVER;
    return RE_UNKNOWN;
}

EnReebaActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case RE_SETUP_SURFACE: return EnReeba_SetupSurface;
        case RE_SURFACE: return EnReeba_Surface;
        case RE_MOVE: return EnReeba_Move;
        case RE_SETUP_SINK: return EnReeba_SetupSink;
        case RE_SINK: return EnReeba_Sink;
        case RE_SETUP_MOVE_BIG: return EnReeba_SetupMoveBig;
        case RE_MOVE_BIG: return EnReeba_MoveBig;
        case RE_RECOILED: return EnReeba_Recoiled;
        case RE_SETUP_DAMAGED: return EnReeba_SetupDamaged;
        case RE_DAMAGED: return EnReeba_Damaged;
        case RE_DIE: return EnReeba_Die;
        case RE_STUNNED: return EnReeba_Stunned;
        case RE_STUN_DIE: return EnReeba_StunDie;
        case RE_STUN_RECOVER: return EnReeba_StunRecover;
        default: return nullptr;
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnReeba*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnReeba*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnReeba*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->collider);
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnReeba*>(actor);
    payload["reAction"] = ActionFuncToId(a->actionfunc);
    payload["reYOffset"] = a->actor.shape.yOffset;
    payload["reShadow"] = a->actor.shape.shadowScale;
    payload["reYOffStep"] = a->yOffsetStep;
    payload["reYOffTgt"] = a->yOffsetTarget;
    payload["reMoveT"] = a->moveTimer;
    payload["reWaitT"] = a->waitTimer;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnReeba*>(actor);

    u8 wireId = payload.value("reAction", (u8)RE_UNKNOWN);
    EnReebaActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionfunc != desired) {
        a->actionfunc = desired;
    }

    a->actor.shape.yOffset = payload.value("reYOffset", a->actor.shape.yOffset);
    a->actor.shape.shadowScale = payload.value("reShadow", a->actor.shape.shadowScale);
    a->yOffsetStep = payload.value("reYOffStep", a->yOffsetStep);
    a->yOffsetTarget = payload.value("reYOffTgt", a->yOffsetTarget);
    a->moveTimer = payload.value("reMoveT", (s16)0);
    a->waitTimer = payload.value("reWaitT", (s16)0);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_REEBA, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
