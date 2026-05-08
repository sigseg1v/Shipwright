#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

// ITEM_SPAWN / ITEM_COLLECT replicate a single collectible drop --
// currently only the drops produced by EnIshi rock smashes -- across
// clients. The rock-destroying client rolls Item_DropCollectibleRandom
// locally (which is non-deterministic across peers), captures the
// resulting EnItem00 actors and broadcasts each one's exact position
// and params with a unique itemId. Peers spawn their own EnItem00 at
// the same position and bind it to the same itemId.
//
// When the EnItem00 dies on any client (player walked over it and
// picked it up, or the ~13s despawn timer ran out), that client
// broadcasts ITEM_COLLECT with the itemId. Peers find their bound
// local actor by itemId and Actor_Kill it so the world stays in sync.
// Rupee/heart/etc. count is reconciled separately via the existing
// FEATURE_SHARED_RUPEES path on the originator's pickup.

void Anchor::SendPacket_ItemSpawn(s16 sceneNum, uint64_t itemId, f32 x, f32 y, f32 z, s16 params) {
    nlohmann::json payload;
    payload["type"] = ITEM_SPAWN;
    payload["sceneNum"] = sceneNum;
    payload["itemId"] = itemId;
    payload["x"] = x;
    payload["y"] = y;
    payload["z"] = z;
    payload["params"] = params;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_ItemSpawn(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("x") || !payload.contains("y") ||
        !payload.contains("z") || !payload.contains("params") || !payload.contains("itemId")) {
        return;
    }
    if (!IsSaveLoaded() || gPlayState == nullptr) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    if (gPlayState->sceneNum != sceneNum) {
        return;
    }

    Vec3f pos;
    pos.x = payload["x"].get<f32>();
    pos.y = payload["y"].get<f32>();
    pos.z = payload["z"].get<f32>();
    s16 params = payload["params"].get<s16>();
    uint64_t itemId = payload["itemId"].get<uint64_t>();

    // Spawn a normal collectible on the ground. NOTE: do not set the
    // 0x8000 bit -- in EnItem00_Init that flag means "give item directly
    // to Link" (auto-collect, used by Item_Give-style flows), not "skip
    // random roll". The originator already filtered the drop type
    // through func_8001F404 in Item_DropCollectibleRandom, so an extra
    // pass on this side is effectively idempotent for the rock drop
    // table.
    Actor* spawned = (Actor*)Item_DropCollectible(gPlayState, &pos, params);
    if (spawned != NULL) {
        itemActorToId[spawned] = itemId;
        itemIdToActor[itemId] = spawned;
    }
}

void Anchor::SendPacket_ItemCollect(uint64_t itemId) {
    nlohmann::json payload;
    payload["type"] = ITEM_COLLECT;
    payload["itemId"] = itemId;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_ItemCollect(nlohmann::json payload) {
    if (!payload.contains("itemId")) return;
    uint64_t itemId = payload["itemId"].get<uint64_t>();

    auto it = itemIdToActor.find(itemId);
    if (it == itemIdToActor.end()) return;
    Actor* actor = it->second;

    // Skip if the actor's already gone (e.g. local pickup raced the
    // remote pickup). The OnActorKill hook for EnItem00 will erase the
    // map entry on its own; we just leave it in place.
    if (actor != NULL && actor->update != NULL) {
        Actor_Kill(actor);
    }

    itemIdToActor.erase(it);
    auto a2i = itemActorToId.find(actor);
    if (a2i != itemActorToId.end()) {
        itemActorToId.erase(a2i);
    }
}
