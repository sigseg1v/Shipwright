#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/JsonConversions.hpp"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

extern "C" {
#include "variables.h"
extern PlayState* gPlayState;
}

/**
 * UPDATE_CLIENT_STATE
 *
 * Contains a small subset of data that is cached on the server and important for the client to know for various reasons
 *
 * Sent on various events, such as changing scenes, soft resetting, finishing the game, opening file select, etc.
 *
 * Note: This packet should be cross version compatible, so if you add anything here don't assume all clients will be
 * providing it, consider doing a `contains` check before accessing any version specific data
 */

nlohmann::json Anchor::PrepClientState() {
    nlohmann::json payload;
    payload["name"] = CVarGetString(CVAR_REMOTE_ANCHOR("Name"), "");
    payload["color"] = CVarGetColor24(CVAR_REMOTE_ANCHOR("Color.Value"), { 100, 255, 100 });
    payload["clientVersion"] = clientVersion;
    payload["features"] = selfFeatures;
    payload["teamId"] = CVarGetString(CVAR_REMOTE_ANCHOR("TeamId"), "default");
    payload["online"] = true;

    if (IsSaveLoaded()) {
        payload["seed"] = IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
        payload["isSaveLoaded"] = true;
        payload["isGameComplete"] = gSaveContext.ship.stats.gameComplete;
        payload["sceneNum"] = gPlayState->sceneNum;
        payload["curRoomNum"] = gPlayState->roomCtx.curRoom.num;
        payload["entranceIndex"] = gSaveContext.entranceIndex;
    } else {
        payload["seed"] = 0;
        payload["isSaveLoaded"] = false;
        payload["isGameComplete"] = false;
        payload["sceneNum"] = SCENE_ID_MAX;
        payload["curRoomNum"] = -1;
        payload["entranceIndex"] = 0x00;
    }

    return payload;
}

void Anchor::SendPacket_UpdateClientState() {
    nlohmann::json payload;
    payload["type"] = UPDATE_CLIENT_STATE;
    payload["state"] = PrepClientState();

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_UpdateClientState(nlohmann::json payload) {
    uint32_t clientId = payload.at("clientId").get<uint32_t>();

    if (clients.contains(clientId)) {
        AnchorClient client = payload["state"].get<AnchorClient>();
        s16 prevSceneNum = clients[clientId].sceneNum;
        bool prevSaveLoaded = clients[clientId].isSaveLoaded;
        clients[clientId].clientId = clientId;
        clients[clientId].name = client.name;
        clients[clientId].color = client.color;
        clients[clientId].clientVersion = client.clientVersion;
        clients[clientId].features = client.features;
        clients[clientId].teamId = client.teamId;
        clients[clientId].online = client.online;
        clients[clientId].seed = client.seed;
        clients[clientId].isSaveLoaded = client.isSaveLoaded;
        clients[clientId].isGameComplete = client.isGameComplete;
        clients[clientId].sceneNum = client.sceneNum;
        clients[clientId].curRoomNum = client.curRoomNum;
        clients[clientId].entranceIndex = client.entranceIndex;

        // Inverse late-join trigger: a peer just arrived in our scene (or
        // just finished loading their save while in our scene). If we're the
        // current authority for this scene, send them a full snapshot of the
        // live synced enemies so they don't sit empty waiting for our next
        // scene-spawn (which won't fire until WE re-enter).
        bool peerEnteredOurScene =
            client.isSaveLoaded && IsSaveLoaded() && client.sceneNum == gPlayState->sceneNum &&
            (prevSceneNum != client.sceneNum || !prevSaveLoaded) && clientId != ownClientId;
        if (peerEnteredOurScene && IsAuthorityForCurrentScene()) {
            SendPacket_EnemyFullSnapshot(clientId);
        }
    }
}
