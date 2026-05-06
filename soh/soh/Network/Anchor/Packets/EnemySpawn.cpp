#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/EnemySync.h"
#include "soh/Network/Anchor/JsonConversions.hpp"
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
 * ENEMY_SPAWN
 *
 * Authority broadcasts when a synced enemy comes into existence (scene load,
 * scripted spawn, splitter wave). Non-authority clients spawn the actor
 * locally with the supplied params and bind the network ID via
 * ObjectExtension. Receivers self-filter on sceneNum.
 */

void Anchor::SendPacket_EnemySpawn(Actor* actor, uint32_t enemyNetId) {
    if (!IsSaveLoaded() || actor == nullptr) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_SPAWN;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = actor->room;
    payload["actorId"] = actor->id;
    payload["params"] = actor->params;
    payload["pos"] = actor->world.pos;
    payload["rot"] = actor->shape.rot;
    payload["enemyNetId"] = enemyNetId;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_EnemySpawn(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }
    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }

    s16 actorId = payload.value("actorId", (s16)0);
    s16 params = payload.value("params", (s16)0);
    Vec3f pos = payload.value("pos", Vec3f{ 0, 0, 0 });
    Vec3s rot = payload.value("rot", Vec3s{ 0, 0, 0 });
    uint32_t enemyNetId = payload.value("enemyNetId", (uint32_t)0);
    if (enemyNetId == 0) {
        return;
    }

    // Already have it? Idempotent.
    auto existing = enemyNetIdToActor.find(enemyNetId);
    if (existing != enemyNetIdToActor.end() && existing->second != nullptr) {
        return;
    }

    Actor* spawned = Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z,
                                 params);
    if (spawned == nullptr) {
        return;
    }

    EnemyNetState state;
    state.enemyNetId = enemyNetId;
    state.isAuthority = false;
    state.isSynced = true;
    ObjectExtension::GetInstance().Set<EnemyNetState>(spawned, std::move(state));
    enemyNetIdToActor[enemyNetId] = spawned;
}
