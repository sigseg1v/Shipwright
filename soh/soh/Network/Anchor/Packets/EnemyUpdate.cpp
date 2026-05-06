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
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
extern PlayState* gPlayState;
}

/**
 * ENEMY_UPDATE
 *
 * Authority -> peers, ~15Hz, room-broadcast. Carries a batch of all live
 * synced enemies in the current scene with their pos/rot/vel/hp + per-family
 * AI state extras. Non-authority clients write back into the actor struct
 * and reset the action func / animation as needed.
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

        if (actor->id == ACTOR_EN_SKB) {
            EnSkb* skb = reinterpret_cast<EnSkb*>(actor);
            skb->actionState = e.value("skbActionState", (u8)0);
            skb->breakFlags = e.value("skbBreakFlags", (u8)0);
            skb->headlessYawOffset = e.value("skbHeadlessYaw", (s16)0);
        }
    }
}
