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

// Mirror of EnemySync.cpp::IsSyncableEnemy. Kept here as a static so the
// snapshot writer doesn't take a dependency on the EnemySync TU's internals.
// Update both lists when a new actor type joins the synced set.
static bool IsSnapshottableEnemy(s16 actorId) {
    switch (actorId) {
        case ACTOR_EN_SKB:
        case ACTOR_EN_DEKUBABA:
        case ACTOR_EN_KAREBABA:
        case ACTOR_EN_DEKUNUTS:
        case ACTOR_EN_GOMA:
        case ACTOR_EN_ST:
        case ACTOR_EN_SW:
            return true;
        default:
            return false;
    }
}

/**
 * ENEMY_FULL_SNAPSHOT
 *
 * Authority -> single peer (unicast). Used for late-join: when a peer's
 * sceneNum changes to match the authority's, the authority sends one full
 * dump of every live synced enemy so the joiner can spawn them locally.
 *
 * For Phase 2 we keep this minimal: actor id, params, position, rotation,
 * net id, hp. No per-family extras (those will arrive in the first
 * ENEMY_UPDATE tick after spawn). Phase 5 will harden this for
 * authority-handoff scenarios.
 */

void Anchor::SendPacket_EnemyFullSnapshot(uint32_t targetClientId) {
    if (!IsSaveLoaded() || !IsAuthorityForCurrentScene()) {
        return;
    }

    nlohmann::json enemies = nlohmann::json::array();

    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_ENEMY].head;
    while (actor != NULL) {
        if (IsSnapshottableEnemy(actor->id)) {
            EnemyNetState* state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
            if (state != nullptr && state->isSynced && state->isAuthority && state->enemyNetId != 0) {
                nlohmann::json e;
                e["enemyNetId"] = state->enemyNetId;
                e["actorId"] = actor->id;
                e["params"] = actor->params;
                e["pos"] = actor->world.pos;
                e["rot"] = actor->shape.rot;
                e["hp"] = actor->colChkInfo.health;
                e["roomNum"] = actor->room;
                enemies.push_back(e);
            }
        }
        actor = actor->next;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_FULL_SNAPSHOT;
    payload["targetClientId"] = targetClientId;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["enemies"] = enemies;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_EnemyFullSnapshot(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }
    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }
    if (!payload.contains("enemies") || !payload["enemies"].is_array()) {
        return;
    }

    for (auto& e : payload["enemies"]) {
        uint32_t enemyNetId = e.value("enemyNetId", (uint32_t)0);
        if (enemyNetId == 0) {
            continue;
        }
        auto existing = enemyNetIdToActor.find(enemyNetId);
        if (existing != enemyNetIdToActor.end() && existing->second != nullptr) {
            continue;
        }

        s16 actorId = e.value("actorId", (s16)0);
        s16 params = e.value("params", (s16)0);
        Vec3f pos = e.value("pos", Vec3f{ 0, 0, 0 });
        Vec3s rot = e.value("rot", Vec3s{ 0, 0, 0 });

        Actor* spawned = Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId, pos.x, pos.y, pos.z, rot.x, rot.y,
                                     rot.z, params);
        if (spawned == nullptr) {
            continue;
        }
        spawned->colChkInfo.health = e.value("hp", (u8)0);

        EnemyNetState state;
        state.enemyNetId = enemyNetId;
        state.isAuthority = false;
        state.isSynced = true;
        ObjectExtension::GetInstance().Set<EnemyNetState>(spawned, std::move(state));
        enemyNetIdToActor[enemyNetId] = spawned;
    }
}
