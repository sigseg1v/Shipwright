#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/JsonConversions.hpp"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/OTRGlobals.h"

extern "C" {
#include "macros.h"
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

        // Proactive save mirror: when a peer just finished loading their
        // save and we're the room owner with a save loaded, push our
        // current save state to the team. The joiner also sends
        // REQUEST_TEAM_STATE on OnLoadGame which round-trips back to us,
        // but doing this eagerly closes the brief window where they
        // play on their pre-merge save. UPDATE_TEAM_STATE itself is
        // gated by syncItemsAndFlags and team membership, so it's a
        // no-op for cross-team or sync-disabled rooms.
        bool peerJustLoadedSave =
            client.isSaveLoaded && !prevSaveLoaded && clientId != ownClientId;
        if (peerJustLoadedSave && roomState.ownerClientId == ownClientId &&
            IsSaveLoaded() && roomState.syncItemsAndFlags) {
            SendPacket_UpdateTeamState();
        }

        // Zone-follow (boss scenes only): when the room owner enters a
        // boss room, pull every other client to the same entrance so the
        // fight happens together. Outside boss scenes players roam
        // independently -- per-scene authority handles enemy sync, so
        // splitting up the dungeon into rooms with different authorities
        // is the intended flow. Per-client opt-in via CVar (default on).
        // We reuse the TELEPORT_TO machinery, but omit respawnFlag so
        // the joiner lands at the entrance's natural spawn point.
        auto isBossScene = [](s16 s) {
            switch (s) {
                case SCENE_DEKU_TREE_BOSS:
                case SCENE_DODONGOS_CAVERN_BOSS:
                case SCENE_JABU_JABU_BOSS:
                case SCENE_FOREST_TEMPLE_BOSS:
                case SCENE_FIRE_TEMPLE_BOSS:
                case SCENE_WATER_TEMPLE_BOSS:
                case SCENE_SPIRIT_TEMPLE_BOSS:
                case SCENE_SHADOW_TEMPLE_BOSS:
                case SCENE_GANONDORF_BOSS:
                case SCENE_GANON_BOSS:
                    return true;
                default:
                    return false;
            }
        };
        bool ownerChangedScene = clientId == roomState.ownerClientId && clientId != ownClientId &&
                                 client.isSaveLoaded && client.sceneNum != SCENE_ID_MAX &&
                                 isBossScene(client.sceneNum) &&
                                 (prevSceneNum != client.sceneNum || !prevSaveLoaded);
        bool followEnabled = CVarGetInteger(CVAR_REMOTE_ANCHOR("FollowHostZone"), 1) != 0;
        if (ownerChangedScene && followEnabled && IsSaveLoaded() &&
            gPlayState->sceneNum != client.sceneNum) {
            s32 entranceIndex = client.entranceIndex;
            gPlayState->nextEntranceIndex = entranceIndex;
            gPlayState->transitionTrigger = TRANS_TRIGGER_START;
            gPlayState->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
            gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK_FAST;
            // Suppress any void damage that might fire mid-transition
            // (e.g., owner entering a dungeon while we're falling).
            static HOOK_ID followVoidHookId = 0;
            followVoidHookId = REGISTER_VB_SHOULD(VB_INFLICT_VOID_DAMAGE, {
                *should = false;
                GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnVanillaBehavior>(
                    followVoidHookId);
            });
        }
    }
}
