#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/EnemySync.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "src/overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"
#include "src/overlays/actors/ovl_En_Dekunuts/z_en_dekunuts.h"
#include "src/overlays/actors/ovl_En_Goma/z_en_goma.h"
#define this thisx
#include "src/overlays/actors/ovl_En_St/z_en_st.h"
#undef this
#include "src/overlays/actors/ovl_En_Sw/z_en_sw.h"
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

    // Per-family: flag AC_HIT on the collider(s) the actor's UpdateDamage /
    // action-dispatch fn polls. Without this the authority's vanilla AI sees
    // colChkInfo.damage filled in but no incoming-hit signal, so it never
    // routes through the stagger / death paths -- the damage just sits there
    // until the next real local hit nudges things forward.
    switch (actor->id) {
        case ACTOR_EN_SKB: {
            EnSkb* a = reinterpret_cast<EnSkb*>(actor);
            a->collider.base.acFlags |= AC_HIT;
            break;
        }
        case ACTOR_EN_DEKUBABA: {
            EnDekubaba* a = reinterpret_cast<EnDekubaba*>(actor);
            a->collider.base.acFlags |= AC_HIT;
            break;
        }
        case ACTOR_EN_KAREBABA: {
            EnKarebaba* a = reinterpret_cast<EnKarebaba*>(actor);
            a->headCollider.base.acFlags |= AC_HIT;
            a->bodyCollider.base.acFlags |= AC_HIT;
            break;
        }
        case ACTOR_EN_DEKUNUTS: {
            EnDekunuts* a = reinterpret_cast<EnDekunuts*>(actor);
            a->collider.base.acFlags |= AC_HIT;
            break;
        }
        case ACTOR_EN_GOMA: {
            EnGoma* a = reinterpret_cast<EnGoma*>(actor);
            a->colCyl1.base.acFlags |= AC_HIT;
            a->colCyl2.base.acFlags |= AC_HIT;
            break;
        }
        case ACTOR_EN_ST: {
            EnSt* a = reinterpret_cast<EnSt*>(actor);
            a->colSph.base.acFlags |= AC_HIT;
            for (int i = 0; i < 6; i++) {
                a->colCylinder[i].base.acFlags |= AC_HIT;
            }
            break;
        }
        case ACTOR_EN_SW: {
            EnSw* a = reinterpret_cast<EnSw*>(actor);
            a->collider.base.acFlags |= AC_HIT;
            break;
        }
        default:
            break;
    }
}
