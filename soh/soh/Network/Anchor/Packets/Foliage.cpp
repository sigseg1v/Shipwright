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
