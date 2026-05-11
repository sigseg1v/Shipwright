#include "../Family.h"
#include "../Helpers.h"
#include "../Registry.h"

#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"

// Stalchild. JntSph collider on the body. Action state, break flags,
// and the headless yaw offset all need plumbing through ENEMY_UPDATE
// because they're driven from inside the suppressed update fn -- without
// them the puppet on a non-authority client snaps back to the wrong
// animation phase between packets.

namespace {

void RegisterAC(Actor* actor) {
    auto* a = reinterpret_cast<EnSkb*>(actor);
    EnemySyncHelpers::RegisterJntSph(&a->collider);
}

void ClearACHits(Actor* actor) {
    auto* a = reinterpret_cast<EnSkb*>(actor);
    a->collider.base.acFlags &= ~AC_HIT;
}

void SerializeAI(const Actor* actor, nlohmann::json& payload) {
    const auto* a = reinterpret_cast<const EnSkb*>(actor);
    payload["skbActionState"] = a->actionState;
    payload["skbBreakFlags"] = a->breakFlags;
    payload["skbHeadlessYaw"] = a->headlessYawOffset;
}

void ApplyAI(Actor* actor, const nlohmann::json& payload) {
    auto* a = reinterpret_cast<EnSkb*>(actor);
    a->actionState = payload.value("skbActionState", (u8)0);
    a->breakFlags = payload.value("skbBreakFlags", (u8)0);
    a->headlessYawOffset = payload.value("skbHeadlessYaw", (s16)0);
}

}  // namespace

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_SKB, &RegisterAC, &ClearACHits, &SerializeAI, &ApplyAI }));
