#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include <cstdio>

#include "macros.h"
#include "variables.h"
#include "functions.h"
extern "C" {
extern PlayState* gPlayState;
}

// FOLIAGE_DESTROY / FOLIAGE_SNAPSHOT (FEATURE_FOLIAGE_SYNC)
//
// Identifying a specific bush/grass clump across clients is the
// trickiest part: actor pointers obviously aren't stable, params
// often collide (an entire room of grass might share params=0), and
// spawn order is non-deterministic. We use the actor's home position
// (the spawn-time pos that survives any in-game movement) plus its
// id and params, formatted at one decimal place to absorb tiny
// floating-point drift.

std::string Anchor::MakeFoliageId(const Actor* actor) {
    char buf[80];
    std::snprintf(buf, sizeof(buf), "%d:%d:%.1f:%.1f:%.1f", (int)actor->id, (int)actor->params,
                  actor->home.pos.x, actor->home.pos.y, actor->home.pos.z);
    return buf;
}

void Anchor::SendPacket_FoliageDestroy(s16 sceneNum, const std::string& foliageId) {
    nlohmann::json payload;
    payload["type"] = FOLIAGE_DESTROY;
    payload["sceneNum"] = sceneNum;
    payload["foliageId"] = foliageId;
    SendJsonToRemote(payload);
}

// Walk the current scene's actor list and Actor_Kill any foliage whose
// id is in the destroyed set for the current scene. Used by
// FOLIAGE_SNAPSHOT (apply the whole set at once after entering a
// scene) and FOLIAGE_DESTROY (apply the just-arrived single id).
static void KillDestroyedFoliageInCurrentScene(const std::set<std::string>& set) {
    if (gPlayState == nullptr || set.empty()) {
        return;
    }
    // Foliage actors live in ACTORCAT_PROP.
    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].head;
    while (actor != NULL) {
        Actor* next = actor->next;
        if (actor->id == ACTOR_EN_KUSA && actor->update != NULL) {
            std::string id = Anchor::Instance->MakeFoliageId(actor);
            if (set.count(id)) {
                Actor_Kill(actor);
            }
        }
        actor = next;
    }
}

void Anchor::HandlePacket_FoliageDestroy(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("foliageId")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    std::string foliageId = payload["foliageId"].get<std::string>();

    destroyedFoliage[sceneNum].insert(foliageId);

    if (IsSaveLoaded() && gPlayState != nullptr && gPlayState->sceneNum == sceneNum) {
        std::set<std::string> singleton = { foliageId };
        KillDestroyedFoliageInCurrentScene(singleton);
    }
}

void Anchor::SendPacket_FoliageRegrow(s16 sceneNum, const std::string& foliageId, f32 x, f32 y, f32 z, s16 rotY,
                                      s16 params) {
    nlohmann::json payload;
    payload["type"] = FOLIAGE_REGROW;
    payload["sceneNum"] = sceneNum;
    payload["foliageId"] = foliageId;
    payload["x"] = x;
    payload["y"] = y;
    payload["z"] = z;
    payload["rotY"] = rotY;
    payload["params"] = params;
    SendJsonToRemote(payload);
}

// TYPE_1 deku shrubs regrow on a timer after being sliced. The
// originating client tracks ACTOR_FLAG_GRASS_DESTROYED transitioning
// back to false on its still-alive EnKusa and broadcasts this packet.
// Peers (which Actor_Killed their copy on the prior FOLIAGE_DESTROY)
// drop the id from their per-scene destroyedFoliage set so future
// snapshots/inits don't immediately re-kill it, and spawn a fresh
// EnKusa at the carried home position. The new instance follows its
// own normal lifecycle from there.
void Anchor::HandlePacket_FoliageRegrow(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("foliageId")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    std::string foliageId = payload["foliageId"].get<std::string>();

    auto setIt = destroyedFoliage.find(sceneNum);
    if (setIt != destroyedFoliage.end()) {
        setIt->second.erase(foliageId);
    }

    if (!IsSaveLoaded() || gPlayState == nullptr || gPlayState->sceneNum != sceneNum) {
        return;
    }

    f32 x = payload.value("x", 0.0f);
    f32 y = payload.value("y", 0.0f);
    f32 z = payload.value("z", 0.0f);
    s16 rotY = payload.value("rotY", (s16)0);
    s16 params = payload.value("params", (s16)0);

    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_KUSA, x, y, z, 0, rotY, 0, params);
}

void Anchor::HandlePacket_FoliageSnapshot(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("foliageIds")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    std::set<std::string> incoming;
    for (const auto& id : payload["foliageIds"]) {
        incoming.insert(id.get<std::string>());
    }

    auto& set = destroyedFoliage[sceneNum];
    for (const auto& id : incoming) {
        set.insert(id);
    }

    if (IsSaveLoaded() && gPlayState != nullptr && gPlayState->sceneNum == sceneNum) {
        KillDestroyedFoliageInCurrentScene(incoming);
    }
}
