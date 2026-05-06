#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/EnemySync.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
extern PlayState* gPlayState;
}

/**
 * ENEMY_DAMAGE
 *
 * Non-authority -> authority, unicast. Tells the authority that a player on
 * the sending client landed a hit on a synced enemy. The authority replays
 * the hit by writing into the actor's collider/colChkInfo, then lets the
 * vanilla AI consume it on the next tick (yielding the correct stagger /
 * stun / death response).
 */

void Anchor::SendPacket_EnemyDamage(uint32_t enemyNetId, uint32_t targetClientId, u8 damage, u8 damageEffect) {
    nlohmann::json payload;
    payload["type"] = ENEMY_DAMAGE;
    payload["targetClientId"] = targetClientId;
    payload["enemyNetId"] = enemyNetId;
    payload["damage"] = damage;
    payload["damageEffect"] = damageEffect;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_EnemyDamage(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }
    uint32_t enemyNetId = payload.value("enemyNetId", (uint32_t)0);
    if (enemyNetId == 0) {
        return;
    }
    auto it = enemyNetIdToActor.find(enemyNetId);
    if (it == enemyNetIdToActor.end() || it->second == nullptr) {
        return;
    }
    Actor* actor = it->second;

    u8 damage = payload.value("damage", (u8)0);
    u8 damageEffect = payload.value("damageEffect", (u8)0);

    actor->colChkInfo.damage = damage;
    actor->colChkInfo.damageEffect = damageEffect;
    if (actor->id == ACTOR_EN_SKB) {
        EnSkb* skb = reinterpret_cast<EnSkb*>(actor);
        // Stalchild's AI watches `collider.base.acFlags & 2` to register hits.
        skb->collider.base.acFlags |= AC_HIT;
    }
}
