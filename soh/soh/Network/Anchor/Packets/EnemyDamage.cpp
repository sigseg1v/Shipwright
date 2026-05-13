#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/EnemySync.h"
#include "soh/Network/Anchor/EnemySync/Helpers.h"
#include "soh/Network/Anchor/EnemySync/Registry.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"
extern "C" {
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
 *
 * Per-family AC_HIT flagging is delegated to EnemyFamily::setACHits so the
 * collider layout for each actor stays in its own Families/<Name>.cpp -- this
 * file does not need to grow a switch statement (or pull in actor-overlay
 * headers) for every newly synced enemy.
 */

void Anchor::SendPacket_EnemyDamage(uint32_t enemyNetId, uint32_t targetClientId, u8 damage, u8 damageEffect,
                                    u16 attackerActorId) {
    nlohmann::json payload;
    payload["type"] = ENEMY_DAMAGE;
    payload["targetClientId"] = targetClientId;
    payload["enemyNetId"] = enemyNetId;
    payload["damage"] = damage;
    payload["damageEffect"] = damageEffect;
    payload["attackerActorId"] = attackerActorId;
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
    u16 attackerActorId = payload.value("attackerActorId", (u16)0);

    actor->colChkInfo.damage = damage;
    actor->colChkInfo.damageEffect = damageEffect;

    // Stash the attacker id so Set{Cyl,JntSph}AcHit can stamp it into the
    // dummy actor that `collider.base.ac` points at. Overlays that branch on
    // `ac->id` (EnHintnuts puzzle scrub: nutsball -> BeginRun vs anything
    // else -> Burrow) need this to take the correct path on a synthesized
    // hit. Reset to 0 right after so a follow-up family that doesn't ship
    // an attacker doesn't inherit a stale id.
    EnemySyncHelpers::PendingAttackerActorId() = (s16)attackerActorId;

    // Per-family: flag AC_HIT on the collider(s) the actor's UpdateDamage /
    // action-dispatch fn polls. Without this the authority's vanilla AI sees
    // colChkInfo.damage filled in but no incoming-hit signal, so it never
    // routes through the stagger / death paths -- the damage just sits there
    // until the next real local hit nudges things forward.
    const EnemyFamily* family = EnemyFamilyRegistry::Find(actor->id);
    if (family != nullptr && family->setACHits != nullptr) {
        family->setACHits(actor);
    }

    EnemySyncHelpers::PendingAttackerActorId() = 0;
}
