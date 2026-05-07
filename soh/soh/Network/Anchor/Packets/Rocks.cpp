#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include <cstdio>

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

// ROCK_DESTROY / ROCK_SNAPSHOT (FEATURE_ROCK_SYNC)
//
// Same identification scheme as foliage: actor->home.pos is set at
// spawn (EnIshi_SnapToFloor) and never moves even while the rock is
// lifted/flying, so it's a stable cross-client id when combined with
// id and params. We only sync ACTOR_EN_ISHI today (small grey rocks
// that drop a collectible when smashed and large silver boulders that
// also clear a switch flag); sister actors like ovl_Obj_Hamishi
// (boulder pile) are not yet covered.
//
// Behaviour: when a rock is Actor_Killed locally (smash by sword/
// hammer/explosion in Wait, or ground/wall impact in Fly after a
// throw), we broadcast ROCK_DESTROY. Peers add to their per-scene
// destroyedRocks set and Actor_Kill any matching live actor. Peers do
// not see the smash visuals (the rock just disappears) and the dropped
// rupee actor only spawns on the originating client; the rupee count
// itself still syncs via FEATURE_SHARED_RUPEES once the originator
// collects it.

std::string Anchor::MakeRockId(const Actor* actor) {
    char buf[80];
    std::snprintf(buf, sizeof(buf), "%d:%d:%.1f:%.1f:%.1f", (int)actor->id, (int)actor->params,
                  actor->home.pos.x, actor->home.pos.y, actor->home.pos.z);
    return buf;
}

void Anchor::SendPacket_RockDestroy(s16 sceneNum, const std::string& rockId) {
    nlohmann::json payload;
    payload["type"] = ROCK_DESTROY;
    payload["sceneNum"] = sceneNum;
    payload["rockId"] = rockId;
    SendJsonToRemote(payload);
}

static void KillDestroyedRocksInCurrentScene(const std::set<std::string>& set) {
    if (gPlayState == nullptr || set.empty()) {
        return;
    }
    // Rock actors live in ACTORCAT_PROP.
    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].head;
    while (actor != NULL) {
        Actor* next = actor->next;
        if (actor->id == ACTOR_EN_ISHI && actor->update != NULL) {
            std::string id = Anchor::Instance->MakeRockId(actor);
            if (set.count(id)) {
                Actor_Kill(actor);
            }
        }
        actor = next;
    }
}

void Anchor::HandlePacket_RockDestroy(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("rockId")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    std::string rockId = payload["rockId"].get<std::string>();

    destroyedRocks[sceneNum].insert(rockId);

    if (IsSaveLoaded() && gPlayState != nullptr && gPlayState->sceneNum == sceneNum) {
        std::set<std::string> singleton = { rockId };
        KillDestroyedRocksInCurrentScene(singleton);
    }
}

void Anchor::HandlePacket_RockSnapshot(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("rockIds")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    std::set<std::string> incoming;
    for (const auto& id : payload["rockIds"]) {
        incoming.insert(id.get<std::string>());
    }

    auto& set = destroyedRocks[sceneNum];
    for (const auto& id : incoming) {
        set.insert(id);
    }

    if (IsSaveLoaded() && gPlayState != nullptr && gPlayState->sceneNum == sceneNum) {
        KillDestroyedRocksInCurrentScene(incoming);
    }
}
