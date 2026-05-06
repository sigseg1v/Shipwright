#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/EnemySync.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include "soh/ObjectExtension/ObjectExtension.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

/**
 * ENEMY_DEATH
 *
 * Authority -> peers, room-broadcast. Tells non-authority clients that the
 * given network ID is dead. Non-authority responds by killing the local
 * actor; the engine handles the death animation if it was already in
 * progress (driven by the synced actionState in ENEMY_UPDATE), otherwise
 * we just `Actor_Kill` here.
 *
 * Loot is intentionally not encoded here in v1 -- Stalchild calls
 * `Item_DropCollectibleRandom` from its death-animation tail, and on
 * non-authority clients the AI is suppressed so no drop happens. Future
 * iterations should make the authority pre-roll the drop and broadcast it
 * with the death event so all clients see the same item.
 */

void Anchor::SendPacket_EnemyDeath(uint32_t enemyNetId) {
    nlohmann::json payload;
    payload["type"] = ENEMY_DEATH;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["enemyNetId"] = enemyNetId;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_EnemyDeath(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }
    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    if (sceneNum != gPlayState->sceneNum) {
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
    Actor_Kill(it->second);
    enemyNetIdToActor.erase(it);
}
