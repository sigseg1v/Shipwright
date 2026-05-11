#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/EnemySync.h"
#include "soh/Network/Anchor/EnemySync/Registry.h"
#include "soh/Network/Anchor/JsonConversions.hpp"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include "soh/ObjectExtension/ObjectExtension.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"
extern "C" {
extern PlayState* gPlayState;
}

/**
 * ENEMY_UPDATE
 *
 * Authority -> peers, 60Hz, room-broadcast. Carries a batch of all live
 * synced enemies in the current scene with their pos/rot/vel/hp + per-family
 * AI state extras.
 *
 * Receiver mirrors PLAYER_UPDATE's pattern: assign pos/rot directly to the
 * actor every packet, no prev/target interpolation buffer. Authority sends
 * once per game tick so the receive cadence already matches the render
 * cadence, the same way DummyPlayer_Update consumes PLAYER_UPDATE.
 *
 * Send side lives in EnemySync.cpp::EnemySync_TickAuthorityBroadcast.
 */

void Anchor::HandlePacket_EnemyUpdate(nlohmann::json payload) {
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
        uint32_t id = e.value("id", (uint32_t)0);
        if (id == 0) {
            continue;
        }

        auto it = enemyNetIdToActor.find(id);
        if (it == enemyNetIdToActor.end() || it->second == nullptr) {
            // Snapshot for an actor we haven't seen yet (race with
            // ENEMY_SPAWN). Skip; we'll catch up next tick.
            continue;
        }
        Actor* actor = it->second;

        Vec3f pos = e.value("pos", Vec3f{ 0, 0, 0 });
        Vec3s rot = e.value("rot", Vec3s{ 0, 0, 0 });

        actor->world.pos = pos;
        actor->shape.rot = rot;
        actor->world.rot.y = rot.y;

        actor->velocity.x = e.value("velX", 0.0f);
        actor->velocity.y = e.value("velY", 0.0f);
        actor->velocity.z = e.value("velZ", 0.0f);
        actor->colChkInfo.health = e.value("hp", (u8)0);

        const EnemyFamily* family = EnemyFamilyRegistry::Find(actor->id);
        if (family != nullptr && family->applyAI != nullptr) {
            family->applyAI(actor, e);
        }
    }
}
