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

// ITEM_SPAWN replicates a single collectible drop -- currently only the
// drops produced by EnIshi rock smashes -- across clients. The rock-
// destroying client rolls Item_DropCollectibleRandom locally (which is
// non-deterministic across peers), captures the resulting EnItem00's
// actual params and world position, and broadcasts those exact values
// here. Peers call Item_DropCollectible (the deterministic variant)
// with `params | 0x8000` so the spawned EnItem00 takes the params we
// pass directly instead of being re-rolled through the drop table.
//
// Each peer's spawned EnItem00 is independent: when the local player
// collects one, only their wallet ticks up locally; the shared rupee
// total is reconciled separately via FEATURE_SHARED_RUPEES. Uncollected
// EnItem00 instances despawn on their own ~13-second timer.

void Anchor::SendPacket_ItemSpawn(s16 sceneNum, f32 x, f32 y, f32 z, s16 params) {
    nlohmann::json payload;
    payload["type"] = ITEM_SPAWN;
    payload["sceneNum"] = sceneNum;
    payload["x"] = x;
    payload["y"] = y;
    payload["z"] = z;
    payload["params"] = params;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_ItemSpawn(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("x") || !payload.contains("y") ||
        !payload.contains("z") || !payload.contains("params")) {
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

    // Spawn a normal collectible on the ground. NOTE: do not set the
    // 0x8000 bit -- in EnItem00_Init that flag means "give item directly
    // to Link" (auto-collect, used by Item_Give-style flows), not "skip
    // random roll". Setting it here would silently award the drop to the
    // local player instead of spawning a pickable rupee. The originator
    // already filtered the drop type through func_8001F404 in
    // Item_DropCollectibleRandom, so an extra pass on this side is
    // effectively idempotent for the rock drop table.
    Item_DropCollectible(gPlayState, &pos, params);
}
