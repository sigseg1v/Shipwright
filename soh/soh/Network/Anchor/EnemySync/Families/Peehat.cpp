#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Peehat/z_en_peehat.h"

// Peahat. One ColliderCylinder (body) plus one ColliderJntSph (blade
// hub) plus one ColliderQuad (spinning blade).
//
// Peehat is a particularly bad offender for missing AI sync: the
// PEAHAT_TYPE_GROUNDED variant starts buried in the ground (state 3,
// xz dist gates a rise transition), and the peer would otherwise never
// see it lift off when the authority's transitions to Fly. The `state`
// field also drives drawing/scale, so syncing it (plus the actionFunc
// pointer) is required for the peer to render the same body the
// authority does. blade rotation and jiggle (used during attack
// recoil) are also visible-only state.

extern "C" {
    void EnPeehat_Ground_SetStateGround(EnPeehat*);
    void EnPeehat_Flying_SetStateGround(EnPeehat*);
    void EnPeehat_Larva_SetStateSeekPlayer(EnPeehat*);
    void EnPeehat_Ground_StateGround(EnPeehat*, PlayState*);
    void EnPeehat_Ground_SetStateRise(EnPeehat*);
    void EnPeehat_Flying_StateGrounded(EnPeehat*, PlayState*);
    void EnPeehat_Flying_SetStateRise(EnPeehat*);
    void EnPeehat_Flying_StateFly(EnPeehat*, PlayState*);
    void EnPeehat_Flying_SetStateLanding(EnPeehat*);
    void EnPeehat_Ground_StateRise(EnPeehat*, PlayState*);
    void EnPeehat_Ground_SetStateHover(EnPeehat*);
    void EnPeehat_Flying_StateRise(EnPeehat*, PlayState*);
    void EnPeehat_Ground_StateSeekPlayer(EnPeehat*, PlayState*);
    void EnPeehat_Ground_SetStateReturnHome(EnPeehat*);
    void EnPeehat_Ground_SetStateLanding(EnPeehat*);
    void EnPeehat_Larva_StateSeekPlayer(EnPeehat*, PlayState*);
    void EnPeehat_SetStateAttackRecoil(EnPeehat*);
    void EnPeehat_Ground_StateLanding(EnPeehat*, PlayState*);
    void EnPeehat_Flying_StateLanding(EnPeehat*, PlayState*);
    void EnPeehat_Ground_StateHover(EnPeehat*, PlayState*);
    void EnPeehat_Ground_StateReturnHome(EnPeehat*, PlayState*);
    void EnPeehat_StateAttackRecoil(EnPeehat*, PlayState*);
    void EnPeehat_StateBoomerangStunned(EnPeehat*, PlayState*);
    void EnPeehat_Adult_StateDie(EnPeehat*, PlayState*);
    void EnPeehat_SetStateExplode(EnPeehat*);
    void EnPeehat_StateExplode(EnPeehat*, PlayState*);
}

namespace {

enum PeehatAction : u8 {
    PH_G_GROUND = 0, PH_G_RISE = 1, PH_G_HOVER = 2, PH_G_SEEK = 3,
    PH_G_RETURN = 4, PH_G_LANDING = 5,
    PH_F_GROUNDED = 6, PH_F_RISE = 7, PH_F_FLY = 8, PH_F_LANDING = 9,
    PH_L_SEEK = 10,
    PH_ATTACK_RECOIL = 11, PH_BOOMERANG_STUNNED = 12,
    PH_DIE = 13, PH_EXPLODE = 14,
    PH_UNKNOWN = 0xFF,
};

u8 ActionFuncToId(EnPeehatActionFunc f) {
    if (f == EnPeehat_Ground_StateGround) return PH_G_GROUND;
    if (f == EnPeehat_Ground_StateRise) return PH_G_RISE;
    if (f == EnPeehat_Ground_StateHover) return PH_G_HOVER;
    if (f == EnPeehat_Ground_StateSeekPlayer) return PH_G_SEEK;
    if (f == EnPeehat_Ground_StateReturnHome) return PH_G_RETURN;
    if (f == EnPeehat_Ground_StateLanding) return PH_G_LANDING;
    if (f == EnPeehat_Flying_StateGrounded) return PH_F_GROUNDED;
    if (f == EnPeehat_Flying_StateRise) return PH_F_RISE;
    if (f == EnPeehat_Flying_StateFly) return PH_F_FLY;
    if (f == EnPeehat_Flying_StateLanding) return PH_F_LANDING;
    if (f == EnPeehat_Larva_StateSeekPlayer) return PH_L_SEEK;
    if (f == EnPeehat_StateAttackRecoil) return PH_ATTACK_RECOIL;
    if (f == EnPeehat_StateBoomerangStunned) return PH_BOOMERANG_STUNNED;
    if (f == EnPeehat_Adult_StateDie) return PH_DIE;
    if (f == EnPeehat_StateExplode) return PH_EXPLODE;
    return PH_UNKNOWN;
}

EnPeehatActionFunc IdToActionFunc(u8 id) {
    switch (id) {
        case PH_G_GROUND: return EnPeehat_Ground_StateGround;
        case PH_G_RISE: return EnPeehat_Ground_StateRise;
        case PH_G_HOVER: return EnPeehat_Ground_StateHover;
        case PH_G_SEEK: return EnPeehat_Ground_StateSeekPlayer;
        case PH_G_RETURN: return EnPeehat_Ground_StateReturnHome;
        case PH_G_LANDING: return EnPeehat_Ground_StateLanding;
        case PH_F_GROUNDED: return EnPeehat_Flying_StateGrounded;
        case PH_F_RISE: return EnPeehat_Flying_StateRise;
        case PH_F_FLY: return EnPeehat_Flying_StateFly;
        case PH_F_LANDING: return EnPeehat_Flying_StateLanding;
        case PH_L_SEEK: return EnPeehat_Larva_StateSeekPlayer;
        case PH_ATTACK_RECOIL: return EnPeehat_StateAttackRecoil;
        case PH_BOOMERANG_STUNNED: return EnPeehat_StateBoomerangStunned;
        case PH_DIE: return EnPeehat_Adult_StateDie;
        case PH_EXPLODE: return EnPeehat_StateExplode;
        default: return nullptr;
    }
}

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnPeehat*>(actor);
    EnemySyncHelpers::RegisterCyl(actor, &a->colCylinder);
    EnemySyncHelpers::RegisterJntSph(&a->colJntSph);
    EnemySyncHelpers::RegisterColliderCommon(&a->colQuad.base);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPeehat*>(actor);
    a->colCylinder.base.acFlags &= ~AC_HIT;
    a->colJntSph.base.acFlags &= ~AC_HIT;
    a->colQuad.base.acFlags &= ~AC_HIT;
}

void SetACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnPeehat*>(actor);
    EnemySyncHelpers::SetCylAcHit(&a->colCylinder);
    EnemySyncHelpers::SetJntSphAcHit(&a->colJntSph);
    a->colQuad.base.acFlags |= AC_HIT;
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnPeehat*>(actor);
    payload["phAction"] = ActionFuncToId(a->actionFunc);
    payload["phState"] = a->state;
    payload["phBladeRot"] = a->bladeRot;
    payload["phBladeVel"] = a->bladeRotVel;
    payload["phJiggleRot"] = a->jiggleRot;
    payload["phScaleShift"] = a->scaleShift;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnPeehat*>(actor);

    u8 wireId = payload.value("phAction", (u8)PH_UNKNOWN);
    EnPeehatActionFunc desired = IdToActionFunc(wireId);
    if (desired != nullptr && a->actionFunc != desired) {
        a->actionFunc = desired;
    }

    a->state = payload.value("phState", a->state);
    a->bladeRot = payload.value("phBladeRot", (s16)0);
    a->bladeRotVel = payload.value("phBladeVel", (s16)0);
    a->jiggleRot = payload.value("phJiggleRot", 0.0f);
    a->scaleShift = payload.value("phScaleShift", 0.0f);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_PEEHAT, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI, &SetACHits }));
